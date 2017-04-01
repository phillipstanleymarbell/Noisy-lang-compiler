#include <string.h>


Dimension * newtonDimensionTableAddDimensionForToken(State *  N, Scope *  scope, Token *  nameToken, Token * abbrevToken);
Dimension * newtonDimensionTableDimensionForIdentifier(State *  N, Scope *  scope, const char *  identifier);

Physics * newtonInitPhysics(State * N, Scope * scope, Token * token);
Physics * newtonPhysicsTableAddPhysicsForToken(State *  N, Scope *  scope, struct Token *  token);
Physics * newtonPhysicsTablePhysicsForIdentifier(State *  N, Scope *  scope, const char *  identifier);
Physics * newtonPhysicsTablePhysicsForIdentifierAndSubindex(State *  N, Scope *  scope, const char *  identifier, int subindex);
Physics * newtonPhysicsTablePhysicsForDimensionAlias(State *  N, Scope *  scope, const char * dimensionAliasIdentifier);
Physics * newtonPhysicsTablePhysicsForDimensionAliasAbbreviation(State *  N, Scope *  scope, const char * dimensionAliasAbbreviation);
Physics * newtonPhysicsTableAddPhysics(State * N, Scope * scope);
Physics * newtonPhysicsTableCopyAndAddPhysics(State * N, Scope * scope, Physics * source);

Physics* deepCopyPhysicsNode(Physics* node);
Physics* shallowCopyPhysicsNode(Physics* node);
Physics* getTailPhysics(Physics* list);

IntegralList* getTailIntegralList(IntegralList* list);

int countNumberTime(Dimension* head);

void newtonPhysicsIncrementExponent(State * N, Physics * source, Dimension * added);
void newtonPhysicsAddExponents(State * N, Physics * left, Physics * right);
void newtonPhysicsSubtractExponents(State * N, Physics * left, Physics * right);
void newtonPhysicsMultiplyExponents(State * N, Physics * source, double multiplier);

void newtonAddInvariant(State * N, Invariant * invariant);

Scope *	newtonSymbolTableAllocScope(State *  N);
Symbol *	newtonSymbolTableAddOrLookupSymbolForToken(State *  N, Scope *  scope, Token *  token);
Symbol *	newtonSymbolTableSymbolForIdentifier(State *  N, Scope *  scope, const char *  identifier);
Scope *	newtonSymbolTableOpenScope(State *  N, Scope *  scope, IrNode *  subtree);
void		        newtonSymbolTableCloseScope(State *  N, Scope *  scope, IrNode *  subtree);

bool areTwoPhysicsEquivalent(State * N, Physics * left, Physics * right);
