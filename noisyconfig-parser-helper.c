#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <stdint.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "noisyconfig.h"
#include "noisyconfig-errors.h"
#include "version.h"
#include "noisyconfig-lexer.h"
#include "noisyconfig-parser.h"
#include "noisyconfig-firstAndFollow.h"
#include "noisyconfig-parser-helper.h"
#include "noisyconfig-parser-errors.h"
#include "noisyconfig-symbolTable.h"
#include "noisyconfig-irHelpers.h"
#include "noisyconfig-parser-helper.h"

extern char *		gNoisyConfigAstNodeStrings[];
extern int		gNoisyConfigFirsts[kNoisyConfigIrNodeTypeMax][kNoisyConfigIrNodeTypeMax];

extern void		noisyConfigFatal(NoisyConfigState *  N, const char *  msg);
extern void		noisyConfigError(NoisyConfigState *  N, const char *  msg);

/*
 *	Static local functions
 */

void
errorUseBeforeDefinition(NoisyConfigState *  N, const char *  identifier)
{
	flexprint(N->Fe, N->Fm, N->Fperr, "Saw identifier \"%s\" in use before definition\n", identifier);
}

void
errorMultiDefinition(NoisyConfigState *  N, NoisyConfigSymbol *  symbol)
{
	flexprint(N->Fe, N->Fm, N->Fperr, "errorMultiDefinition \n");

}


bool
peekCheck(NoisyConfigState *  N, int lookAhead, NoisyConfigIrNodeType expectedType)
{
	if (noisyConfigLexPeek(N, lookAhead) == NULL)
	{
		return false;
	}

	return (noisyConfigLexPeek(N, lookAhead)->type == expectedType);
}


NoisyConfigIrNode *
depthFirstWalk(NoisyConfigState *  N, NoisyConfigIrNode *  node)
{
	if (node->irLeftChild == NULL || node->irRightChild == NULL)
	{
		return node;
	}

	return depthFirstWalk(N, node->irRightChild);
}

void
addLeaf(NoisyConfigState *  N, NoisyConfigIrNode *  parent, NoisyConfigIrNode *  newNode, NoisyConfigScope * currentScope)
{
	NoisyConfigIrNode *	node = depthFirstWalk(N, parent);

	if (node == NULL)
	{
		noisyConfigFatal(N, Esanity);
	}
	
    newNode->currentScope = currentScope;
    newNode->irParent = parent;
	
    if (node->irLeftChild == NULL)
	{
		node->irLeftChild = newNode;
		
		return;
	}

	node->irRightChild = newNode;
}

void
addLeafWithChainingSeq(NoisyConfigState *  N, NoisyConfigIrNode *  parent, NoisyConfigIrNode *  newNode, NoisyConfigScope * currentScope)
{
	NoisyConfigIrNode *	node = depthFirstWalk(N, parent);
    newNode->currentScope = currentScope;
    newNode->irParent = parent;

	if (node->irLeftChild == NULL)
	{
		node->irLeftChild = newNode;

		return;
	}
	
	node->irRightChild = genNoisyConfigIrNode(N,	kNoisyConfigIrNodeType_Xseq,
						newNode /* left child */,
						NULL /* right child */,
						noisyConfigLexPeek(N, 1)->sourceInfo /* source info */);

    node->irRightChild->currentScope = currentScope;
}

void
addToConfigFileTypeScopes(NoisyConfigState *  N, char *  identifier, NoisyConfigScope *  configFileTypeScope)
{
	configFileTypeScope->identifier = identifier;

	if (N->progtypeScopes == NULL)
	{
		N->progtypeScopes = configFileTypeScope;

		return;
	}

	NoisyConfigScope *	p = N->progtypeScopes;
	while (p->next != NULL)
	{
		p = p->next;
	}
	p->next = configFileTypeScope;

	return;
}



/*
 *	kNoisyConfigIrNodeType_PidentifierList
 *
 *	AST subtree:
 *
 *		node		= kNoisyConfigIrNodeType_Tidentifier
 *		node.left	= kNoisyConfigIrNodeType_Tidentifier
 *		node.right	= Xseq of kNoisyConfigIrNodeType_Tidentifier
 */
