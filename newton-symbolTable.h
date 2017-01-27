#include <string.h>


Dimension * newtonDimensionTableAddDimensionForToken(NoisyState *  N, NoisyScope *  scope, NoisyToken *  nameToken, NoisyToken * abbrevToken);
Dimension * newtonDimensionTableDimensionForIdentifier(NoisyState *  N, NoisyScope *  scope, const char *  identifier);


Physics * newtonPhysicsTableAddPhysicsForToken(NoisyState *  N, NoisyScope *  scope, struct NoisyToken *  token);
Physics * newtonPhysicsTablePhysicsForIdentifier(NoisyState *  N, NoisyScope *  scope, const char *  identifier);
Physics * newtonPhysicsTablePhysicsForDimensionAlias(NoisyState *  N, NoisyScope *  scope, const char * dimensionAliasIdentifier);
Physics * newtonPhysicsTableAddPhysics(NoisyState * N, NoisyScope * scope);

Physics* copyPhysicsNode(Physics* list);
Physics* getTailPhysics(Physics* list);

IntegralList* getTailIntegralList(IntegralList* list);

int countNumberTime(Dimension* head);

void newtonPhysicsAddNumeratorDimension(NoisyState * N, Physics * physics, Dimension * numerator); 
void newtonPhysicsAddDenominatorDimension(NoisyState * N, Physics * physics, Dimension * denominator);

void newtonPhysicsCopyNumeratorDimensions(NoisyState * N, Physics * dest, Physics * source);
void newtonPhysicsCopyDenominatorDimensions(NoisyState * N, Physics * dest, Physics * source);
void newtonPhysicsCopyNumeratorToDenominatorDimensions(NoisyState * N, Physics * dest, Physics * source);
void newtonPhysicsCopyDenominatorToNumeratorDimensions(NoisyState * N, Physics * dest, Physics * source);
void newtonAddInvariant(NoisyState * N, Invariant * invariant);

NoisyScope *	newtonSymbolTableAllocScope(NoisyState *  N);
NoisySymbol *	newtonSymbolTableAddOrLookupSymbolForToken(NoisyState *  N, NoisyScope *  scope, NoisyToken *  token);
NoisySymbol *	newtonSymbolTableSymbolForIdentifier(NoisyState *  N, NoisyScope *  scope, const char *  identifier);
NoisyScope *	newtonSymbolTableOpenScope(NoisyState *  N, NoisyScope *  scope, NoisyIrNode *  subtree);
void		        newtonSymbolTableCloseScope(NoisyState *  N, NoisyScope *  scope, NoisyIrNode *  subtree);
