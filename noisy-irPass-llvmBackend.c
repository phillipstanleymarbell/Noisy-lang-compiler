/*
	Authored 2008--2011, Phillip Stanley-Marbell
 
	All rights reserved.

	Redistribution and use in source and binary forms, with or without 
	modification, are permitted provided that the following conditions
	are met:

	*	Redistributions of source code must retain the above
		copyright notice, this list of conditions and the following
		disclaimer.

	*	Redistributions in binary form must reproduce the above
		copyright notice, this list of conditions and the following
		disclaimer in the documentation and/or other materials
		provided with the distribution.

	*	Neither the name of the author nor the names of its
		contributors may be used to endorse or promote products
		derived from this software without specific prior written 
		permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
	FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
	COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
	BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
	CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
	LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN 
	ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
	POSSIBILITY OF SUCH DAMAGE.
*/
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <fcntl.h>
#include "flextypes.h"
#include "flex.h"
#include "libsets.h"
#include "optimizer.h"

/*									*/
/*	So tools like Clang static analyzer know that fatal() is	*/
/*	like an assertion.						*/
/*									*/
extern void		fatal(char *) __attribute__ ((noreturn));
extern void		error(char*);

extern char		Esanity[],
			Ebadptreearithnodetype[],
			Emalloc[],
			Eimmediatecycle[];

extern char*		pred_ptreeopstrs[];


/*									*/
/*	Approach: should take as input a predicate tree, and return	*/
/*	the optimized pred tree. It should be called by other		*/
/*	routines such as the main body of Salc, which handle the	*/
/*	reading / parsing / emission, or by the interactive command	*/
/*	interface.							*/
/*									*/
/*	Given a subtree which is an arithmetic expression, it should	*/
/*	generate LLVM instruction sequence for the expression, pass	*/
/*	to LLVM library for optimization, then convert back to		*/
/*	ptree. In principle, we could also have LLVM JIT the		*/
/*	expression, returning a function pointer (?) to the JITed	*/
/*	code, which we store in the pred node's "fn" field.		*/
/*									*/
/*	While this back and forth convertion might seem tedious,	*/
/*	the alternative is to use LLVM three-address code and data	*/
/*	structures explicitly for all expressions, which at the		*/
/*	moment seems unwieldy...					*/
/*									*/
/*	While optimizing arithmetic expressions via LLVM has a		*/
/*	straightforward path to implementation, it is unlikely to	*/
/*	provide significant performance benefits, since, anecdotally,	*/
/*	the larger influence is from predicates that necessitate	*/
/*	walking the whole universe (quantifiers), and from having	*/
/*	to apply predicates to all members of large universes.		*/
/*	Larger benefits are likely to come from optimizations for	*/
/*	restricting the regions of the universe that must be searched,	*/
/*	given restrictions embodied in the predicates; one way in	*/
/*	which this can be done is to add an expression denoting the	*/
/*	range of such a universe region in a conjunction to the		*/
/*	predicate, then optimizing---if the optimized expression	*/
/*	reduces to false, then we know statically that the region	*/
/*	should not be searched.						*/
/*									*/
/*	Using this idea, the optimizer should be able to statically	*/
/*	construct new universe definitions which are such restrictions	*/
/*	given a predicate: i.e., take a contiguous universe definition	*/
/*	like								*/
/*									*/
/*		U0 : integers = <-10000 ... 10000>			*/
/*									*/
/*	and split it into something like 				*/
/*									*/
/*		U0 : integers = <-22 ... 0, 1, 4 ... 200, 220>		*/
/*									*/

typedef struct PlistItem PlistItem;
struct PlistItem
{
	Predicate	*p;
	char		*s1;
	char		*s2;
	int		i1;
	int		i2;

	PlistItem	*prev;
	PlistItem	*next;
};
PlistItem	*OptimizerPlistHd, *OptimizerPlistTl;

char*	OPT_SALTYPE2LLVMTYPE[] =
	{
		[TYPE_INTCONST]			= "%I64",
		[TYPE_STRINGCONST]		= "%PTR",
		[TYPE_REALCONST]		= "%R64",
	};

static int	emitcomment(Multiverse *M, Predicate *p, char *buf, int buflen);



