void noisyTypeCheck(State * N);
bool noisySignatureIsMatching(State * N, IrNode * definitionSignature, IrNode * declarationSignature);


typedef enum
{
        noisyInitType,
        noisyBool,
        noisyInt4,
        noisyInt8,
        noisyInt16,
        noisyInt32,
        noisyInt64,
        noisyInt128,
        noisyIntegerConstType,
        noisyNat4,
        noisyNat8,
        noisyNat16,
        noisyNat32,
        noisyNat64,
        noisyNat128,
        noisyFloat16,
        noisyFloat32,
        noisyFloat64,
        noisyFloat128,
        noisyRealConstType,
        noisyString,
        noisyArrayType,
        noisyTypeError
} NoisyBasicType;

typedef struct NoisyType
{
        NoisyBasicType basicType;
        int dimensions;
        NoisyBasicType arrayType;
        int * sizeOfDimension;
} NoisyType;


NoisyType getNoisyTypeFromTypeExpr(State * N, IrNode * typeExpr);
NoisyType getNoisyTypeFromBasicType(IrNode * basicType);
void deallocateNoisyType(NoisyType * typ);