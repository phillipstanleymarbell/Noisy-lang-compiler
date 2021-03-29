CC= ../submodules/llvm-project/build/bin/clang
CFLAGS=-g `llvm-config --cflags` 
LD=../submodules/llvm-project/build/bin/clang++
LDFLAGS=`llvm-config --cxxflags --ldflags --libs core executionengine mcjit interpreter analysis native bitwriter target --system-libs` 

all: sum

sum.o: sum.c
	$(CC) $(CFLAGS) -c $<

sum: sum.o
	$(LD) $< -o $@ $(LDFLAGS)

sum.bc: sum
	./sum 0 0

sum.ll: sum.bc
	llvm-dis $<

clean:
	-rm -f sum.o sum sum.bc sum.ll