static Predicate *
lookup(char *s1, char *s2, int i1, int i2)
{
	PlistItem	*ptr = OptimizerPlistHd;

	while (ptr != NULL)
	{
		if (	(ptr->i1 == i1) &&
			(ptr->i2 == i2) &&
			!strcmp(ptr->s1, s1) &&
			!strcmp(ptr->s2, s2)
			)
		{
			return ptr->p;
		}

		ptr = ptr->next;
	}

	return NULL;
}

static PlistItem*
allocplistitem(Predicate *p, char *s1, char *s2, int i1, int i2)
{
	PlistItem	*item;


	item = (PlistItem*) calloc(1, sizeof(PlistItem));
	if (item == NULL)
	{
		fatal(Emalloc);
	}

	item->p = p;
	item->i1 = i1;
	item->i2 = i2;

	if (s1 != NULL)
	{
		item->s1 = (char*) calloc(strlen(s1)+1, sizeof(char));
		if (item->s1 == NULL)
		{
			fatal(Emalloc);
		}
		strcpy(item->s1, s1);
	}

	if (s2 != NULL)
	{
		item->s2 = (char*) calloc(strlen(s2)+1, sizeof(char));
		if (item->s2 == NULL)
		{
			fatal(Emalloc);
		}
		strcpy(item->s2, s2);
	}

	item->next = NULL;


	return item;
}

static void
insert(Predicate *p, char *s1, char *s2, int i1, int i2)
{
	PlistItem	*item;


	item = allocplistitem(p, s1, s2, i1, i2);
	item->next = NULL;

	if (OptimizerPlistHd == NULL)
	{
		OptimizerPlistHd = OptimizerPlistTl = item;

		return;
	}
	
	OptimizerPlistTl->next = item;

	return;
}

static void
destroyplist(void)
{
	PlistItem	*tmp, *ptr = OptimizerPlistHd;

	while (ptr != NULL)
	{
		tmp = ptr->next;

		free(ptr->s1);
		free(ptr->s2);
		free(ptr);
		ptr = tmp;
	}

	return;
}

static int
emitcomment(Multiverse *M, Predicate *p, char *buf, int buflen)
{
	int	n = 0;


	/*	See note at emitptreeasmetadatawalk() about the address masking		*/
	//	Disable for now...
	n += snprintf(&buf[n], buflen-n, ", !dbg !" FLEX_ULONGFMT "\n", (ulong)p&0xFFFFFFFF);
	//n += pred_printnode(M, p, PRINT_FMT_PREWALK, &buf[n], buflen-n, "; ", "\n");

	return n;
}

static int
emitptreeasmetadatawalk(Multiverse *M, Predicate *p, char *buf, int buflen)
{
	int	n = 0;

	if (p == NULL)
	{
		return 0;
	}

	n += emitptreeasmetadatawalk(M, p->l, &buf[n], buflen-n);
	n += emitptreeasmetadatawalk(M, p->r, &buf[n], buflen-n);

	/*										*/
	/*	NOTE/TODO: As the unique labeling for each ptree output, we use 	*/
	/*	lower-32 bits of node address, as llvm currently only allows unnamed 	*/
	/*	metadata IDs to be 32 bits. This could in principle be a problem when 	*/
	/*	there is some information that differs across nodes in the upper 32	*/
	/*	bits of an address on a 64-bit architecture, but I'd rather take the	*/
	/*	current path, since I well imagine LLVM mending its ways and 		*/
	/*	supporting 64-bit unnamed metadata IDs in the future.			*/
	/*										*/
	n += snprintf(&buf[n], buflen-n, "!" FLEX_ULONGFMT " = metadata !{metadata !\"",
		(ulong)p&0xFFFFFFFF);

	n += pred_printnode(M, p, PRINT_FMT_PREWALK, &buf[n], buflen-n, "", "");
	n += snprintf(&buf[n], buflen-n, "\"}\n");

	return n;
}

