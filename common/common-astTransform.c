/*
    Authored 2018. Zhengyang Gu.

    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    *   Redistributions of source code must retain the above
        copyright notice, this list of conditions and the following
        disclaimer.

    *   Redistributions in binary form must reproduce the above
        copyright notice, this list of conditions and the following
        disclaimer in the documentation and/or other materials
        provided with the distribution.

    *   Neither the name of the author nor the names of its
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

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <stdint.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "common-errors.h"
#include "common-timeStamps.h"
#include "data-structures.h"
#include "common-lexers-helpers.h"
#include "common-irPass-helpers.h"
#include "common-astTransform.h"
#include "common-irHelpers.h"

/*
 * In the comments below, trees are written as (ROOT LEFT_CHILD RiGHT_CHILD)
 * Node names in UPPER CASE are usually auxiliary nodes the nature of which
 * can be ignored for this purpose. Node names in 'single quotes' are the "exact"
 * content of the node ("exact" as in kNewtonIrNodeType_Tminus will be written as
 * '-'). Node names in camelCase are meant to represent the type of the node.
 */


IrNode *  binaryOpTreeTransform(State *  N, IrNode *  inputAST)
{
    /*
     * In a Newton AST, a binary operator tree has the following structure
     * (ROOT leftOperand (X_SEQ Operator rightOperand)), where ROOT could be
     * any auxiliary type, leftOperand is a tree with the root node showing
     * its type (expression or term). rightOperand could be eithor another 
     * binary expression tree with X_SEQ as the root or an X_SEQ node with
     * a single expression/term tree as its child.
     */
    IrNodeType opType = L(R(inputAST))->type;
    IrNode *  leftOperand = commonTreeTransform(N, L(inputAST));
    IrNode *  rightOperand = NULL;
    /*
     * This tests if the right operand is the last item in a long chain of
     * addition/multiplication, and creates the right operand node accordingly.
     */
    if (R(R(R(inputAST))) == NULL)
    {
        rightOperand = commonTreeTransform(N, L(R(R(inputAST))));
    }
    else
    {
        rightOperand = binaryOpTreeTransform(N, R(R(inputAST)));
    }
    return genIrNode(N, opType, leftOperand, rightOperand, inputAST->sourceInfo);
}

IrNode *  expandMinus(State *  N, IrNode *  rootNode)
{
    /*
     * With the current parser, a quantity term of -1 will be represented as
     * (TERM '-' (X_SEQ '1' NILL)), where TERM is the quantity_term auxiliary
     * node, if more items follows the -1, they will be attached to where the 
     * NILL node currently is.
     * 
     * As a result, it "breaks" the normal structure of binary operation tree,
     * where L(R(root)) is the operator (it would be '1' in the -1 case). It is
     * therefore necessary to shrink it to a single node, in the format of 
     * ('-' NILL '1')so that we can use binaryOpTreeTransform.
     * 
     * NB: the number is stored in the right child since it is more-or-less 0-1
     */
    IrNode *  expandedTree = genIrNode(N, kNewtonIrNodeType_Tminus, NULL, \
                                       commonTreeTransform(N, L(R(rootNode))),\
                                       L(rootNode)->sourceInfo);

    irPassHelperColorIr(N, expandedTree, kNoisyIrNodeColorTreeTransformedColoring, true, false);
    return genIrNode(N, rootNode->type, expandedTree, R(R(rootNode)), rootNode->sourceInfo);
}

IrNode *  exponentTreeTransform(State *  N, IrNode *  baseNode)
{
    /*
     * We could use the original "baseNode" as our base, although it will also
     * inherent its children, so it is probably better to make a copy of it.
     * Since it should be (hopefully) clear enough from the token type, we can
     * just copy both tokenString and value attributes to the new node.
     */
    IrNode *  base = genIrNode(N, baseNode->type, NULL, NULL, baseNode->sourceInfo);
    base->tokenString = baseNode->tokenString;
    base->value = baseNode->value;

    IrNode *  exponent = commonTreeTransform(N, L(R(baseNode)));

    return genIrNode(N, kNewtonIrNodeType_Texponent, base, exponent, baseNode->sourceInfo);
}


IrNode *  commonTreeTransform(State *  N, IrNode *  inputAST)
{
    if (inputAST->nodeColor & kNoisyIrNodeColorTreeTransformedColoring)
    {
        return inputAST;
    }
    switch(inputAST->type)
    {
        case kNewtonIrNodeType_Pconstraint:
        {
            return binaryOpTreeTransform(N, inputAST);
        }
        case kNewtonIrNodeType_PquantityExpression:
        {
            /*
             * When it is a single quantity term
             */
            if (R(inputAST) == NULL)
            {
                return commonTreeTransform(N, L(inputAST));
            }
            /*
             * When it is a sum of quantity expressions
             */
            else if (R(inputAST)->type == kNoisyIrNodeType_Xseq)
            {
                return binaryOpTreeTransform(N, inputAST);
            }
            else
            {
                fatal(N, "Unrecognized Expression AST Structure!");
            }
            break;
        }
        case kNewtonIrNodeType_PquantityTerm:
        {
            if (R(inputAST) == NULL)
            {
                return commonTreeTransform(N, L(inputAST));
            }
            else if (L(inputAST)->type == kNewtonIrNodeType_Tminus &&
                     !(L(inputAST)->nodeColor & kNoisyIrNodeColorTreeTransformedColoring))
            {
                return commonTreeTransform(N, expandMinus(N, inputAST));
            }
            else if (R(inputAST)->type == kNoisyIrNodeType_Xseq)
            {
                return binaryOpTreeTransform(N, inputAST);
            }
            else
            {
                fatal(N, "Unrecognized Term AST Structure!");
            }
            break;
        }
        case kNewtonIrNodeType_Tidentifier:
        case kNewtonIrNodeType_Tnumber:
        {
            /*
             * Depends on the type of the base, an exponent tree in Newton may take
             * the format of (base (AUX '**' NILL) (X_SEQ exponent NILL))
             */
            if (L(inputAST) == NULL)
            {
                return inputAST;
            }
            else if (L(inputAST)->type == kNewtonIrNodeType_PhighPrecedenceBinaryOp)
            {
                return exponentTreeTransform(N, inputAST);
            }
        }
        default:
        {
            fatal(N, "Unknown AST Root Node!");
        }
    }
}
