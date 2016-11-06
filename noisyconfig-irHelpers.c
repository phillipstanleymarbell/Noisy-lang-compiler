#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "noisyconfig-errors.h"
#include "version.h"
#include "noisyconfig.h"



NoisyConfigIrNode *
genNoisyConfigIrNode(
    NoisyConfigState *  N, 
    NoisyConfigIrNodeType type, 
    NoisyConfigIrNode *  irLeftChild, 
    NoisyConfigIrNode *  irRightChild, 
    NoisyConfigSourceInfo *  sourceInfo
) {

	NoisyConfigIrNode *		node;

	node = (NoisyConfigIrNode *) calloc(1, sizeof(NoisyConfigIrNode));
	if (node == NULL)
	{
		noisyConfigFatal(N, Emalloc);
		/*	Not reached	*/
	}

	node->type		= type;
	node->sourceInfo	= sourceInfo;
	node->irLeftChild	= irLeftChild;
	node->irRightChild	= irRightChild;

	if (irLeftChild != NULL)
	{
		irLeftChild->irParent = node;
	}

	if (irRightChild != NULL)
	{
		irRightChild->irParent = node;
	}

	return node;
}