static int
emitbinop(Multiverse *M, Predicate *p, char *buf, int buflen, Type type, char *intop, char *realop)
{
	int	n = 0;

	//
	//	TODO: must lookup to see if the child is in the OptimizerPlist, since
	//	in that case we just emit the entry in table. In that case, we should
	//	not be emitting the non-volatile load in emitparamorvar, and should
	//	simply do nothing...
	//

	/*										*/
	/*	We checked in caller to ensure only TYPE_INTCONST and TYPE_REALCONST	*/
	/*										*/
	n += snprintf(&buf[n], buflen-n,
		"\t%%N." FLEX_PTRFMTH " = %s %s %%N." FLEX_PTRFMTH ", %%N." FLEX_PTRFMTH "\t",
		(FlexAddr)p, (type == TYPE_INTCONST ? intop : realop),
		OPT_SALTYPE2LLVMTYPE[type], (FlexAddr)p->l, (FlexAddr)p->r);
	n += emitcomment(M, p, &buf[n], buflen-n);

	return n;
}

static int
emitparamorvar(Multiverse *M, Predicate *p, char *buf, int buflen, char *s1, char *s2, int i1, int i2, Type type)
{
	int		n = 0;
	Predicate	*node;

	node = lookup(s1, s2, i1, i2);
	if (node == NULL)
	{
		insert(p, s1, s2, i1, i2);

		n += snprintf(&buf[n], buflen-n, "\t%%A." FLEX_PTRFMTH " = alloca %s\t\t\t\t",
			(FlexAddr)p, OPT_SALTYPE2LLVMTYPE[type]);
		n += emitcomment(M, p, &buf[n], buflen-n);

		/*	In this case, we want a 'load volatile'		*/
		n += snprintf(&buf[n], buflen-n, 
			"\t%%N." FLEX_PTRFMTH " = load volatile %s* %%A." FLEX_PTRFMTH "\t\t",
			(FlexAddr)p, OPT_SALTYPE2LLVMTYPE[type], (FlexAddr)p);
		n += emitcomment(M, p, &buf[n], buflen-n);	
	}
	else
	{
		n += snprintf(&buf[n], buflen-n, 
			"\t%%N." FLEX_PTRFMTH " = load %s* %%A." FLEX_PTRFMTH "\t\t\t",
			(FlexAddr)node, OPT_SALTYPE2LLVMTYPE[type], (FlexAddr)node);
		n += emitcomment(M, node, &buf[n], buflen-n);
	}

	return n;
}

int
optmz_pred2llvmtxtwalk(Multiverse *M, Predicate *p, Type type, char *buf, int buflen)
{
	int	n = 0;
	char	*constelembuf;


	//
	//	TODO: if we run out of space in print buffer, we should
	//	print a "..." rather than just ending like we do now.
	// 	(see e.g., lsregs for betsy.sal).
	//

	if (p == NULL)
	{
		return 0;
	}

	if (p->l == p || p->r == p)
	{
		error(Eimmediatecycle);

		return 0;
	}

	if (p->paramorconstelem != NULL)
	{
		constelembuf = alloca(MAX_PRINT_BUF);
		if (constelembuf == NULL)
		{
			fatal(Emalloc);
		}

		pred_snprintparamorconst(constelembuf, MAX_PRINT_BUF, p);
	}
	else
	{
		constelembuf = "";
	}


	/*										*/
	/*	For certain types of nodes (e.g., upon seeing the top node of a 	*/
	/*	P_OP_SUM or P_OP_PROD), we should not blindly recurse, since the 	*/
	/*	entire subtree is treated as one "complicated" node type.		*/
	/*										*/
	if (p->op == P_OP_SUM || p->op == P_OP_PRODUCT)
	{
		n += optmz_prednode2llvmstmt(M, p, type, &buf[n], buflen-n);
	}
	else
	{
		n += optmz_pred2llvmtxtwalk(M, p->l, type, &buf[n], buflen-n);
		n += optmz_pred2llvmtxtwalk(M, p->r, type, &buf[n], buflen-n);
		n += optmz_prednode2llvmstmt(M, p, type, &buf[n], buflen-n);
	}

	
	return n;
}

