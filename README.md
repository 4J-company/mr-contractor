# mr-contractor
Declarative C++ task execution library based on work contracts. \
It aims at providing performant solution to repeatable task graphs problem.

## Usage
```cpp
#include "mr-contractor/contractor.hpp"

int main() {
    auto prototype = mr::Sequence {
        [](int x)   -> float    { return (float)x / 2; },
        [](float x) -> int      { return (int)x;       },
        [](int x)   -> unsigned { return std::abs(x);  }
    };

    // create concrete schedulable task by binding invokable described in prototype above
    auto task = mr::apply(prototype, 30);

    task->schedule();
    task->wait();
    std::println("Result: {}", task->result());

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
