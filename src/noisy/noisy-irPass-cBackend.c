/*
	Authored 2015. Phillip Stanley-Marbell.
 
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

*** was codegen.b

#
#	(0)	IMPORTANT: items are added to code[idx] in reverse order, so (1) when
#		done, we have to reverse it before emitting code.
#		(2) Only call emit() with a single line of emitted code ---
#		don't put in newlines.
#
#	,(1)	At top of each scope, we should (a) scan the scope for all type name 
#		definitions (either type defn, adt defn, etc), and emit the relevant
#		structs immediately, at top of scope. (b) scan the scope for any
#		T_DAS and emit the declaration half of the DAS at top of scope
#
#	,(3)	It should be ensured that for type names (introduced by either
#		an ADT definition, a "type" alias, or a progtype-qual-type included
#		from somewhere or defined in the current progtype, will have their
#		the sym.typetree of the PTYPENAME Node pointing to a valid typetree
#		built on the basic types).  This is assumed, e.g., by mk_typeexpr2typefordefn()
#
#	. (*)	At top of each new scope, emit struct definitions for Arrays, Lists, Tuples and Sets
#
#	(4)	When emitting a List-type structure, emit an inline structure definition
#		within the list structure that defines the type of the elements (we already
#		have mk_typeexpr2typefordefn() for doing this). This internal struct has one
#		member, " *data"...
#
#	.(5)	For many of the mk_*, the sanity check should not be on n.op, but on
#		some other part of the supplied Parse node to validate it is correct.
#		e.g., for T_INT, we get P_STMT in which
#
#	.(6)	Clean up the mk_XXXdecl: their incoming node is actually a stmt node ?
#
#	(7)	Need to decide on a convention/interface for the generated C functions
#		corresponding to namegens.  What is form of their identifier ?
#		stub/basename ? etc.  We are currently using "init_<namegenident>(void)"
#
#	(8)	Figure out what we want to do with writing to a ident-or-nil that is nil
#
#	(9)	Check all the mk_* to see if they are actually used.  Don't delete the unused ones yet,
#		and don't delete this todo item either, yet.
#
#	.(10)	Make sure the typetrees of all nodes which are ident's are filled in.
#		Currently, at least idents corresponding to namegens do not seem to
#		have their typetrees filled in (fixed namegen case --- pull type out of progtypedefn symbol table).
#
#	(11)	Make sure the sym.intconst for array type has the array's length. Assumed by code that generates array
#		types.
#
#	(12)	After emitting ARRAY and SET definitions (in emit_stmt() ?), need to set
#		their length fields.  Unlike in Limbo, the length must be carried explicitly.
#
#	(*)	ADT type definition vs a var decl or definition.  Its only result is to introduce
#		a new typename. Do we handle this correctly
#
#	General strategy:
#		Arrays, lists, sets and chans are treated by default as pointers.
#		They are pointers to structs of some sort.
#

implement CodeGen;

include "sys.m";
include "draw.m";
include "string.m";
include "m.m";
include "error.m";
include "lex.m";
include "symtab.m";
include "parse.m";
include "types.m";
include "codegen.m";

sys		: Sys;
types		: Types;

#
#	Rather than pass this around.  We are always eating
#	a single progtype and its implementation
#
ptypename	: string;

#
#	Its easier to have this out here and have the different
#	routines emit into it. Elements in array are list of
#	strings representing the code for each complete namegen
#	in the file being compiled.
#
code		: array of list of string;
ngidents	: array of string;
inptype		: array of int;

cgen(root: ref Parser->Node) : array of list of string
{
	sys	= load Sys Sys->PATH;
	types	= load Types Types->PATH;

	emit_program(root);
	for (i := 0; i < len code; i++)
	{
		code[i] = reversestrlist(code[i]);
	}

	return code;
}














#
#	emit_*:
#
#	Main code generation routines.  Most (except the first 2 and a
#	few others ?) act on grammar productions. They all emit code
#	directly into the code[] array.  The auxiliary routines which
#	return strings instead of emitting directly into code[] are the
#	mk_* routines (see below).
#












emit(idx: int, line: string)
{
	code[idx] = line :: code[idx];
}


emit_dfltheaders(idx: int)
{
	#	Default C #include's.
	emit(idx, "#include \"mruntime.h\"");
}


#
#	P_NAMEGENDEFN
#
#	AST subtree:
#
#		node.left	= T_IDENTIFIER
#		node.right	= [Xseq of 2 tupletypes] P_SCOPESTMTLIST
#
#	Takes a namegendefn node and emits the registration in nametable/chantable
#
emit_initreg(n: ref Parser->Node, idx: int)
{
	#	Sanity Check
	if (n.op != P_NAMEGENDEFN)
	{
		fatal(E_SANITY+" codegen.b:emit_initreg()");
	}

	emit(idx, sys->sprint("m_register_namegen(&init_%s, \"%s\", \"%s\");",
		n.left.sym.ident,
		n.left.sym.ident,
		types->mktypesig(n.left.sym.typetree)));
}


#
#	P_PROGRAM
#
#	AST subtree:
#
#		node.left	= P_PROGTYPEDECL
#		node.right	= Xseq of P_NAMEGENDEFN
#
#	Strategy:
#		(1)	Count the number of namegens, by walking top of AST
#			use this to size and initialize the code[] and
#			similar arrays
#
#		(2)	Output common runtime headers for each namegen
#
#		(3)	Output progtype structure declarations for each namegen
#
#		(4)	Output beginning of C init() function for each namegen;
#			Register the namegen if it is declared in progtype.
#
#		(5)	Generate code for each namegen
#
#		(6)	Generate a single simulator config file to map namegens
#			to nodes, etc.
#
emit_program(root: ref Parser->Node)
{
	#	Sanity Check
	if (root.op != P_PROGRAM)
	{
		fatal(E_SANITY+" codegen.b:emit_program()");
	}

	kids	:= get_imm_childs(root);

	numgens	:= get_namegencnt(root);
	code	= array [numgens] of list of string;
	inptype	= array [numgens] of {* => 0};
	set_ngidents(root);

	ptypedecl := hd kids;
	kids = tl kids;

	#	Give each output file its preamble
	for (i := 0; i < numgens; i++)
	{
		#	Default runtime system headers included inline
		emit_dfltheaders(i);

		#	Root.left is progtypedecl, Root.right is seq. of namegens
		emit_progtypedecl(ptypedecl, i);
	}

	#	Walk along top of tree, pick off the namegen roots and eat 'em
	for (i = 0; kids != nil; i++)
	{
		emit_namegendefn(hd kids, i);
		kids = tl kids;
	}
}


#
#	P_PROGTYPEDECL
#
#	AST subtree:
#
#		node.left	= T_IDENTIFIER
#		node.right	= P_PROGTYPEBODY
#
#	Strategy:
#		(1)	Set global progtype name
#
#		(2)	Handle the inner productions
#
emit_progtypedecl(root: ref Parser->Node, idx: int)
{
	#	Sanity Check
	if (root.op != P_PROGTYPEDECL)
	{
		fatal(E_SANITY+" codegen.b:emit_progtypedecl()");
	}

	#	Set the global progtype name
	ptypename = root.left.sym.ident;

	emit_progtypebody(root.right, idx);
}

#
#	P_PROGTYPEBODY
#
#	AST subtree:
#
#		node.left	= P_PTYPENAMEDECL
#		node.right	= Xseq of P_PTYPENAMEDECL
#
#	Strategy:
#		(*)	Rather than previous idea of putting them in a structure, emit
#			them as global defs for each output file.  This is partly because
#			some things are awkward to put in a structure definition, like
#			typedefs, and such.
#
#		(1)	Emits a series of type and structure definitions based on the
#			progtype definition. Define constants, structs for new types
#
#		(*)	Standard struct for bounded arrays, strings, fixedpoint, etc.,
#			should not be emitted here, but rather included in minclude.h
#
#		(2)	namegendecl's are handled separately
#		
emit_progtypebody(n: ref Parser->Node, idx: int)
{
	#	Sanity Check
	if (n.op != P_PROGTYPEBODY)
	{
		fatal(E_SANITY+" codegen.b:emit_progtypebody()");
	}

	kids := get_imm_childs(n);
	while (kids != nil)
	{
		emit_ptypenamedecl(hd kids, idx);
		kids = tl kids;
	}
}


#
#	P_PTYPENAMEDECL
#
#	AST subtree:
#
#		node.left	= P_IDENTLIST
#		node.right	= P_CONDECL | P_TYPEDECL | P_NAMEGENDECL
#
#	Strategy:
#
#		(0)	Convert node.left into a list of string
#
#		(1)	For each item on ident list, do:
#			-	Depending on type of node.right, emit
#				either a C const defn, a typedef, or a
#				function prototype (for namegens).
#			-	Yes, a function prototype. This function will
#				then be registered with the local nametable.
#
#		(2)	Maybe not here, but we'll need a way to take a namegendecl
#			and represent its interface in a form that is stored in
#			nametable : just convert its typetree into a canonical
#			linearized form ?
#
emit_ptypenamedecl(n: ref Parser->Node, idx: int)
{
	#	Sanity Check
	if (n.op != P_PTYPENAMEDECL)
	{
		fatal(E_SANITY+" codegen.b:emit_ptypenamedecl()");
	}

	#	Get the identlist as a list of strings
	idlist := get_identstrlist(n.left);

	#	For each member of the identlist, emit type, then emit member
	while (idlist != nil)
	{
		ident := hd idlist;

		#	n.right is typedecl/condecl/namegendecl
		case (n.right.op)
		{
			P_TYPEDECL	=>
			{
				c1 := mk_typedecl(n.right);
				emit(idx, c1 + " " + ident + ";");
			}

			P_CONDECL	=>
			{
				(c1, c2) := mk_condecl(n.right);
				emit(idx, c1 + " " + ident + " = " + c2 + ";");
			}

			P_NAMEGENDECL	=>
			{
				#
				#	Mark this namegen in the inptype array to cause
				#	output of code to register namegen in name table for
				#	those namegens in progtype interface definition
				#
				inptype[get_ngident2idx(ident)] = 1;
			}

			*	=> fatal(E_SANITY+" codegen.b:emit_ptypenamedecl()");
		}

		idlist = tl idlist;
	}
}



#
#	P_NAMEGENDEFN
#
#	AST subtree:
#
#		node.left	= T_IDENTIFIER
#		node.right	= [Xseq of 2 tupletypes] P_SCOPESTMTLIST
#
#	Strategy:
#		(0)	Emit function return type, name (based on node.left), brace open
#
#		(1)	Loop over the Xseq of P_SCOPESTMTLIST  and emit them
#
#		(2)	Emit brace close
#
#		(-1)	Preamble function to run the main body on a stack
#			provided as argument ? --- no. The nametable hooks
#			should be the ones who setup a stack for us and 
#			the preamble function should be a generic assembler
#			routine that the nametable calls with a stack addr
#			and a namegen C func, and the preamble sets up the stack
#			and jumps to the C function.
#
emit_namegendefn(n: ref Parser->Node, idx: int)
{
	#	Sanity Check
	if (n.op != P_NAMEGENDEFN)
	{
		fatal(E_SANITY+" codegen.b:emit_namegendefn()");
	}

	kids := get_imm_childs(n);
	if (len kids != 4 && len kids != 2)
	{
		fatal(E_SANITY+" codegen.b:emit_namegendefn()");
	}

	ngident := (hd kids).sym.ident;
	
	#	TODO: if there's 4 kids, then this namegendefn includes
	#	type.  It might be a namegen that is not declared in the
	#	progtype.  We'll need to take care of that.
	if (len kids == 4)
	{
		#	Eat up the ident node and the two tupletypes
		kids = tl tl tl kids;
	}
	else
	{
		kids = tl kids;
	}

	emit(idx, "void");
	emit(idx, "init_"+ngident+"(void)");
	emit(idx, "{");

	if (inptype[get_ngident2idx(ngident)])
	{
		emit_initreg(n, idx);
	}

	emit_scopedstmtlist(hd kids, idx);

	emit(idx, "}");
}


#
#	P_SCOPEDSTMTLIST
#
#	AST subtree:
#
#		node.left	= P_STMTLIST
#		node.right	= nil
#
#	Strategy:
#		(*)	NOTE: scoped statement list is ... just a stament list in a new scope
#
#		(0)	Walk subtree to find all channels defined in current (not deeper) scope
#
#		(1)	Insert statement to create entries in chan table for all
#			channels that are defined in the current scope
#
#		(*)	Walk subtree to find all anonaggrtyes (nodes exist in tree as array, etc).
#			emit the C struct deifnitions to introfuce new struct types.
#
#		(*)	Walk subtree to find all T_DAS, and explicitly declare lvalue at top of scope
#
#		(*)	Walk subtree to find all declarations (including CONDECL|...|TYPEDECL) and 
#			emit them at top of scope.
#
#		(2)	Handle the inner productions
#
#		(3)	Insert statements to remove entries from
#			chan table
#
emit_scopedstmtlist(n: ref Parser->Node, idx: int)
{
	#	Sanity Check
	if (n.op != P_SCOPEDSTMTLIST)
	{
		fatal(E_SANITY+" codegen.b:emit_scopedstmtlist()");
	}

	#	These will appear again in the decls harvest (below)

#	2009: is this a vestige from when we planned to have channel types
#	separate from their use? we currently can only create channels via a
#	name2chan. We cannot independently declare or define a channel.
#
#	chandecls := get_stmts(n, P_CHANTYPE);
#	emit_decls(chandecls, idx);

	#	ADT type name introductions
	adtnames := get_stmts(n, T_ADT);
	emit_decls(adtnames, idx);

	#	Subsequently, we'll treat T_DAS as T_AS
	dasdecls := get_stmts(n, T_DAS);
	emit_decls(dasdecls, idx);

	#	NOTE: the ':' does not appear in the AST 
	decls := get_stmts(n, P_CONDECL);
	decls = joinnodelist(decls, get_stmts(n, P_TYPEDECL));
		#	P_TYPEEXPRS:
		decls = joinnodelist(decls, get_stmts(n, T_BOOL));
		decls = joinnodelist(decls, get_stmts(n, T_NYBBLE));
		decls = joinnodelist(decls, get_stmts(n, T_BYTE));
		decls = joinnodelist(decls, get_stmts(n, T_INT));
		decls = joinnodelist(decls, get_stmts(n, T_REAL));
		decls = joinnodelist(decls, get_stmts(n, T_FIXED));
		decls = joinnodelist(decls, get_stmts(n, T_STRING));
		decls = joinnodelist(decls, get_stmts(n, P_ARRAYTYPE));
		decls = joinnodelist(decls, get_stmts(n, P_LISTTYPE));
		decls = joinnodelist(decls, get_stmts(n, P_SETTYPE));
		decls = joinnodelist(decls, get_stmts(n, P_TUPLETYPE));
		decls = joinnodelist(decls, get_stmts(n, P_TYPENAME));
	emit_decls(decls, idx);


	#	Now, go generate code for the statements
	emit_stmtlist(n.left, idx);
}

emit_decls(decls: list of ref Parser->Node, idx: int)
{
	#	Each item on the decls list is going to be either
	#	'identlist typeexpr' or 'identlist das expr'
	while (decls != nil)
	{
		kids := get_imm_childs(hd decls);

		if (	len kids == 2 &&
			(hd kids).op == T_IDENTIFIER &&
			(hd tl kids).op == P_TYPEEXPR)
		{
			mk_field_or_vardecl(hd kids, hd tl kids);
		}
		else if (len kids == 3 &&
			(hd kids).op == T_IDENTIFIER &&
			(hd tl kids).op == T_DAS &&
			(hd tl tl kids).op == P_TYPEEXPR)
		{
			mk_field_or_vardecl(hd kids, hd tl tl kids);
		}
		else
		{
			fatal(E_SANITY + "");
		}

		decls = tl decls;
	}
}

#
#	P_STMTLIST
#
#	AST subtree:
#
#		node.left	= P_STMT or nil
#		node.right	= Xseq of P_STMT
#
#	Strategy:
#		(*)	Loop over inner productions
#
emit_stmtlist(n: ref Parser->Node, idx: int)
{
	#	Sanity Check
	if (n.op != P_STMTLIST)
	{
		fatal(E_SANITY+" codegen.b:emit_stmtlist()");
	}

	kids := get_imm_childs(n);
	while (kids != nil)
	{
		emit_stmt(hd kids, idx);
		kids = tl kids;
	}
}


#
#	P_STMT
#
#	AST subtree:
#
#		node.left	= nil | P_IDENTORNILLIST
#		node.right	= nil | (P_CONDECL | .. | P_TYPEXPR)---the colon is implicit
#					| P_ASSIGNOP (in tree directly as T_AS|... etc) P_EXPR
#
#	Strategy:
#		(0)	If its a das, the declaration has already been emitted
#			when scope was entered.
#
#		(*)	All declarations have been pulled up and emitted at top of scope
#
#		(1)	If T_CHANWRITE, handle specially, else, identical to C
#
#		(2)	If the stmt contains a array/list/adt-CASTEXPR:
#			(a)	Typecheck P_INITLIST.  Ensure all items on list have same
#				type, and assign that as base type of the list.
#
#			(b)	If it is a T_DAS, delcare variables on the lhs to have the
#				generated list type
#
#			(c)	For each of the variables on the lhs, emit a C expressions that 
#				evaluates to a pointer to list struct
#
#		(*)	BUG: how does emit_stmt deal w/ ADT decls (already emitted at top of scope)
#
#		(2)	Likely handle all inner productions inline here,
#			since most inner productions in stmt depend on conext
#
emit_stmt(n: ref Parser->Node, idx: int)
{
	#	Sanity Check
	if (n.op != P_STMT)
	{
		fatal(E_SANITY+" codegen.b:emit_stmt()");
	}

	kids := get_imm_childs(n);

	#	NULL statement will have no kids.  Emit ";" and done
	if (len kids == 0)
	{
		emit(idx, ";");

		return;
	}

	#	Unlike in other languages (Limbo, C), we cannot have expressions
	#	where statemnts are required. To achieve similar effect explicitly
	#	requires 'nil = expr'.
	if (len kids < 2 || len kids > 3)
	{
		fatal(E_SANITY+" codegen.b:emit_stmt()");
	}


	#	Case 1: identlist condecl|...|typedecl  --- these have already
	#	been taken care of when scope was entered.


	#	Case 2: identlist assignop expr
	ids	:= get_identstrlist(hd kids);
	assignop:= hd tl kids;
	expr	:= hd tl tl kids;

	while (ids != nil)
	{
		if (assignop.op == T_CHANWRITE)
		{
			emit(idx, sys->sprint("m_chanwrite(%s, %s);",
					hd ids, mk_expr(expr)));
		}
		else
		{
			emit(idx, hd ids +" "+C_ASSIGNOPMAP[assignop.op]+" "+mk_expr(expr));
		}
		ids = tl ids;
	}
}


#
#	P_ASSIGNOP: should not exist in AST
#
#	AST subtree:
#
#		node		= T_AS | ... | T_DAS
#		node.left	= nil
#		node.right	= nil
#


#
#	P_MATCHSTMT
#
#	AST subtree:
#
#		node		= T_MATCH | T_MATCHSEQ
#		node.left	= P_GUARDBODY
#		node.right	= nil
#
#	Strategy:
#		(1)	For a T_MATCHSEQ, emit an equivalent switch statement
#
#		(2)	For a T_MATCH, emit several "if" statements.
#			We could technically emit a new namegen for each guarded
#			block and run them in parallel
#
emit_matchstmt(n: ref Parser->Node, idx: int)
{
	#	Sanity Check
	if (n.op != T_MATCH && n.op != T_MATCHSEQ)
	{
		fatal(E_SANITY+" codegen.b:emit_matchstmt()");
	}

	#	The P_GUARDBODY expands into a sequence of <P_EXPR; P_STMT>
	glist := get_imm_childs(n.left);	
	if (len glist & 1)
	{
		fatal(E_SANITY+" codegen.b:emit_matchstmt()");
	}

	#	Though we could parallelize, for now, match becomes sequence of ifs (_not_ a switch())
	if (n.op == T_MATCH)
	{
		x_ifseq("", "", glist, idx);
		return;
	}

	#	Matchseq becomes ifs-else chain
	if (n.op == T_MATCHSEQ)
	{
		x_ifelseseq("", glist, idx);
		return;
	}
}

x_ifelseseq(flagset: string, glist: list of ref Parser->Node, idx: int)
{
	if (glist != nil)
	{
		emit(idx, sys->sprint("if (%s)", mk_expr(hd glist)));
		emit(idx, "{");
		emit_stmt(hd tl glist, idx);
		emit(idx, flagset);
		emit(idx, "}");

		glist = tl tl glist;
	}

	x_ifseq(flagset, "else ", glist, idx);
}

x_ifseq(flagset, preif: string, glist: list of ref Parser->Node, idx: int)
{
	while (glist != nil)
	{
		emit(idx, sys->sprint("%sif (%s)", preif, mk_expr(hd glist)));
		emit(idx, "{");
		emit_stmt(hd tl glist, idx);
		emit(idx, flagset);
		emit(idx, "}");

		glist = tl tl glist;
	}
}

#
#	P_ITERSTMT
#
#	AST subtree:
#
#		node.left	= T_ITER
#		node.right	= P_GUARDBODY
#
#	Strategy:
#		(1)	Emit an equivalent for() statement
#
emit_iterstmt(n: ref Parser->Node, idx: int)
{
	#	Sanity Check
	if (n.op != P_ITERSTMT)
	{
		fatal(E_SANITY+" codegen.b:emit_iterstmt()");
	}

	#	The P_GUARDBODY expands into a sequence of <P_EXPR; P_STMT>
	glist := get_imm_childs(n.right);	
	if (len glist & 1)
	{
		fatal(E_SANITY+" codegen.b:emit_matchstmt()");
	}

	tmpname := get_tmpname(n.sym.scope);

	#	We put the emitted C code in its own scope to ease definition of tmpname
	emit(idx, "{");
	emit(idx, sys->sprint("int %s;", tmpname));
	emit(idx, sys->sprint("while(%s)", tmpname));
	emit(idx, "{");
	x_ifseq(sys->sprint("%s = 1", tmpname), "",
		glist, idx);
	emit(idx, "}");
	emit(idx, "}");
}


#
#	P_GUARDBODY
#
#	AST subtree:
#
#		node.left	= P_EXPR
#		node.right	= Xseq of P_STMT and P_EXPR alternating
#
#	Strategy:
#		(*)	Implicit in parent (MATCH/MATCHSEQ)
#
















#
#	mk_*: auxiliary code generation routines
#

















#	P_CONDECL
#
#	Generated AST subtree:
#
#		node		= T_INTCONST | T_REALCONST | BOOLCONST
#		node.left	= nil
#		node.right	= nil
#
mk_condecl(n: ref Parser->Node) : (string, string)
{
	c1, c2 : string;

	case (n.op)
	{
		T_INTCONST	=>
		{
			c1 = "const int";
			c2 = sys->sprint("%d", n.sym.intconst);
		}

		T_REALCONST	=>
		{
			#	BUG: there is a correct way to print reals so you retain
			#	their accuracy.
			c1 = "const double";
			c2 = sys->sprint("%f", n.sym.realconst);
		}

		T_BOOLCONST	=>
		{
			c1 = "const char";
			c2 = sys->sprint("%d", n.sym.intconst);
		}

		*	=>	fatal(E_SANITY+" codegen.b:get_condecl()");	
	}

	return (c1, c2);
}



#
#	P_FIELDSELECT
#
#	AST subtree:
#
#		node		= T_DOT or T_LBRAC (nothing else)
#		node.left	= T_IDENTIFIER | P_EXPR_low
#		node.right	= nil or P_EXPR_high
#
#	Strategy:
#		(*)	Emit equivalent C statement for indexing / field selection
#
#		(1)	If node is DOT, then node.left must be identifier, emit
#			"->identname", since we implement ADTs as pointers to structs.
#
#		(2)	If node if BRAC, 
#			(a) simple case: node.right == nil. This is not a slice.
#				emit "->data[expr]" since arrays are implemented as
#				a struct with a "data" and a "len" field
#			(b) slice case: handled by parent
#		
#
mk_fieldselect(n: ref Parser->Node) : string
{
	case (n.op)
	{
		T_DOT	=>	;
		T_LBRAC	=>	;

		*	=>	fatal(E_SANITY+" codegen.b:mk_fieldselect()");
	}

	return "fixme";
}

#
#	P_FACTOR
#
#	AST subtree:
#
#		node		= T_IDENTIFIER | ... | P_EXPR
#		node.left	= nil or P_FIELDSELECT (only if node = ident)
#		node.right	= nil or Xseq of P_FIELDSELECT (only if node = ident)
#
#	Strategy:
#		(1)	If not identifier, equivalent C lang construct
#
#		(2)	If identifier,
#			(a) scan subtree to see if it contains a slice.
#				if it does, emit a new temp var for the whole
#				sequence of fieldselects up to the first slice
#				If it contains multiple slices, we should be able
#				to emit a single temp var that represents the
#				final subslice ?
#
#				(i) Emit a new array struct _instance_, named
#				"x", where x is some unique temp name
#				(ii) Inefficient, but will work:
#				emit a "memcpy(x->data, expr_high - expr_low);"
#				emit "x->len = expr_high - expr_low"
#
#			(b) emit identifier name
#			(c) loop over remaning children and emit fieldselects
#
mk_factor(n: ref Parser->Node) : string
{
	case (n.op)
	{
		T_IDENTIFIER	=> return mk_fieldselect_train(n);

		T_INTCONST	=> return sys->sprint("%d", n.sym.intconst);
		T_REALCONST	=> return sys->sprint("%f", n.sym.realconst);
		T_STRCONST	=> return sys->sprint("%s", n.sym.strconst);
		T_BOOLCONST	=> return sys->sprint("%d", n.sym.intconst);

		P_EXPR		=> return mk_expr(n);

		*		=>	fatal(E_SANITY+" codegen.b:mk_factor()");
	}

	return nil;
}

#
#		node		= T_IDENTIFIER
#		node.left	= nil or P_FIELDSELECT
#		node.right	= nil or Xseq of P_FIELDSELECT
#
mk_fieldselect_train(n: ref Parser->Node) : string
{
	out	: string;

	#	Get all children, then parent.  Ensures right order
	nodes := get_imm_childs(n);
	nodes = n :: nodes;

	if (nodes != nil)
	{
		out = (hd nodes).sym.ident;
	}

	while (nodes != nil)
	{
		out += ".";
		out += (hd nodes).sym.ident;
		nodes = tl nodes;
	}

	return out;
}


#
#	P_TERM
#
#	AST subtree:
#
#		node.left	= P_FACTOR
#		node.right	= Xseq of P_HPRECBINOP  and P_FACTOR
#
#	Strategy:
#		(1)	Loop over emission of children
#
mk_term(n: ref Parser->Node) : string
{
	out	: string;

	#	Sanity Check
	if (n.op != P_TERM)
	{
		fatal(E_SANITY+" codegen.b:mk_term()");
	}

	#	If there is a T_CONS, we need to pass to emit calls to list builder
	if (scan_tree(n, T_CONS) != nil)
	{
		return mk_listbuilder(n);
	}

	kids := get_imm_childs(n);
	while (kids != nil)
	{
		case ((hd kids).op)
		{
		P_FACTOR	=>	out += mk_factor(hd kids);

		#	P_HPRECBINOP is one of... :
		T_ASTERISK	=>	out += C_OPMAP[T_ASTERISK];
		T_DIV		=>	out += C_OPMAP[T_DIV];
		T_PERCENT	=>	out += C_OPMAP[T_PERCENT];
		T_CARET		=>	out += C_OPMAP[T_CARET];
		T_CONS		=>	fatal(E_MKTERMTCONS);
		}

		kids = tl kids;
	}

	return out;
}


#
#	P_TERM
#
#	AST subtree:
#
#		node.left	= P_FACTOR
#		node.right	= Xseq of P_HPRECBINOP  and P_FACTOR
#
mk_listbuilder(n: ref Parser->Node) : string
{
	#	Here, the P_HPRECBINOP is  a T_CONS.  Emit a valid C term
	#	that yields the cons'd list...
	return nil;
}


#	P_TYPEDECL
#
#	Generated AST subtree:
#
#		node.left	= T_TYPE | P_ADTTYPEDECL
#		node.right	= P_TYPEEXPR | nil
#
mk_typedecl(n: ref Parser->Node) : string
{
	#	Sanity Check
	if (n.op != P_TYPEDECL)
	{
		fatal(E_SANITY+" codegen.b:mk_typedecl()");
	}

	if (n.left.op == T_TYPE && n.right.op == P_TYPEEXPR)
	{
		return "typedef " + mk_typeexpr2struct(n.right);
	}
	else if (n.left.op == P_ADTTYPEDECL && n.right == nil)
	{
		return mk_adttypedecl(n.left);
	}
	else
	{
		fatal(E_SANITY+" codegen.b:mk_typedecl()");
	}

	return nil;
}


mk_identlist(n: ref Parser->Node) : string
{
	#	Sanity Check
	if (n.op != T_IDENTIFIER)
	{
		fatal(E_SANITY+" codegen.b:mk_identlist()");
	}

	ids := get_identstrlist(n);
	if (ids == nil)
	{
		fatal(E_SANITY+" codegen.b:identlist2flatstr()");
	}

	out := hd ids;
	ids = tl ids;

	while (ids != nil)
	{
		out += ", " + hd ids;
		ids = tl ids;
	}

	return out;
}

mk_field_or_vardecl(idlistnode: ref Parser->Node, typeexpr: ref Parser->Node) : string
{
	#
	#	Reuseable routine that emits bodies for different basic types
	#	as well as structured types.
	#
	out := mk_typeexpr2struct(typeexpr);

	idlist := get_imm_childs(idlistnode);
	if (idlist == nil)
	{
		fatal(E_SANITY+" idlist, codegen.b:mk_field_or_vardecl()");
	}

	out += (hd idlist).sym.ident;
	idlist = tl idlist;

	while (idlist != nil)
	{
		out += ", " + (hd idlist).sym.ident;
		idlist = tl idlist;
	}
	out += ";";

	return out;
}

#
#	P_ADTTYPEDECL
#
#	AST subtree:
#
#		node.left	= P_IDENTLIST
#		node.right	= Xseq of texpr + zero or more P_IDENTLIST+texpr
#
#	Strategy:
#		(0)	First, emit a structure beginning corresponding to
#			the ADT body
#
#		(1)	Flatten children to get sequence of <identlist, tyepexpr> pairs
#
#		(2)	For each field definition in the ADT body:
#			- 	Call mk_field_or_vardecl(), which takes an identlist
#			  	and a typeexpr, and emits (a) a structure defn for the typeexpr,
#				defining all members of the identlist to have type of struct.
#
#		(3)	Emit "}", then list of all node.left, then ";"
#
mk_adttypedecl(n: ref Parser->Node) : string
{
	#	Sanity Check
	if (n.op != P_ADTTYPEDECL)
	{
		fatal(E_SANITY+" codegen.b:emit_adttypedecl()");
	}

	kids := get_imm_childs(n);
	if (len kids & 1)
	{
		#	Odd number of kids. Badly formed AST
		fatal(E_SANITY+" codegen.b:emit_adttypedecl()@get_imm_childs()");
	}

	out := "typedef struct";
	out += "{";
	while (kids != nil)
	{
		out += mk_typeexpr2struct(hd tl kids)+ "\t" +
				mk_identlist(hd kids)+";";
		
		kids = tl tl kids;
	}
	out += "} ";

	return out;
}


#	P_TYPEEXPR
#
#	AST subtree:
#
#		node		= P_BASICTYPE | P_ANONAGGRTYPE | P_TYPENAME
#		node.left	= P_TOLERANCE | nil
#		node.right	= Xseq of P_TOLERANCE | nil
#
mk_typeexpr2struct(typeexpr: ref Parser->Node) : string
{
	#
	#	Emits a preamble to be used in a C language variable definition
	#	that will be followed by a list of C identifiers (variables), in a
	#	variable deifnition, or structure field definition.
	#	E.g., in the trivial case of typeexpr representing type "byte",
	#	we emit the string "unsigned char ". In the case of, say typeexpr
	#	representing type "string", we emit "struct {uchar *data; int len} "
	#	etc.
	#
	#	Strategy:
	#
	#	If the typeexpr is one that maps directly to a basic C type,
	#	then the preamvle is, e.g., "int ".
	#
	#	O/w, emits a possibly nested set of struct definitions, which
	#	define new types M lang types that don't match C types
	#	directly.
	#
	#	Preamble is "struct tempname { ... } ", where there may be additional
	#	fields nested at the "..."
	#
	#	The typeexpr production from the grammar:
	#
	#	     typeexpr ::= (basictype [tolerance {"," tolerance}]) | anonaggrtype | typename .
	#
	#

	case (typeexpr.op)
	{
		T_BOOL		=>	return C_TYPEMAP[T_BOOL]	+ "\t";
		T_NYBBLE	=>	return C_TYPEMAP[T_NYBBLE]	+ "\t";
		T_BYTE		=>	return C_TYPEMAP[T_BYTE]	+ "\t";
		T_INT		=>	return C_TYPEMAP[T_INT]		+ "\t";
		T_REAL		=>	return C_TYPEMAP[T_REAL]	+ "\t";
	
		P_FIXEDTYPE	=>
		{
			#
			#	Whole and fractional nbits are in left and right
			#	subtrees of the T_FIXED
			#
			w := typeexpr.left.sym.intconst;
			f := typeexpr.right.sym.intconst;

			#	'w' and 'f': whole and fractional
			return sys->sprint("struct {unsigned w:%d; unsigned f:%d;}\t", w, f);
		}

		T_STRING	=>	return C_TYPEMAP[T_STRING]	+ "\t";
	




#	TODO: need to generate multi-dimensional C array based on the P_ARRAYTYPE typetree

		#
		#	P_ARRAYTYPE
		#
		#	AST subtree:
		#
		#		node.left	= T_INTCONST
		#		node.right	= Xseq of zero or more T_INTCONST then a P_TYPEEXPR
		#
		#	Strategy:
		#		(*)	We are called the moment a new scope is entered, to emit
		#			C structs a the top of the scope.
		#
		#		(*)	Arrays, tuples and lists are defined as structs
		#
		#		(*)	Use the list of INTCONSTs and the size of typeexpr to calculate
		#			the size of the struct's data array (there is an mk_* routine
		#			to do this)
		#
		#		(*)	Emit code to initialize the structs 'len' field
		#
		#		(*)	Emit code for initializing struct from initlist
		#

		#	Array type includes length.  The sym.intconst should be the type
		P_ARRAYTYPE	=>	return sys->sprint("struct {Generic data[%d]; int len;}\t",
							typeexpr.sym.intconst);






		#
		#	P_LISTTYPE
		#
		#	AST subtree:
		#
		#		node.left	= P_TYPEEXPR
		#		node.right	= nil
		#
		#	Strategy:
		#		(*)	We are called the moment a new scope is entered, to emit
		#			C structs a the top of the scope.
		#
		#		(*)	Arrays, tuples and lists are defined as structs
		#
		P_LISTTYPE	=>	return C_TYPEMAP[P_LISTTYPE]	+ "\t";


		#
		#	P_TUPLETYPE
		#
		#	AST subtree:
		#
		#		node.left	= P_TYPEEXPR
		#		node.right	= Xseq of P_TYPEEXPR
		#
		#	Strategy:
		#		(*)	We are called the moment a new scope is entered, to emit
		#			C structs a the top of the scope.
		#
		#		(*)	Arrays, tuples and lists are defined as structs
		#

		P_TUPLETYPE	=>
		{
			out := sys->sprint("struct Tuple_%s {", types->mktypesig(typeexpr));

			#
			#	Children of the P_TUPLETYPE node are the tuple entries
			#	--- recurse on contents of tuple structure
			#
			t := get_imm_childs(typeexpr);
			while (t != nil)
			{
				out += mk_typeexpr2struct(hd t);
				t = tl t;
			}

			out += "}\t";

			return out;
		}

	
		#
		#	P_SETTYPE
		#
		#	AST subtree:
		#
		#		node.left	= T_INTCONST
		#		node.right	= P_TYPEEXPR
		#
		#	Strategy:
		#		(*)	We are called the moment a new scope is entered, to emit
		#			C structs a the top of the scope.
		#
		#		(*)	Arrays, tuples and lists are defined as structs
		#

		#	We don't yet implement optimisation of small boolean sets as bits in word.
		#2009: for now sets are implemented identical to arrays. We will eventually migrate to libsets
		P_SETTYPE	=>
		{
			#	Set type includes length.  The sym.intconst should be the type
			return sys->sprint("struct {Generic data[%d]; int len;}\t",
							typeexpr.sym.intconst);
		}


		#
		#	If it is a typename, the type name was either (1) introduced via an ADT defn
		#	(2) introduced by a type alias (3) a progtype-qualified-type from a progtype
		#	No matter, its sym.typetree should hold its type tree.
		#
		P_TYPENAME	=>
		{
			return mk_typeexpr2struct(typeexpr.sym.typetree);
		}


		#
		#	This will not be reached directly since there are no variables in
		#	an ADT type delcaration.  It wil hoever be reached indirectly when
		#	we recurse on a typename, and get here via typename_typeexpr.sym.typeexpr
		#	These are in AST as a list of one or more <identlist, typeexpr> pairs.
		#
		P_ADTTYPEDECL	=>
		{
			#	ADT type structure uniquely defines name
			#	(rather than calling getnewname/gettmpname)
			out := sys->sprint("struct ADT_%s {", types->mktypesig(typeexpr));

			#	Get all children and eat them in pairs.
			t := get_imm_childs(typeexpr);

			#	Should be a list of a nonzero m'ple of 2 items
			if (len t < 2 || ((len t)&1) != 0)
			{
				fatal(E_SANITY+
					"codegen.b:mk_typeexpr2struct()@get_imm_childs()");
			}

			while (t != nil)
			{
				idnode := hd t;
				typexnode := hd tl t;
				
				#	for each <identlist, typeexpr> pair, emit
				#	the struct base don typeexpr, and define the
				#	idents in list to have its type.
				out += mk_typeexpr2struct(typexnode);
				
				idlist := get_imm_childs(idnode);
				if (idlist == nil)
				{
				}

				out += (hd idlist).sym.ident;
				idlist = tl idlist;
				while (idlist != nil)
				{
					out += ","+(hd idlist).sym.ident;
					idlist = tl idlist;
				}
				out += ";";

				t = tl t;
				t = tl t;
			}

			out += "} ";

			return out;
		}
	}

	fatal(E_SANITY+ " codegen.b:mk_typeexpr2struct()");

	return nil;
}


#
#	P_IDENTORNILLIST
#
#	AST subtree:
#
#		node.left	= P_IDENTORNIL
#		node.right	= Xseq of P_IDENTORNIL
#
#	Strategy:
#		(*)	Implicit in parent productions
#
#		(*)	Used only in stmt might be best to 
#			determine the wrangling in
#			stmt where we have more context.
#
mk_identornillist(n: ref Parser->Node) : string
{
	#	Sanity Check
	if (n.op != P_IDENTORNILLIST)
	{
		fatal(E_SANITY+" codegen.b:emit_identornillist()");
	}

	return "fixme";
}


#
#	P_TYPENAME	(type name use, i.e., either ident, or [ident"->"ident])
#			Note that we are talking about progtype-qualified-types,
#			which are typenames.  These are the only progtype-qualified
#			entities.  There are no progtype-qualified-variables.
#
#			BUG:	The grammar currently does not permit
#				progtype-qualified-consts, thought that
#				is something we'd like to be able to have.
#
#	AST subtree:
#
#		node.left	= P_IDENTIFIER
#		node.right	= nil | T_IDENTIFIER (not an Xseq chain)
#
#	Note: T_PROGTYPEQUAL is the "->"
#
#	Strategy:
#		(*)	These are type names, so must be handled by a parent doing
#			something like a var definition.
#
#		Parent does things along the lines of:
#		(*)	If node.right is nil, then just emit equivalent C definition
#
#		(*)	If node.right isn't nil:
#			(a)	Use node.left to find the scope of external progtype
#				definition included in us
#			(b)	Within that scope, determine the type structure of
#				node.right
#			(c)	Emit equivalent C constructs
#
mk_typename(n: ref Parser->Node) : string
{
	#	Sanity Check
	if (n.op != P_TYPENAME)
	{
		fatal(E_SANITY+" codegen.b:emit_typename()");
	}

	return "fixme";
}



#
#	P_TOLERANCE: should never exist in AST
#
#	AST subtree:
#
#		node		= P_ERRORMAGTOL | P_LOSSTOL | P_LATENCYTOL
#		node.left	= nil
#		node.right	= nil
#
#	Strategy (for P_ERRORMAGTOL, P_LOSSTOL, P_LATENCYTOL):
#
#		*	Add tolerance to list for variable, in symbol table 
#	
#		*	Latency and loss tolerance are only menaingful
#			when applied to channels. Application to ordinary
#			vars is currently allowed, and the compiler should
#			eventually propagate these through the dataflow graph
#			to derive tolerances at channels.  Need to clean up 
#			the semantics (or rather, define it formally to start!!
#
#		*	Errormagtolerances may mean that the variable needs to
#			be represented with more or fewer bits, and the interpretation
#			of those bits changed.
#
#		*	There should be a pass that, given the compile-time
#			ft() distribution, walks through the AST, and determines
#			all the unique encodings that are necessary. For each of these,
#			it should generate a function that takes values in the
#			original type and encodes them, likewise for decode.
#
#		*	There's no way i'll have time to implement this is first
#			version of compiler, so leave it out: all tolerances are
#			ignored.
#


#
#	P_ERRORMAGTOLERANCE
#
#	AST subtree:
#
#		node.left	= T_REALCONST
#		node.right	= T_REALCONST
#
#	Strategy:
#		(*)	Implicit in parent productions
#
mk_errormagtolerance(n: ref Parser->Node) : string
{
	#	Sanity Check
	if (n.op != P_ERRORMAGTOLERANCE)
	{
		fatal(E_SANITY+" codegen.b:emit_errormagtolerance()");
	}

	return "fixme";
}


#
#	P_LOSSTOLERANCE
#
#	AST subtree:
#
#		node.left	= T_REALCONST
#		node.right	= T_REALCONST
#
#	Strategy:
#		(*)	Implicit in parent productions
#
mk_losstolerance(n: ref Parser->Node) : string
{
	#	Sanity Check
	if (n.op != P_LOSSTOLERANCE)
	{
		fatal(E_SANITY+" codegen.b:emit_losstolerance()");
	}

	return "";
}


#
#	P_LATENCYTOLERANCE
#
#	AST subtree:
#
#		node.left	= T_REALCONST
#		node.right	= T_REALCONST
#
#	Strategy
#		(*)	Implicit in parent productions
#
mk_latencytolerance(n: ref Parser->Node) : string
{
	#	Sanity Check
	if (n.op != P_LATENCYTOLERANCE)
	{
		fatal(E_SANITY+" codegen.b:emit_latencytolerance()");
	}

	return "";
}
















#
#	P_INITLIST
#
#	AST subtree:
#
#		node.left	= P_EXPR
#		node.right	= Xseq of P_EXPR
#
#	Strategy:
#		(*)	Use a "for" loop to perform init. Emitted by parent production
#
mk_initlist(n: ref Parser->Node) : string
{
	#	Sanity Check
	if (n.op != P_INITLIST)
	{
		fatal(E_SANITY+" codegen.b:emit_initlist()");
	}

	return "fixme";
}


#
#	P_IDXINITLIST
#
#	AST subtree:
#
#		node.left	= P_ELEMENT
#		node.right	= Xseq of P_ELEMENT
#
#	Strategy:
#		(*)	Use a "for" loop to perform init. Emitted by parent production
#
mk_idxinitlist(n: ref Parser->Node) : string
{
	#	Sanity Check
	if (n.op != P_IDXINITLIST)
	{
		fatal(E_SANITY+" codegen.b:emit_idxinitlist()");
	}

	return "fixme";
}


#
#	P_STARINITLIST
#
#	AST subtree:
#
#		node.left	= P_ELEMENT
#		node.right	= Xseq of P_ELEMENT, zero or more "*"+P_ELEMENT
#
#	Strategy:
#		(*)	Use a "for" loop to perform init. Emitted by parent production
#
mk_starinitlist(n: ref Parser->Node) : string
{
	#	Sanity Check
	if (n.op != P_STARINITLIST)
	{
		fatal(E_SANITY+" codegen.b:emit_starinitlist()");
	}

	return "fixme";
}


#
#	P_ELEMENT
#
#	AST subtree:
#
#		node.left	= P_EXPR
#		node.right	= nil | P_EXPR
#
#	Strategy:
#		(*)	Implicit in parent productions
#
mk_element(n: ref Parser->Node) : string
{
	#	Sanity Check
	if (n.op != P_ELEMENT)
	{
		fatal(E_SANITY+" codegen.b:emit_element()");
	}

	return "fixme";
}



#
#	P_EXPR
#
#	AST subtree:
#
#		node		= P_TERM | P_ANONAGGRCASTEXPR | ... | P_NAME2CHANEXPR
#		node.left	= P_LPRECBINOP
#		node.right	= Xseq of P_LPRECBINOP, P_TERM
#
#	Strategy:
#		(1)	Emit equivalent C expressions
#
mk_expr(n: ref Parser->Node) : string
{
	#	Sanity Check
	if (n.op != P_EXPR)
	{
		fatal(E_SANITY+" codegen.b:emit_expr()");
	}

	return "fixme";
}


#
#	P_LISTCASTEXPR
#
#	AST subtree:
#
#		node.left	= P_INITLIST
#		node.right	= nil
#
#	Strategy:
#		(*)	Handled in parent
#
mk_listcastexpr(n: ref Parser->Node) : string
{
	#	Sanity Check
	if (n.op != P_LISTCASTEXPR)
	{
		fatal(E_SANITY+" codegen.b:emit_listcastexpr()");
	}

	return "fixme";
}


#
#	P_SETCASTEXPR
#
#	AST subtree:
#
#		node.left	= P_INITLIST
#		node.right	= nil
#
#	Strategy:
#		(*)	Handled in parent
#
mk_setcastexpr(n: ref Parser->Node) : string
{
	#	Sanity Check
	if (n.op != P_SETCASTEXPR)
	{
		fatal(E_SANITY+" codegen.b:emit_setcastexpr()");
	}

	return "fixme";
}


#
#	P_ARRCASTEXPR
#
#	AST subtree:
#
#		node.left	= P_IDXINITLIST | T_INTCONST
#		node.right	= nil | P_STARINITLIST
#
#	Strategy:
#		(*)	Handled in parent
#
mk_arrcastexpr(n: ref Parser->Node) : string
{
	#	Sanity Check
	if (n.op != P_ARRCASTEXPR)
	{
		fatal(E_SANITY+" codegen.b:emit_arrcastexpr()");
	}

	return "fixme";
}


#
#	P_ANONAGGRCASTEXPR: should not exist in AST
#
#	AST subtree:
#
#		node		= P_LISTCAST | P_SETCAST | P_ARRAYCAST
#		node.left	= nil
#		node.right	= nil
#
#	Strategy:
#		(*)	Handle the inner productions
#


#
#	P_CHANEVTEXPR
#
#	AST subtree:
#
#		node.left	= T_ERASURES | T_ERRORS | T_LATENCY
#		node.right	= Xseq of T_IDENTIFIER, cmpop, expr
#
#	Strategy:
#		(*)	Handled in parent
#
mk_chanevtexpr(n: ref Parser->Node) : string
{
	#	Sanity Check
	if (n.op != P_CHANEVTEXPR)
	{
		fatal(E_SANITY+" codegen.b:mk_chanevtexpr()");
	}

	return "fixme";

#	Emit call to runtime library function which counts number of
#	errors, etc., e.g. result = m_errcnt().  Need to have a unique numbering
#	of channels, and to pass in that number to runtime lib call
#	That number should be in the symbol table. Then, emit
#
#	if (errs/era/lat cmpop expr) { stmt }
}


#
#	P_NAME2CHANEXPR
#
#	AST subtree:
#
#		node.left	= P_TYPEEXPR
#		node.right	= Xseq of P_EXPR, REALCONST
#
#	Strategy:
#		(1)	Emit a call to m_name2chan(), which takes as argument a string (name)
#			and returns a pointer to a Chan structure
#
mk_name2chanexpr(n: ref Parser->Node) : string
{
	#	Sanity Check
	if (n.op != P_NAME2CHANEXPR)
	{
		fatal(E_SANITY+" codegen.b:mk_name2chanexpr()");
	}

	return "fixme";
}

#
#	P_VAR2NAMEEXPR
#
#	AST subtree:
#
#		node.left	= P_FACTOR
#		node.right	= T_STRCONST | nil
#
#	Strategy:
#		(1)	Emit a call to m_var2name() causing an entry to be created in the chan table
#
mk_var2nameexpr(n: ref Parser->Node) : string
{
	#	Sanity Check
	if (n.op != P_VAR2NAMEEXPR)
	{
		fatal(E_SANITY+" codegen.b:mk_var2nameexpr()");
	}

	return "fixme";
}


#
#	P_CHAN2NAMEEXPR
#
#	AST subtree:
#
#		node.left	= P_FACTOR
#		node.right	= T_STRCONST or nil
#
#	Strategy:
#
mk_chan2nameexpr(n: ref Parser->Node) : string
{
	#	Sanity Check
	if (n.op != P_CHAN2NAMEEXPR)
	{
		fatal(E_SANITY+" codegen.b:mk_chan2nameexpr()");
	}

	return "fixme";

#	(1)	Each identifier with type chan in the symbol table should have a unique
#		ID by which it is referred to, in, say, the chan table manipulations
#
#	(2)	Emit a call to m_chan2name() with the uniqque ID of channel
#		as well as node.right as argument. The return value of that call
#		should be a string which we return.  Call causes an entry to be created
#		in the runtime chan table
}
















#
#	get_*
#

















get_tmpname(nil: ref SymbolTable->Scope) : string
{
	#	Creates a temporary name that is unique to the current scope
	#	must keep track of the names created so we don't create duplicates.
	#	(needed, e.g. to create a temp struct to implement array slices)

	return "fixme";
}

get_scopedepth(nil: ref Parser->Node) : int
{
	#	useful for things like pretty-printing the generated
	#	C code to increase tab stops with scope depth.

	return -1;
}

get_ngident2idx(ident: string) : int
{
	#	Given an identifier for a namegen, find its index in the code[] array

	for (i := 0; i < len ngidents; i++)
	{
		if (ngidents[i] == ident) return i;
	}

	fatal(E_SANITY+" codegen.b:get_ngident2idx()");
	return -1;
}

get_namegencnt(root: ref Parser->Node) : int
{
	r := root.right;
	for (n := 0; r != nil;)
	{
		if (r.left.op != P_NAMEGENDEFN)
		{
			fatal(E_SANITY+" codegen.b:namegencnt()");
		}
		r = r.right;
		n++;
	}

	return n;
}


#
#	P_IDENTLIST
#
#	AST subtree:
#
#		node		= T_IDENTIFIER
#		node.left	= T_IDENTIFIER
#		node.right	= Xseq of T_IDENTIFIER
#
#	Strategy:
#		(*)	Implicit in parent productions
#
get_identstrlist(n: ref Parser->Node) : list of string
{
	out	: list of string;

	#	Sanity Check
	if (n.op != T_IDENTIFIER)
	{
		fatal(E_SANITY+" codegen.b:get_identlist()");
	}

	while (n != nil)
	{
		out = n.sym.ident :: out;
		n = n.right;
	}

	return out;
}


get_arraydim(nil: ref Parser->Node) : (int, int)
{
	#	n is the right subtree of an array decl/defn
	#	walk the subtree and determine
	#	(1)	The number of elements in the array
	#	(2)	The size of elements
	#	(*)	return (nelem, nelem*elemsize)

	return (0, 0);
}



#
#	P_SCOPEDSTMTLIST
#
#	AST subtree:
#
#		node.left	= P_STMTLIST
#		node.right	= nil
#
get_stmts(n: ref Parser->Node, nodefilter: int) : list of ref Parser->Node
{
	chandefs	: list of ref Parser->Node;

	#	Sanity Check
	if (n.op != P_SCOPEDSTMTLIST)
	{
		fatal(E_SANITY+" codegen.b:get_chandefs()");
	}

	kids := get_imm_childs(n.left);
	while (kids != nil)
	{
		x := hd kids;

		if (scan_tree(x, nodefilter) != nil)
		{
			chandefs = x :: chandefs;
		}

		kids = tl kids;
	}
	
	return chandefs;
}

















#
#	Misc
#














scan_tree(n: ref Parser->Node, ntype: int) : ref Parser->Node
{
	#	Search a tree for a node of type ntype
#...
	return nil;
}



set_ngidents(root: ref Parser->Node)
{
	ngidents = array [get_namegencnt(root)] of string;

	r := root.right;
	for (n := 0; r != nil; )
	{
		if (r.left.op != P_NAMEGENDEFN)
		{
			fatal(E_SANITY+" codegen.b:namegencnt()");
		}
		ngidents[n++] = r.left.left.sym.ident;
		r = r.right;
	}
}


#	Return a list of non-Xseq nodes rooted at an Xseq
inorderseqwalk(parent: ref Parser->Node) : list of ref Parser->Node
{
	if (parent == nil)
	{
		return nil;
	}

	if (parent.left == nil && parent.right == nil)
	{
		#	Should never have a lone Xseq with no children
		if (parent.op == Xseq)
		{
			fatal(E_BADXSEQNODE+" parse.b:inorderseqwalk()");
		}

		return parent :: nil;
	}

	#	Items at level of the Xseq sequence will be hanging off node.right
	return parent.left :: inorderseqwalk(parent.right);
}

joinscopelist(l, r: list of ref SymbolTable->Scope): list of ref SymbolTable->Scope
{
	l = reversescopelist(l);
	while (l != nil)
	{
		r = hd l :: r;
		l = tl l;
	}

	return r;
}

reversescopelist(in: list of ref SymbolTable->Scope) : list of ref SymbolTable->Scope
{
	out	: list of ref SymbolTable->Scope;

	while (in != nil)
	{
		out = (hd in) :: out;
		in = tl in;
	}
	return out;
}

joinstrlist(l, r: list of string) : list of string
{
	l = reversestrlist(l);
	while (l != nil)
	{
		r = (hd l) :: r;
		l = tl l;
	}
	return r;
}

reversestrlist(in: list of string) : list of string
{
	out	: list of string;

	while (in != nil)
	{
		out = (hd in) :: out;
		in = tl in;
	}
	return out;
}

joinnodelist(l, r: list of ref Parser->Node) : list of ref Parser->Node
{
	l = reversenodelist(l);
	while (l != nil)
	{
		r = (hd l) :: r;
		l = tl l;
	}

	return r;
}

reversenodelist(in: list of ref Parser->Node) : list of ref Parser->Node
{
	out	: list of ref Parser->Node;

	while (in != nil)
	{
		out = (hd in) :: out;
		in = tl in;
	}
	return out;
}

#
#	(a)	If the node.right is an Xseq, then it is a >2 child node
#		that has been rendered as a binary tree using Xseqs.
#		Flatten out the left node + all the nodes hanging of the
#		Xseqs and return as a list. List is ordered with leftmost
#		child at head, rightmost child at tail.
#
#	(b)	If node.right isn't an Xseq, then it is a a true binary
#		node.  Return the list of (1 or 2) children. The list is
#		ordered with left child at head, right child at tail.
#
get_imm_childs(parent: ref Parser->Node) : list of ref Parser->Node
{
	flat 	: list of ref Parser->Node;

	if (parent.left == nil && parent.right == nil)
	{
		return nil;
	}

	#
	#	Assumption is that we've only put Xseqs in node.right, to chain
	#	together >1 children which wouldn't fit after filling node.left
	#
	if (parent.left.op == Xseq)
	{
		fatal(E_ILLXSEQNODE+" parse.b:get_imm_childs()");
	}

	if (parent.right != nil && parent.right.op != Xseq)
	{
		#	Put them on s.t. head of final list has node.left
		flat = parent.right :: flat;
		flat = parent.left :: flat;

		return flat;
	}

	return (parent.left :: inorderseqwalk(parent.right));
}




fatal(s: string)
{
	raise sys->sprint("%s\n", s);
}