int
optmz_prednode2llvmstmt(Multiverse *M, Predicate *p, Type type, char *buf, int buflen)
{
	int	n = 0;
	Type	tmptype;
	char	*constelembuf;


	if (p->paramorconstelem != NULL)
	{
		constelembuf = (char *)calloc(1, MAX_PRINT_BUF);
		if (constelembuf == NULL)
		{
			fatal(Emalloc);
		}

		pred_snprintparamorconst(constelembuf, MAX_PRINT_BUF, p);
	}
	else
	{
		constelembuf = "";
	}


	/*								*/
	/*	The caller passes in the assumed type, since it might	*/
	/*	be determinable above us in predicate tree, but not	*/
	/*	at present depth in the tree. In cases where we can	*/
	/*	deduce our own type, we do so.				*/
	/*								*/
	/*	In all cases except P_OP_FREE and P_OP_BOUND, we are	*/
	/*	certain about type based on node itself alone. In all 	*/
	/*	other cases, we hope the caller provided the type. 	*/
	/*	If caller didn't know, they will pass in TYPE_UNSET, 	*/
	/*	which is OK, because we map all such cases to a 	*/
	/*	single "unknown" LLVM type.				*/
	/*								*/
	/*	In generating code, the node address and entire		*/
	/*	information of its contents are printed in pred_print	*/
	/*	format, and added as string comment to the instr.	*/
	/*								*/
	if ((type == TYPE_UNSET) && (p->op == P_OP_FREE))
	{
		char	tmpbuf[MAX_PRINT_BUF];

		tmptype = pred_statgetfreevartype(M, p, tmpbuf, MAX_PRINT_BUF);

		if (strlen(tmpbuf) != 0)
		{
			fatal(tmpbuf);
		}
	}
	else if ((type == TYPE_UNSET) && (p->op == P_OP_BOUND))
	{
		/*							*/
		/*	Incoming type is unset, and prednode is a 	*/
		/*	P_OP_BOUND: can't infer the type statically 	*/
		/*	(at least, not easily); Should never happen,	*/
		/*	since we check at binding parent, above in 	*/
		/*	the walk, and pass a meaningful type down 	*/
		/*	the tree.					*/
		/*							*/
		fatal(Esanity);
	}
	else
	{
		tmptype = type;
	}

	if ((tmptype != TYPE_INTCONST) && (tmptype != TYPE_REALCONST))
	{
		fatal(Ebadptreearithnodetype);
	}

	/*									*/
	/*	1. For bound variables, we want the optimizer to eventually	*/
	/*	merge together all instances in the expression. This will not 	*/
	/*	work using the current load volatile for each one. Secondly, 	*/
	/*	each instance of the same bound var name currently yields a 	*/
	/*	unique SSA name, so we need to use the bound varname rather 	*/
	/*	than the node address.						*/
	/*									*/
	/*	2. For free variables, we again want to ignore the node 	*/
	/*	address and recognize all items with the same universe number 	*/
	/*	and subdimension index to be the same.				*/
	/*									*/
	/*	3. Similarly, for parameters, we just care about the type 	*/
	/*	(I/R/W) and the register number or index.			*/
	/*									*/
	/*	One way to achieve all three above, is to, for each param/	*/
	/*	free/bound, emit only one load volatile for all identical 	*/
	/*	names / all identical unv and dimidx / all identicall ioregs 	*/
	/*	respectively, and use that single target for all occurrences.	*/
	/*									*/

	switch (p->op)
	{
		case P_OP_INTPARAM:
		case P_OP_REALPARAM:
		case P_OP_STRINGPARAM:
		{
			/*	In principle, this could still be NULL here.	*/
			if (p->paramorconstelem == NULL)
			{
				fatal(Esanity);
			}

			/*	Even for real and string, reg # is an int	*/
			n += emitparamorvar(M, p, &buf[n], buflen-n,
						pred_ptreeopstrs[p->op], NULL,
						*((int *)p->paramorconstelem->value), -1,
						tmptype);
			break;
		}

		case P_OP_BOUND:
		{
			n += emitparamorvar(M, p, &buf[n], buflen-n,
						pred_ptreeopstrs[p->op], p->varname,
						-1, -1,
						tmptype);
			break;
		}

		case P_OP_FREE:
		case P_OP_INTPARAMIDX:
		case P_OP_REALPARAMIDX:
		case P_OP_STRINGPARAMIDX:
		{
			/*										*/
			/*	All of these are indexed with an arbitrary arithexpr. We check if 	*/
			/*	the arithexpr is statically resolvable, and if so, generate nodes.	*/
			/*	NOTE: to simplify the logic, we let the treewalk still go ahead and	*/
			/*	generate code for the left subtree. We will simply ignore it when	*/
			/*	reconstructing the ptree from the optimized code.			*/
			/*										*/
			if (!pred_hasbound(p->l, NULL) && !pred_hasfree(p->l, NULL))
			{
				int		idxarithval;
				Universe 	*tmpunv;


				/*      A temporary universe needed for arithexpr evaluation    */
				tmpunv = (Universe *)calloc(1, sizeof(Universe));
				if (tmpunv == NULL)
				{
					fatal(Emalloc);
				}

				/*      Needed in awalk()       */
				tmpunv->M = M;

				idxarithval = *((integer *)pred_awalk(M, p->l, tmpunv, NULL));

				/*	    For all except P_OP_FREE, didx will be unset	*/
				n += emitparamorvar(M, p, &buf[n], buflen-n,
						pred_ptreeopstrs[p->op], NULL,
						idxarithval, p->didx,
						tmptype);
			}
			else
			{
				/*									*/
				/*	We just emit a placeholder function call. The left subtree	*/
				/*	will have code generated naturally as part of the tree walk.	*/
				/*									*/
				n += snprintf(&buf[n], buflen-n,
					"\t%%N." FLEX_PTRFMTH " = call %s @%s()\t\t\t",
					(FlexAddr)p, OPT_SALTYPE2LLVMTYPE[tmptype], pred_ptreeopstrs[p->op]);
				n += emitcomment(M, p, &buf[n], buflen-n);
			}

			break;
		}

		case P_OP_REALCONST:
		case P_OP_STRINGCONST:
		case P_OP_INTCONST:
		{
			n += snprintf(&buf[n], buflen-n, "\t%%A." FLEX_PTRFMTH " = alloca %s\t\t\t\t",
					(FlexAddr)p, OPT_SALTYPE2LLVMTYPE[tmptype]);
			n += emitcomment(M, p, &buf[n], buflen-n);

			/*										*/
			/*	NOTE: p->constelembuf[0] is a char like 'P' or 'C', and it is then 	*/
			/*	followed by the actual constant element in string form.			*/
			/*										*/
			n += snprintf(&buf[n], buflen-n,
				"\tstore %s %8s, %s* %%A." FLEX_PTRFMTH "\t\t\t",
				OPT_SALTYPE2LLVMTYPE[tmptype], &constelembuf[1],
				OPT_SALTYPE2LLVMTYPE[tmptype], (FlexAddr)p);
			n += emitcomment(M, p, &buf[n], buflen-n);

			n += snprintf(&buf[n], buflen-n,
				"\t%%N." FLEX_PTRFMTH " = load %s* %%A." FLEX_PTRFMTH "\t\t\t",
				(FlexAddr)p, OPT_SALTYPE2LLVMTYPE[tmptype], (FlexAddr)p);
			n += emitcomment(M, p, &buf[n], buflen-n);

			break;
		}

		case P_OP_ADD:
		{
			n += emitbinop(M, p, &buf[n], buflen-n, tmptype, "add", "fadd");

			break;
		}

		case P_OP_SUB:
		{
			n += emitbinop(M, p, &buf[n], buflen-n, tmptype, "sub", "fsub");

			break;
		}

		case P_OP_DIV:
		{
			n += emitbinop(M, p, &buf[n], buflen-n, tmptype, "sdiv", "fdiv");

			break;
		}

		case P_OP_MOD:
		{
			n += emitbinop(M, p, &buf[n], buflen-n, tmptype, "srem", "frem");

			break;
		}

		case P_OP_MUL:
		{
			n += emitbinop(M, p, &buf[n], buflen-n, tmptype, "mul", "fmul");

			break;
		}

		case P_OP_POW:
		case P_OP_LOG:
		{
			/*									*/
			/*	We just emit a placeholder function call. We can reconstruct 	*/
			/*	the node from the target SSA register name.			*/
			/*									*/
			n += snprintf(&buf[n], buflen-n,
				"\t%%N." FLEX_PTRFMTH " = call %s @%s()\t\t\t",
				(FlexAddr)p, OPT_SALTYPE2LLVMTYPE[tmptype], pred_ptreeopstrs[p->op]);
			n += emitcomment(M, p, &buf[n], buflen-n);

			break;
		}

		case P_OP_PRODUCT:
		case P_OP_SUM:
		{
			char	errbuf[MAX_PRINT_BUF] = "";
			Type	sumtype, indextype;


			/*									*/
			/*	Emit placeholder function call for the P_OP_SUM node itself,	*/
			/*	then emit expressions for its four constituent arithmetic	*/
			/*	expressions (fromval, toval, stepval, ofval). In rebuilding	*/
			/*	the ptree, we can search for the nodes corresponding to those	*/
			/*	children, in the LLVM code, and rebuild their subtrees.		*/
			/*									*/
			n += snprintf(&buf[n], buflen-n,
				"\t%%N." FLEX_PTRFMTH " = call %s @%s()\t\t\t",
				(FlexAddr)p, OPT_SALTYPE2LLVMTYPE[tmptype], pred_ptreeopstrs[p->op]);
			n += emitcomment(M, p, &buf[n], buflen-n);

			/*									*/
			/*	Subtree is of the form:						*/
			/*				P_OP_SUM/PROD ($$)			*/
			/*				  /  \					*/
			/*			  varintro   P_OP_SUM/PROD (t1)			*/
			/*					 /  \				*/
			/*				arithexpr   P_OP_SUM/PROD (t2)		*/
			/*					     /  \			*/
			/*				     arithexpr   P_OP_SUM/PROD (t3)	*/
			/*						   /  \			*/
			/*					  arithexpr    arithexpr	*/
			/*									*/

			indextype = pred_statgetfreevartype(M, p->l, errbuf, MAX_PRINT_BUF);
			if (strlen(errbuf) != 0)
			{
				error(errbuf);
				fatal(Esanity);
			}

			sumtype = tmptype;

			/*	From	*/
			n += optmz_pred2llvmtxtwalk(M, p->r->l, indextype, &buf[n], buflen-n);

			/*	To	*/
			n += optmz_pred2llvmtxtwalk(M, p->r->r->l, indextype, &buf[n], buflen-n);

			/*	Step	*/
			n += optmz_pred2llvmtxtwalk(M, p->r->r->r->l, indextype, &buf[n], buflen-n);

			/*	Of	*/
			n += optmz_pred2llvmtxtwalk(M, p->r->r->r->r, sumtype, &buf[n], buflen-n);


			break;
		}




		/*											*/
		/*	Boolean + Boolean -> Boolean, and arith + arith -> Boolean. Since the float 	*/
		/*	values are from ptree, we know there won't be a	NaN, so we use the 'ordered' 	*/
		/*	versions of the LLVM comparator operands.					*/
		/*											*/




		case P_OP_AND:
		{
			n += emitbinop(M, p, &buf[n], buflen-n, tmptype, "and", "and");

			break;
		}

		case P_OP_NOT:
		{
			n += snprintf(&buf[n], buflen-n,
				"\t%%N." FLEX_PTRFMTH " = xor %s %%N." FLEX_PTRFMTH ", 1\t\t",
				(FlexAddr)p, OPT_SALTYPE2LLVMTYPE[tmptype], (FlexAddr)p->l);
			n += emitcomment(M, p, &buf[n], buflen-n);


			break;
		}

		case P_OP_OR:
		{
			n += emitbinop(M, p, &buf[n], buflen-n, tmptype, "or", "or");

			break;
		}

		case P_OP_XOR:
		{
			n += emitbinop(M, p, &buf[n], buflen-n, tmptype, "xor", "xor");

			break;
		}

		case P_OP_EQ:
		{
			n += emitbinop(M, p, &buf[n], buflen-n, tmptype, "icmp eq", "fcmp oeq");

			break;
		}

		case P_OP_GE:
		{
			n += emitbinop(M, p, &buf[n], buflen-n, tmptype, "icmp sge", "fcmp oge");

			break;
		}

		case P_OP_GT:
		{
			n += emitbinop(M, p, &buf[n], buflen-n, tmptype, "icmp sgt", "fcmp ogt");

			break;
		}

		case P_OP_LE:
		{
			n += emitbinop(M, p, &buf[n], buflen-n, tmptype, "icmp sle", "fcmp ole");

			break;
		}

		case P_OP_LT:
		{
			n += emitbinop(M, p, &buf[n], buflen-n, tmptype, "icmp slt", "fcmp olt");

			break;
		}

		case P_OP_NE:
		{
			n += emitbinop(M, p, &buf[n], buflen-n, tmptype, "icmp ne", "fcmp one");

			break;
		}

		default:
		{
			fatal(Esanity);
		}
	}

	return n;
}