void
assignTypes(NoisyConfigState *  N, NoisyConfigIrNode *  node, NoisyConfigIrNode *  typeExpression)
{
	/*
	 *	TODO: The typeExpr might be, say, an identifier that is an
	 *	alias for a type. We should check for this case and get the
	 *	identifier's sym->typeTree. Also, do sanity checks to validate
	 *	the typeTree, e.g., make sure it is always made up of basic
	 *	types and also that it's not NULL.
	 */

	if (node->type != kNoisyConfigIrNodeType_Tidentifier)
	{
		noisyConfigFatal(N, EassignTypeSanity);
	}

	/*
	 *	Walk subtree identifierList, set each node->symbol.typeExpr = typeExpr
	 */
	node->symbol->typeTree = typeExpression;

	/*
	 *	Might be only one ident, or only two, or a whole Xseq of them
	 */
	if (node->irLeftChild != NULL)
	{
		node->irLeftChild->symbol->typeTree = typeExpression;
	}

	node = node->irRightChild;

	while (node != NULL)
	{
		/*
		 *	In here, node->type is always Xseq, with node->irLeftChild a node,
		 *	and node->irRightChild either another Xseq or NULL.
		 */
		if (node->irLeftChild != NULL)
		{
			node->irLeftChild->symbol->typeTree = typeExpression;
		}

		node = node->irRightChild;
	}
}

void
noisyConfigParserSyntaxError(
	NoisyConfigState *  N, 
	NoisyConfigIrNodeType currentlyParsingTokenOrProduction, 
    NoisyConfigIrNodeType expectedProductionOrToken
) {
    int     seen = 0;


    //errors++;

    /*
     *  TODO: Other places where we need the string form of a NoisyIrNodeType
     *  should also use gNoisyAstNodeStrings[] like we do here, rather than
     *  gProductionStrings[] and gTerminalStrings[].
     */
    flexprint(N->Fe, N->Fm, N->Fperr, "\n-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - --\n");
    flexprint(N->Fe, N->Fm, N->Fperr, "\n\t%s, %s line %d position %d, %s \"",
                    EsyntaxA,
                    noisyConfigLexPeek(N, 1)->sourceInfo->fileName,
                    noisyConfigLexPeek(N, 1)->sourceInfo->lineNumber,
                    noisyConfigLexPeek(N, 1)->sourceInfo->columnNumber,
                    EsyntaxD);
    noisyConfigLexPrintToken(N, noisyConfigLexPeek(N, 1));
    flexprint(N->Fe, N->Fm, N->Fperr, "\" %s %s.\n\n\t%s", EsyntaxB, gNoisyConfigAstNodeStrings[currentlyParsingTokenOrProduction], EsyntaxC);

    if (((expectedProductionOrToken > kNoisyConfigIrNodeTypeMax) && (expectedProductionOrToken < kNoisyConfigIrNodeTypeMax)) || (expectedProductionOrToken == kNoisyConfigIrNodeTypeMax))
    {
        flexprint(N->Fe, N->Fm, N->Fperr, " one of:\n\n\t\t");
        for (int i = 0; i < kNoisyConfigIrNodeTypeMax && gNoisyConfigFirsts[currentlyParsingTokenOrProduction][i] != kNoisyConfigIrNodeTypeMax; i++)
        {
            if (seen > 0)
            {
                flexprint(N->Fe, N->Fm, N->Fperr, ",\n\t\t");
            }

            flexprint(N->Fe, N->Fm, N->Fperr, "'%s'", gNoisyConfigAstNodeStrings[gNoisyConfigFirsts[currentlyParsingTokenOrProduction][i]]);
            seen++;
        }
    }
    else if ((currentlyParsingTokenOrProduction == kNoisyConfigIrNodeTypeMax) && (expectedProductionOrToken < kNoisyConfigIrNodeTypeMax))
    {
        flexprint(N->Fe, N->Fm, N->Fperr, ":\n\n\t\t");
        flexprint(N->Fe, N->Fm, N->Fperr, "'%s'", gNoisyConfigAstNodeStrings[expectedProductionOrToken]);
    }
    else
    {
        noisyConfigFatal(N, Esanity);
    }

    flexprint(N->Fe, N->Fm, N->Fperr, ".\n\n\tInstead, saw:\n\n");
    noisyConfigLexPeekPrint(N, 5, 0);

    // if (N->mode & kNoisyConfigModeCGI)
    // {
    //     flexprint(N->Fe, N->Fm, N->Fperr, "%s</b>", kNoisyConfigErrorDetailHtmlTagClose);
    // }

    flexprint(N->Fe, N->Fm, N->Fperr, "\n-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - --\n\n");
}
