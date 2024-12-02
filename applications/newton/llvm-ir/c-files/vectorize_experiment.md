# Experiment Results of vectorization

## Compile only with `Clang`
### x86-64
```bash
clang -O1 vec_add.c -o vec_add # 0.000209616 s
clang -O1 vec_add.c -o vec_add -fvectorize # 0.000157489 s
clang -O1 vec_add_8.c -o vec_add_8 # 0.000111221 s
clang -O1 vec_add_8.c -o vec_add_8 -fvectorize # 0.000048906 s
```

#### arm64
```bash
clang --target=aarch64-arm-none-eabi -O1 vec_add.c -o vec_add # 0.001143304 s
clang --target=aarch64-arm-none-eabi -O1 vec_add.c -o vec_add -fvectorize # 0.000856311 s
clang --target=aarch64-arm-none-eabi -O1 vec_add_8.c -o vec_add_8 # 0.000776979 s
clang --target=aarch64-arm-none-eabi -O1 vec_add_8.c -o vec_add_8 -fvectorize # 0.000201536 s
```

## Compile with `Clang` and `opt`
### x 86-64
```bash
clang -O0 -g -Xclang -disable-O0-optnone vec_add.c -S -emit-llvm -o vec_add.ll
opt vec_add.ll --O1 -S -o vec_add_none_opt.ll
clang vec_add_none_opt.ll -o vec_add_none_opt
./vec_add_none_opt # 0.000328377 s
opt vec_add.ll --O1 --loop-vectorize -S -o vec_add_opt.ll
clang vec_add_opt.ll -o vec_add_opt
./vec_add_opt # 0.000319101 s
clang -O0 -g -Xclang -disable-O0-optnone vec_add_8.c -S -emit-llvm -o vec_add_8.ll
opt vec_add_8.ll --O1 -S -o vec_add_8_none_opt.ll
clang vec_add_8_none_opt.ll -o vec_add_8_none_opt
./vec_add_8_none_opt # 0.000207441 s
opt vec_add_8.ll --O1 --loop-vectorize -S -o vec_add_8_opt.ll
clang vec_add_8_opt.ll -o vec_add_8_opt
./vec_add_8_opt # 0.000206795 s
```

### arm64
```bash
clang --target=aarch64-arm-none-eabi -O0 -g -Xclang -disable-O0-optnone vec_add.c -S -emit-llvm -o vec_add.ll
opt vec_add.ll --O1 -S -o vec_add_none_opt.ll
clang --target=aarch64-arm-none-eabi vec_add_none_opt.ll -o vec_add_none_opt
./vec_add_none_opt # 0.002345815 s
opt vec_add.ll --O1 --loop-vectorize -S -o vec_add_opt.ll
clang --target=aarch64-arm-none-eabi vec_add_opt.ll -o vec_add_opt
./vec_add_opt # 0.000947018 s
clang --target=aarch64-arm-none-eabi -O0 -g -Xclang -disable-O0-optnone vec_add_8.c -S -emit-llvm -o vec_add_8.ll
opt vec_add_8.ll --O1 -S -o vec_add_8_none_opt.ll
clang --target=aarch64-arm-none-eabi vec_add_8_none_opt.ll -o vec_add_8_none_opt
./vec_add_8_none_opt # 0.002099071 s
opt vec_add_8.ll --O1 --loop-vectorize -S -o vec_add_8_opt.ll
clang --target=aarch64-arm-none-eabi vec_add_8_opt.ll -o vec_add_8_opt
./vec_add_8_opt # 0.000227494 s
```

## Run with Newton Compiler
```bash
cd ../../../../src/newton
./newton-linux-EN --llvm-ir=../../applications/newton/llvm-ir/vec_add.ll --llvm-ir-liveness-check ../../applications/newton/sensors/test.nt
cd -
llvm-dis ../vec_add_output.bc
opt ../vec_add.ll --O1 --loop-vectorize -S -o vec_add_opt.ll
clang vec_add_opt.ll -o vec_add_opt
./vec_add_opt # 0.000318110 s
opt ../vec_add_output.ll --O1 --loop-vectorize -S -o vec_add_output_opt.ll
clang vec_add_output_opt.ll -o vec_add_output_opt
./vec_add_output_opt # 0.000205080 s
```