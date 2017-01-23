#include <string.h>


Dimension * newtonDimensionTableAddDimensionForToken(NoisyState *  N, NoisyScope *  scope, NoisyToken *  nameToken, NoisyToken * abbrevToken);
Dimension * newtonDimensionTableDimensionForIdentifier(NoisyState *  N, NoisyScope *  scope, const char *  identifier);

Physics * newtonPhysicsTableAddPhysicsForToken(NoisyState *  N, NoisyScope *  scope, struct NoisyToken *  token);
Physics * newtonPhysicsTablePhysicsForIdentifier(NoisyState *  N, NoisyScope *  scope, const char *  identifier);

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

NoisyScope *	newtonSymbolTableAllocScope(NoisyState *  N);
NoisySymbol *	newtonSymbolTableAddOrLookupSymbolForToken(NoisyState *  N, struct NoisyScope *  scope, struct NoisyToken *  token);
NoisySymbol *	newtonSymbolTableSymbolForIdentifier(NoisyState *  N, struct NoisyScope *  scope, const char *  identifier);
NoisyScope *	newtonSymbolTableOpenScope(NoisyState *  N, struct NoisyScope *  scope, struct NoisyIrNode *  subtree);
void		        newtonSymbolTableCloseScope(NoisyState *  N, struct NoisyScope *  scope, struct NoisyIrNode *  subtree);
