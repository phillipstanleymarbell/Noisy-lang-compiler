#!/bin/bash
set -e

file="break_down.log"

if [ -f "$file" ]; then
  rm "$file"
fi

# run without newton
make perf_float64_add
echo "without CoSense" >> "$file"
for i in {1..100}; do
  ./main_out -134.5 -133.8 >> "$file"
done

# run with newton
cp $"pwd"../../../../src/newton/newton-linux-EN_v0_1 $"pwd"../../../../src/newton/newton-linux-EN
make perf_float64_add_opt
echo "with type compression" >> "$file"
for i in {1..100}; do
  ./main_out -134.5 -133.8 >> "$file"
done

cp $"pwd"../../../../src/newton/newton-linux-EN_v0_2 $"pwd"../../../../src/newton/newton-linux-EN
make perf_float64_add_opt
echo "with branch elimination" >> "$file"
for i in {1..100}; do
  ./main_out -134.5 -133.8 >> "$file"
done

cp $"pwd"../../../../src/newton/newton-linux-EN_v0_3 $"pwd"../../../../src/newton/newton-linux-EN
make perf_float64_add_opt
echo "with const substitution" >> "$file"
for i in {1..100}; do
  ./main_out -134.5 -133.8 >> "$file"
done

cp $"pwd"../../../../src/newton/newton-linux-EN_v0 $"pwd"../../../../src/newton/newton-linux-EN
make perf_float64_add_opt
echo "overall" >> "$file"
for i in {1..100}; do
  ./main_out -134.5 -133.8 >> "$file"
done
