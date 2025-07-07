# version_weaver: fast semver library

A fast and standard compliant implementation of the
[Semantic Versioning](https://semver.org/) standard.


Usage:
```
cmake -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/benchmarks/benchmark 
```

## Current status

The library is currently a prototype. We need more features, more tests, more benchmarks.

## Coding standard

We are currently targeting C++ 23 although C++ 26 might be the next step.

We are assuming that the code can run without exceptions.
