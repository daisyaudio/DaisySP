# Initialization regression tests

These host-only tests exercise public oscillator and AD envelope initialization
with synthetic samples. They do not open audio devices or require libDaisy.

From the repository root, using CMake and a C++14 compiler:

```sh
cmake -S . -B build-host
cmake --build build-host
c++ -std=c++14 -I Source tests/initialization/tst_initialization.cpp build-host/libDaisySP.a -o test-initialization
./test-initialization
```

The tests process a polyBLEP triangle, reinitialize the oscillator, and check
that its integrator starts from zero. They also reinitialize an AD envelope
with a pending trigger and verify that processing leaves it idle. Static test
objects make the original failure deterministic without reading arbitrary
uninitialized memory. Both cases run at 44.1, 48, and 96 kHz.
