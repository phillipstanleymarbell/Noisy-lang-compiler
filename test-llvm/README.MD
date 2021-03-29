# Build LLVM project

* ``cd llvm-project``

* ``mkdir build``

* ``cd build``

* `` cmake ../llvm -G "Unix Makefiles" -DBUILD_SHARED_LIBS=on -DLLVM_ENABLE_PROJECTS=clang -DLLVM_USE_LINKER=gold``

For more info you can see at this, taken from https://llvm.org/docs/GettingStarted.html.
``cmake -S llvm -B build -G <generator> [options]``

        Some common build system generators are:
    
        * ``Ninja`` --- for generating [Ninja](https://ninja-build.org)
          build files. Most llvm developers use Ninja.
        * ``Unix Makefiles`` --- for generating make-compatible parallel makefiles.
        * ``Visual Studio`` --- for generating Visual Studio projects and
          solutions.
        * ``Xcode`` --- for generating Xcode projects.
    
        Some Common options:
    
        * ``-DLLVM_ENABLE_PROJECTS='...'`` --- semicolon-separated list of the LLVM
          sub-projects you'd like to additionally build. Can include any of: clang,
          clang-tools-extra, libcxx, libcxxabi, libunwind, lldb, compiler-rt, lld,
          polly, or debuginfo-tests.
    
          For example, to build LLVM, Clang, libcxx, and libcxxabi, use
          ``-DLLVM_ENABLE_PROJECTS="clang;libcxx;libcxxabi"``.
    
        * ``-DCMAKE_INSTALL_PREFIX=directory`` --- Specify for *directory* the full
          path name of where you want the LLVM tools and libraries to be installed
          (default ``/usr/local``).
    
        * ``-DCMAKE_BUILD_TYPE=type`` --- Valid options for *type* are Debug,
          Release, RelWithDebInfo, and MinSizeRel. Default is Debug.
    
        * ``-DLLVM_ENABLE_ASSERTIONS=On`` --- Compile with assertion checks enabled
          (default is Yes for Debug builds, No for all other build types).
    
      * ``cmake --build build [-- [options] <target>]`` or your build system specified above
        directly.
    
        * The default target (i.e. ``ninja`` or ``make``) will build all of LLVM.
    
        * The ``check-all`` target (i.e. ``ninja check-all``) will run the
          regression tests to ensure everything is in working order.
    
        * CMake will generate targets for each tool and library, and most
          LLVM sub-projects generate their own ``check-<project>`` target.
    
        * Running a serial build will be **slow**.  To improve speed, try running a
          parallel build.  That's done by default in Ninja; for ``make``, use the option
          ``-j NNN``, where ``NNN`` is the number of parallel jobs, e.g. the number of
          CPUs you have.

* ``make -j4``
This will take some time.
You can use more or less than 4 cores depending on your system.

# Add llvm to the path and llvm library to the linker path.

* ``sudo vim /etc/environment``

* Inside the environment file we add to the PATH variable the /home/PATH-TO/Noisy-lang-compiler/submodules/llvm-project/build/bin.

* We create a conf file for the llvm lib. ``sudo vim /etc/ld.so.conf.d/llvm-lib.conf``

* Inside the llvm-lib.conf we just write ``/home/PATH-TO/Noisy-lang-compiler/submodules/llvm-project/build/lib`` .

* ``sudo ldconfig``

# Test LLVM

* Go to test-llvm directory.
* ``make``
* The example sum.c should work and create a sum.bc file as well as an executable sum that adds two numbers. In order to see the llvm IR of this example we can use ``llvm-dis sum.bc > less``.

* From this point on all the llvm tools and clang should work.