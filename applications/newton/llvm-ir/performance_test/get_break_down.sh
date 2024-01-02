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
  ./main_out 0 200 >> "$file"
done
wc -l out.ll >> "$file"

# run with newton
cp "$PWD"/../../../../src/newton/newton-linux-EN_v0_1 "$PWD"/../../../../src/newton/newton-linux-EN
make perf_float64_add_opt
echo "with type compression" >> "$file"
for i in {1..100}; do
  ./main_out 0 200 >> "$file"
done
wc -l out.ll >> "$file"

cp "$PWD"/../../../../src/newton/newton-linux-EN_v0_2 "$PWD"/../../../../src/newton/newton-linux-EN
make perf_float64_add_opt
echo "with branch elimination" >> "$file"
for i in {1..100}; do
  ./main_out 0 200 >> "$file"
done
wc -l out.ll >> "$file"

cp "$PWD"/../../../../src/newton/newton-linux-EN_v0_3 "$PWD"/../../../../src/newton/newton-linux-EN
make perf_float64_add_opt
echo "with const substitution" >> "$file"
for i in {1..100}; do
  ./main_out 0 200 >> "$file"
done
wc -l out.ll >> "$file"

cp "$PWD"/../../../../src/newton/newton-linux-EN_v0 "$PWD"/../../../../src/newton/newton-linux-EN
make perf_float64_add_opt
echo "overall" >> "$file"
for i in {1..100}; do
  ./main_out 0 200 >> "$file"
done
wc -l out.ll >> "$file"
