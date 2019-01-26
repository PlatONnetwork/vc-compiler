# vlang compiler manual

--------
- [Verifiable Computation Contract Guide] (#Verifiable Computation Contract Guide)
    - [Brief Description](#brief-description)
    - [Compiling](#compiling)
    - [Encoding Constraints](#encoding-constraints)
    - [Usage](#usage)
    - [Additional Compilation Options](#additional-compilation-options)
    - [Example](#example)

------------------

## Brief Description
The vlang compiler is based on the llvm, clang compiler, and libsnark password library, supports all options of the clang/gcc compiler, and adds some additional options.

## Compiling
* Installation dependencies

```bash
    sudo apt-get install pkg-config libboost-all-dev
    sudo apt-get install llvm-6.0-dev llvm-6.0 libclang-6.0-dev
    sudo apt-get install libgmpxx4ldbl libgmp-dev libprocps4-dev libssl-dev
```

If the installation is not successful, please try:

```bash
    sudo apt-get upgrate
```

* Download submodule

```bash
    git submodule update --init --recursive
```
* Download llvm

Take a directory-based example:/opt

```bash
    wget http://releases.llvm.org/6.0.1/clang+llvm-6.0.1-x86_64-linux-gnu-ubuntu-16.04.tar.xz.sig
	xz -d clang+llvm-6.0.1-x86_64-linux-gnu-ubuntu-16.04.tar.xz.sig
	tar zxvf clang+llvm-6.0.1-x86_64-linux-gnu-ubuntu-16.04.tar
```
* Compile vlang

```bash
    mkdir build
    cd build
    cmake .. -DLLVM_CONFIG=/opt/clang+llvm-6.0.1-x86_64-linux-gnu-ubuntu-16.04/bin/llvm-config
    make
```

Compiled successfully, you can find *vlang in the vlang directory.

## Encoding Constraints
```comment
    The name of the main function must be vc_main
    vc_main function prototype can not appear pointer / reference type
    The step number of loop/recursion must be determined at compile time
    Array addressing index must be constant
    Cannot use IO operations
    Cannot use dynamic memory allocation
    Cannot use floating point type
```

## Usage
```text
    vlang [clang args] [options] <inputs>
    clang args: options supported by clang/gcc
    options: additional options for vlang
    inputs: input file (you can enter multiple files, will be linked)
```

## Additional Compilation Options
```bash
    --help print option
    -args=<arg> Setting debug parameters
    -ir <file> llvm IR output to <file> (not output by default)
    -o <file> The contract file is output to <file> (default output to vcc.cpp)
```

## Example
Write a program below, give the integer x, return the 5th power of x

```cpp
int power(int x, int n){
  int a=1;
  while(n>0){
    a = a * x;
    n--;
  }
  return a;
}

int vc_main(int x){
  return power(x, 5);
}
```
Save as test.cpp

Compile:

```bash
./vlang test.cpp -o vcc.cpp
```