char *
optmz_ptree2llvmtxt(Multiverse *M, Predicate *p)
{
	Type	ptype;
	int	buflen, treesz, n = 0;
	char	*buf = NULL;


	//
	//	TODO: similar functions, should, like we now do for universe_printtypetree(),
	//	take the buffer into which to print as an argument, along with the fd,
	//	rather than allocating the buffer locally. The callers are then responsible
	//	for both allocating the buffer and deallocating it (they currently do the
	//	latter). This setup also makes it easier in the cases where we recurse
	//	(see, e.g., universe_printtypetree()).
	//

	/*	Heuristic	*/
	treesz = pred_treesz(p);
	buflen = treesz*CHUNK_PREDPRINTBUF_MULTIPLIER;

	/*									*/
	/*	 This buffer is deallocated by our caller (e.g., vm.y).		*/
	/*									*/
	buf = calloc(buflen, sizeof(char));
	if (buf == NULL)
	{
		fatal(Emalloc);
	}


	/*	Initially also use buf for this, then discard value there	*/
	if (p->op == P_OP_FREE)
	{
		ptype = pred_statgetfreevartype(M, p, buf, buflen);
		if (strlen(buf) != 0)
		{
			error(buf);
			ptype = TYPE_UNSET;
		}
	}
	else if (p->op == P_OP_BOUND)
	{
		ptype = TYPE_UNSET;
	}
	else
	{
		ptype = p->type;
	}


	/*			First, emit type aliases			*/
	n += snprintf(&buf[n], buflen-n, "%%I64 = type i64\n");
	n += snprintf(&buf[n], buflen-n, "%%R64 = type double\n");
	n += snprintf(&buf[n], buflen-n, "%%PTR = type i8*\n");
	n += snprintf(&buf[n], buflen-n, "\n");

	/*		Emit entire tree as metadata nodes			*/
	n += emitptreeasmetadatawalk(M, p, &buf[n], buflen-n);
	n += snprintf(&buf[n], buflen-n, "\n");

	/*				Emit preamble				*/
	n += snprintf(&buf[n], buflen-n, 
		"define %s @salarithexpr() nounwind %s\n",
		OPT_SALTYPE2LLVMTYPE[ptype],
		((	!pred_hasbound(p, NULL) &&
			!pred_hasfree(p, NULL) &&
			!pred_hasparam(p)) ? "readnone" : "readonly"));
	n += snprintf(&buf[n], buflen-n, "{\n");


	/*				Emit body				*/
	n += optmz_pred2llvmtxtwalk(M, p, ptype, &buf[n], buflen-n);


	/*			Emit "ret" and closing brace			*/
	n += snprintf(&buf[n], buflen-n,
		"\n\tret %s %%N." FLEX_PTRFMTH "\t\t\t\t\t",
		OPT_SALTYPE2LLVMTYPE[ptype], (FlexAddr)p);
	n += emitcomment(M, p, &buf[n], buflen-n);
	n += snprintf(&buf[n], buflen-n, "}\n\n");
	USED(n);

//BUG: should handle this more elegantly / regrow buffer
	if (buflen < 0)
	{
		fatal(Esanity);
	}

	destroyplist();

	return buf;
}

