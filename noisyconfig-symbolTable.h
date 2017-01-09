#include <string.h>


Dimension * noisyConfigDimensionTableAddDimensionForToken(NoisyState *  N, NoisyScope *  scope, struct NoisyToken *  token);
Dimension * noisyConfigDimensionTableDimensionForIdentifier(NoisyState *  N, NoisyScope *  scope, const char *  identifier);

Physics * noisyConfigPhysicsTableAddPhysicsForToken(NoisyState *  N, NoisyScope *  scope, struct NoisyToken *  token);
Physics * noisyConfigPhysicsTablePhysicsForIdentifier(NoisyState *  N, NoisyScope *  scope, const char *  identifier);

Physics* copyPhysicsNode(Physics* list);
Physics* getTailPhysics(Physics* list);

void appendIntegralList(IntegralList* head, IntegralList* list);
IntegralList* getTailIntegralList(IntegralList* list);

int countNumberTime(Dimension* head);

void noisyConfigPhysicsAddNumeratorDimension(NoisyState * N, Physics * physics, Dimension * numerator); 
void noisyConfigPhysicsAddDenominatorDimension(NoisyState * N, Physics * physics, Dimension * denominator);

void noisyConfigPhysicsCopyNumeratorDimensions(NoisyState * N, Physics * dest, Physics * source);
void noisyConfigPhysicsCopyDenominatorDimensions(NoisyState * N, Physics * dest, Physics * source);
void noisyConfigPhysicsCopyNumeratorToDenominatorDimensions(NoisyState * N, Physics * dest, Physics * source);
void noisyConfigPhysicsCopyDenominatorToNumeratorDimensions(NoisyState * N, Physics * dest, Physics * source);

NoisyScope *	noisyConfigSymbolTableAllocScope(NoisyState *  N);
NoisySymbol *	noisyConfigSymbolTableAddOrLookupSymbolForToken(NoisyState *  N, struct NoisyScope *  scope, struct NoisyToken *  token);
NoisySymbol *	noisyConfigSymbolTableSymbolForIdentifier(NoisyState *  N, struct NoisyScope *  scope, const char *  identifier);
NoisyScope *	noisyConfigSymbolTableOpenScope(NoisyState *  N, struct NoisyScope *  scope, struct NoisyIrNode *  subtree);
void		        noisyConfigSymbolTableCloseScope(NoisyState *  N, struct NoisyScope *  scope, struct NoisyIrNode *  subtree);
