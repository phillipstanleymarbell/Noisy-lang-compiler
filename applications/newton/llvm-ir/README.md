## Dimensionality check with Newton and LLVM IR

Currently, when we give an LLVM IR file with `--llvm-ir` to Newton, it performs a dimensionality check with it.

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

## Liveness check with Newton and LLVM IR

Pass the flag `--llvm-ir-liveness-check`, to do a liveness check for a LLVM IR file given with `--llvm-ir`.

### Simple example - `if.c`

```make
cd /path/to/Noisy-lang-compiler/applications/newton/llvm-ir
make if.ll
cd ../../../src/newton
./<newton-executable> --llvm-ir=../../applications/newton/llvm-ir/if.ll --llvm-ir-liveness-check ../../applications/newton/invariants/LLVMIRNewtonExample.nt
```

### Simplify control flow by variable range

```make
cd /path/to/Noisy-lang-compiler/applications/newton/llvm-ir
make infer_bound_control_flow.ll
cd ../../../src/newton
./<newton-executable> --llvm-ir=../../applications/newton/llvm-ir/infer_bound_control_flow.ll --llvm-ir-liveness-check --llvm-ir-enable-overload --llvm-ir-enable-builtin-assume ../../applications/newton/sensors/test.nt
opt infer_bound_control_flow_output.ll -O2 -S -o out.ll
```

#### performance test

See `performance_test/README.md`
