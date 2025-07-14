# ZeroFlow

Minimalist, header-only C++11 finite state machine library focused on timing-based control flows.

## Features

- Portable C++11, no dependencies.
- No dynamic memory allocation.
- No macros or operator overloading.
- MISRA-C++ compatible.
- Simple DSL: `transitionTo(state).wait(ms)`.
- Easily testable on desktop.


### Requirements

- A C++11 compatible compiler (GCC, Clang, MSVC).
- CMake.

### Build

```bash
mkdir build
cd build
cmake ..
make
```

### Run tests

```bash
tests
```

## License

MIT
