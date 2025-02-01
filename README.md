# mr-contractor
Declarative C++ task execution library based on work contracts. \
It aims at providing performant solution to repeatable task graphs problem.

## Usage
```cpp
#include "mr-contractor/pipe.hpp"

int main() {
    auto prototype = mr::PipePrototype {
        std::function([](int x)   -> float    { return (float)x / 2; }),
        std::function([](float x) -> int      { return (int)x;       }),
        std::function([](int x)   -> unsigned { return std::abs(x);  })
    };

    auto pipe = prototype.on(30);
    pipe->schedule(); // schedules task described in prototype on argument 30
    pipe->wait();
    std::println("Result: {}", pipe2->result());

    // declare pipe first
    // NOTE: only the prototype creating concrete pipes knows their type
    using PrototypeT = decltype(prototype);
    PrototypeT::PipeHandleT pipe2;

    // use declared pipe inside itself
    // on_finish (2nd argument) is called when
    //   all stages described in prototype are finished
    // NOTE: on_finish must be a `void()` callable
    pipe2 = prototype.on(
        47,
        [&pipe2]() -> void {
            std::println("Result: {}", pipe2->result());
        }
    );
    for (;;) {
        // calculates the result and then prints it
        // NOTE: the result isn't cached, it's always calculated
        pipw2->execute();
    }
}
```

## Download the library
  - via [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake) in your CMake script (suggested):
    ```cmake
    CPMAddPackage("gh:4J-company/mr-contractor#master")

    target_link_libraries(<your-project> PUBLIC mr-contractor-lib)
    ```
