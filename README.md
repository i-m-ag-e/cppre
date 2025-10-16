# cppre: A basic regular expression engine based on a virtual-machine implementaion of Thompson's NFA

This is a basic implementation of a regular expression engine based on this article by Russ Cox

[Regular Expression Matching: the Virtual Machine Approach
](https://swtch.com/~rsc/regexp/regexp2.html).


It uses a virtual machine to interpret an NFA constructed from a given regular expression.

## Building
This project can be built using CMake (requires >= C++17).

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

Tests are built by default and can be run using `ctest` in the build directory. Hence the build depends on GoogleTest being installed.

If you wish to disable building tests, you can do so by passing `-DBUILD_TESTS=OFF` to the `cmake` command.