# ZeroFlow

Minimalist, header-only C++11 finite state machine library focused on timing-based control flows.

## Features

* Portable C++11, no dependencies.
* No dynamic memory allocation.
* No macros or operator overloading.
* MISRA-C++ friendly.
* Simple DSL: `transitionTo(state).wait(ms)`.
* Easily testable on desktop.

### Requirements

* A C++11 compatible compiler (GCC, Clang, MSVC).
* CMake.

### Build

```bash
mkdir build
cd build
cmake ..
make
```

### Run tests

```bash
./tests
```

## Documentation

### API Reference

#### `zeroflow::State<T>`

* **Constructor**: `State(T& obj, void (T::*m)())`
* **Method**: `void invoke() const` – calls the bound member function on the FSM instance.

#### `zeroflow::StateMachine<Derived>`

* **`void setInitialState(const State<Derived>& state)`**
  Sets the starting state. Subsequent calls ignored.

* **`Derived& transitionTo(const State<Derived>& state)`**
  Schedules a pending transition to `state`. Chainable.

* **`Derived& wait(unsigned long ms)`**
  Introduces a delay (milliseconds) before the pending transition occurs.

* **`bool waitCondition(unsigned long ms) const`**
  Virtual hook for custom timing logic. Override in your FSM to return `true` while waiting.

* **`void update()`**
  Executes one cycle:

  1. If in waiting mode, calls `waitCondition`; stays waiting if it returns `true`.
  2. When wait ends, commits any pending transition.
  3. Invokes the current state's behavior exactly once.

### Usage Example

```cpp
#include "ZeroFlow.hpp"
#include <iostream>
#include <chrono>
#include <thread>

using namespace zeroflow;

class Blink : public StateMachine<Blink> {
private:
    State<Blink> offState;
    State<Blink> onState;
    std::chrono::steady_clock::time_point t0;

public:
    Blink() 
      : offState(*this, &Blink::off), 
        onState(*this, &Blink::on) {
        setInitialState(offState);
    }

    void off() {
        std::cout << "LED OFF
";
        t0 = std::chrono::steady_clock::now();
        transitionTo(onState).wait(500);
    }

    void on() {
        std::cout << "LED ON
";
        t0 = std::chrono::steady_clock::now();
        transitionTo(offState).wait(500);
    }

    bool waitCondition(unsigned long ms) const override {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - t0).count();
        return elapsed < ms;
    }
};

int main() {
    Blink blink;
    for (int i = 0; i < 10; ++i) {
        blink.update();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return 0;
}
```

### Notes

* Fully header-only: just include `ZeroFlow.hpp`.
* No external dependencies: only `<chrono>` and `<thread>` if using PC tests.
* Zero dynamic memory; suitable for bare-metal.
* Recommended C++11 or newer.

## License

MIT
