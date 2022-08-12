# Tests for libzstd-seek

This is a repository with tests for [libzstd-seek](https://github.com/martinellimarco/libzstd-seek)

## Dependencies

The tests require [googletest](https://github.com/google/googletest).

It was tested with version 1.21.1

## How to run the tests

Clone this repository with `git clone --recursive` as it will also clone `libzstd-seek`.

Compile with

```
mkdir build
cd build
cmake ..
make
cd ..
```

and finally run the tests with `./build/tests`.

The folder `test_assets` must be in the working directory to run the tests.

## Licensing

See LICENSE