Predicate *
optmz_llvmtxt2ptree(Multiverse *M, char *txt)
{
	return NULL;
}

char *
optmz_llvmopt(Multiverse *M, char *fragment)
{
	/*								*/
	/*	Feeds the supplied source through LLVM 'opt' tool	*/	
	/*								*/

	return NULL;
}



Predicate *
optmz_ptreespecialize(Multiverse *M, Predicate *p, Universe *u)
{
	/*								*/
	/*	Takes a predicate and universe, adds nodes to the	*/
	/*	predicate to indicate constraints on the range of	*/
	/*	the universe, then calls optz_ptree() to optimize	*/
	/*	this specialized form of predicate. This routine	*/
	/*	can be called whenever a predicate is paired to a	*/
	/*	particular universe. Must however be careful about	*/
	/*	semantics if the universe in questions contains		*/
	/*	IOREG values, which cannot be statically resolved.	*/
	/*								*/
	/*	Idea: we can take this further, and in the pass		*/
	/*	over a universe to evaluate a set, we first start 	*/
	/*	with a ptree specialized for the whole universe,	*/
	/*	then start up a queue of JITs in parallel, to 		*/
	/*	specialize the ptree to subsections of the 		*/
	/*	universe. The challenge will be to find the right	*/
	/*	granularity (or, also, adaptively refine this		*/
	/*	granularity).						*/
	/*								*/

	return NULL;
}

