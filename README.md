#### A Declarative C++ Task Execution Library for Complex Workflows

---

**🔍 What is mr-contractor?**  
`mr-contractor` is a header-only C++ library that lets you **declaratively define task execution graphs** with seamless support for:  
- **Sequential & parallel stages**  
- **Arbitrary nesting** (parallel inside sequential, sequential inside parallel, etc.)  
- **Automatic dependency resolution**  
- **Zero-boilerplate thread pooling**  

Built on latest C++ features, it’s perfect for scientific computing, game engines, or any domain requiring complex, repeatable workflows.

---

**🎯 Key Features**  
- **Declarative API**: Describe *what* your workflow does, not *how* to schedule it.  
- **Work Contracts**: Underlying thread pool powered by [`work_contract`](https://github.com/buildingcpp/work_contract) for minimal overhead.  
- **Type Safety**: Compile-time validation of task input/output types.  

---

**🚀 Usage Examples**  

**1. Simple Sequential Pipeline**  
```cpp  
auto task = Sequence{  
  [](int x) -> float { return x / 2.0f; },  
  [](float y) -> std::string { return std::to_string(y); }  
};  
auto result = apply(task, 42)->execute()->result();  
// result = "21.0"  
```  

**2. Parallel Processing**  
```cpp  
auto task = Parallel{  
  [](int a) { return a * a; },     // Task 1  
  [](int b) { return b + 10; }     // Task 2 (runs concurrently)  
};  
auto results = apply(task, std::tuple{2, 3})->execute()->result();  
// results = (4, 13)  
```  

**3. Nested Workflows**  
```cpp  
auto task = Sequence{  
  [](int x) { return std::tuple{x, x*2}; },  
  Parallel{  
    Sequence{  // Nested sequential steps  
      [](int a) { return a + 1; },  
      [](int b) { return std::to_string(b); }  
    },  
    [](int c) { return c * 0.5f; }  
  },  
  [](auto&& inputs) {  
    auto&& [str, flt] = inputs;  
    return str + " @ " + std::to_string(flt);  
  }  
};  
auto result = apply(task, 5)->execute()->result();  
// result = "6 @ 5.0"  
```  

---
## Download the library
  - via [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake) in your CMake script (suggested):
    ```cmake
    CPMAddPackage("gh:4J-company/mr-contractor#master")
    target_link_libraries(<your-project> PUBLIC mr-contractor-lib)
    ```
---

**We’d Love Your Feedback!**  
- Found a bug? Open an issue!  
- Have a feature request? Let’s discuss!  
- Built something cool with it? Share your story!  
