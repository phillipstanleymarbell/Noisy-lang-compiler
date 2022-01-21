## Dimensionality check with Newton and LLVM IR

### Simple example - `application.c`

```make
cd /path/to/Noisy-lang-compiler/applications/newton/llvm-ir
make
cd ../../../src/newton
./<newton-executable> --llvm-ir=../../applications/newton/llvm-ir/application.ll ../../applications/newton/invariants/LLVMIRNewtonExample.nt
```

### ADC Routine

```make
cd /path/to/Noisy-lang-compiler/applications/newton/llvm-ir/adc_test
make
cd ../../../../src/newton
./<newton-executable> --llvm-ir=../../applications/newton/llvm-ir/adc_test/test_hum_adc.ll ../../applications/newton/invariants/LLVMIRNewtonExample.nt
```