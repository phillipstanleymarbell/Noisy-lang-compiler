#include <string.h>


Dimension * newtonDimensionTableAddDimensionForToken(State *  N, Scope *  scope, Token *  nameToken, Token * abbrevToken);
Dimension * newtonDimensionTableDimensionForIdentifier(State *  N, Scope *  scope, const char *  identifier);


Physics * newtonPhysicsTableAddPhysicsForToken(State *  N, Scope *  scope, struct Token *  token);
Physics * newtonPhysicsTablePhysicsForIdentifier(State *  N, Scope *  scope, const char *  identifier);
Physics * newtonPhysicsTablePhysicsForDimensionAlias(State *  N, Scope *  scope, const char * dimensionAliasIdentifier);
Physics * newtonPhysicsTableAddPhysics(State * N, Scope * scope);

Physics* deepCopyPhysicsNode(Physics* node);
Physics* shallowCopyPhysicsNode(Physics* node);
Physics* copyPhysicsNode(Physics* list);
Physics* getTailPhysics(Physics* list);

IntegralList* getTailIntegralList(IntegralList* list);

int countNumberTime(Dimension* head);

void newtonPhysicsAddNumeratorDimension(State * N, Physics * physics, Dimension * numerator); 
void newtonPhysicsAddDenominatorDimension(State * N, Physics * physics, Dimension * denominator);

void newtonPhysicsCopyNumeratorDimensions(State * N, Physics * dest, Physics * source);
void newtonPhysicsCopyDenominatorDimensions(State * N, Physics * dest, Physics * source);
void newtonPhysicsCopyNumeratorToDenominatorDimensions(State * N, Physics * dest, Physics * source);
void newtonPhysicsCopyDenominatorToNumeratorDimensions(State * N, Physics * dest, Physics * source);
void newtonAddInvariant(State * N, Invariant * invariant);

Scope *	newtonSymbolTableAllocScope(State *  N);
Symbol *	newtonSymbolTableAddOrLookupSymbolForToken(State *  N, Scope *  scope, Token *  token);
Symbol *	newtonSymbolTableSymbolForIdentifier(State *  N, Scope *  scope, const char *  identifier);
Scope *	newtonSymbolTableOpenScope(State *  N, Scope *  scope, IrNode *  subtree);
void		        newtonSymbolTableCloseScope(State *  N, Scope *  scope, IrNode *  subtree);