Universe *
optmz_preduniverse(Multiverse *M, Predicate *p, Universe *u)
{
	/*								*/
	/*	Given a predicate, chops up the universe to exclude	*/
	/*	discrete ranges that could never be valid for the	*/
	/*	predicate, and returns the potentially-smaller 		*/
	/*	universe.						*/
	/*								*/

	return NULL;
}



Predicate *
optmz_ptree(Multiverse *M, Predicate *p)
{
	/*								*/
	/*	Call optmz_arith, optmz_quantifier, optmz_ctrlval, 	*/
	/*	optmz_horner, as necessary.				*/
	/*								*/
	return NULL;
}

Predicate *
optmz_arithexpr(Multiverse *M, Predicate *p)
{
	/*								*/
	/*	Arithmetic expressions will be subtrees with either	*/
	/*	an lprecarith2arithop, hprecarith2arithop at their	*/
	/*	root. So, perform a postorder walk (so we can infer	*/
	/*	types, starting from leaves, propagating upwards)	*/
	/*	to tree, passing arithmetic subtrees through LLVM 	*/
	/*	optimizers.						*/
	/*								*/

	return NULL;	
}

Predicate *
optmz_quantifierexpr(Multiverse *M, Predicate *p)
{
	/*								*/
	/*	Emit code stream to loop over all values defined by 	*/
	/*	universe, setting an alloca'd address to '1' when 	*/
	/*	it is true. We will have to convert the final opt	*/
	/*	loop back into a Sal ptree. The aim is that, e.g., 	*/
	/*	a forall which can be shown to be statically false 	*/
	/*	will be simplified as such. This could be applied 	*/
	/*	either before or after our own quantifier optmzns.	*/
	/*								*/
	switch (p->op)
	{
		case P_OP_FORALL:
		{
			break;
		}

		case P_OP_EXISTS:
		{
			break;
		}

		default:
		{
			fatal(Esanity);
		}
	}

	return NULL;
}

void
optmz_ctrlvalevalorder(Multiverse *M, Predicate *p)
{
	/*								*/
	/*	Uses the fact that during evaluation of AND and OR	*/
	/*	subtrees, the left child is checked first to see if	*/
	/*	it is a controlling value.				*/
	/*								*/
}

void
optmz_horner(Multiverse *M, Predicate *p)
{
	/*								*/
	/*	NOTE: this particular routine might be redundant if	*/
	/*	we are using LLVM for optimizing arithmetic exprs.	*/
	/*								*/
	/*	We can use Horners rule to optimize the evaluation 	*/
	/*	of predicate subtrees with arithmetic values that 	*/
	/*	represent polynomials.					*/
	/*								*/
	/*	Horners rule ensures that, e.g., in ax^4 + bx^3 + x,	*/
	/*	we are able to re-use the computed x^3 in the 		*/
	/*	computation of x^4.					*/
	/*								*/
	/*	Apply whenever we have a term in pred tree with 	*/
	/*	exponentiation to an integer power, or multiplication	*/
	/*	of a variable with itself.				*/
	/*								*/
}
