---
title: "Forwarding function objects"
author: Vaibhav Awale
date: "2024-04-28"
description: "Exploring rvalue function call overload"
tags:
    - c++
    - function objects
    - functional programming
---

## clang-tidy check - cppcoreguidelines-missing-std-forward

Recently I worked on updating our llvm toolchain to version 17. This brought in some nice new static
analysis checks from clang-tidy-17. One interesting check introduced is
[cppcoreguidelines-missing-std-forward](https://clang.llvm.org/extra/clang-tidy/checks/cppcoreguidelines/missing-std-forward.html),
that enforces C++ core guideline
[F.19](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rf-forward).

The guideline is pretty clear to understand for non-function types in template arguments. However, does
it serve any purpose when template argument refers to a function/callable type? Consider the following example:

```cpp
template <typename Function, typename ReturnType = std::invoke_result_t<Function>>
ReturnType wrapper(Function&& function)
{
    return function();
}
```

clang-tidy check asks to use `std::forward` for `function` argument -
[godbolt](https://godbolt.org/z/6c8qna58q).

```sh
error: forwarding reference parameter 'function' is never forwarded inside the function body [cppcoreguidelines-missing-std-forward,-warnings-as-errors]
    6 | ReturnType wrapper(Function&& function)
      |                               ^
10 warnings generated.
```

Updating wrapper to forward `function` argument fixes the issue:

```cpp
template <typename Function, typename ReturnType = std::invoke_result_t<Function>>
ReturnType wrapper(Function&& function)
{
    return std::forward<Function>(function)();
}
```

But in what cases does forwarding a function object help?

## Calling the right overload of member function

C++11 onwards, class member functions can be qualified with ref-qualifier. During overload
resolution, compiler chooses the right member function to call based on if object is rvalue/lvalue.
See section [Member functions with ref
qualifier](https://en.cppreference.com/w/cpp/language/member_functions). Providing rvalue overload
of few member functions of your class can be helpful for performance optimization, because rvalue
overload can assume that class data is not going to be used after they are called. For example,
`std::stringstream::str` method has a rvalue overload, see
[(3)](https://en.cppreference.com/w/cpp/io/basic_stringstream/str). This overload returns a string
move-constructed from the underlying string, thus avoiding a potential memory allocation and copy.

Consider following example function object that uses `std::stringstream::str` in a function object:

```cpp
struct function_object
{
    std::string operator()() const &
    {
        std::cout << "lvalue overload, needs to return copy of data from stringstream\n";
        return value.str();
    }

    std::string operator()() &&
    {
        std::cout << "rvalue overload, can move data from stringstream\n";
        return std::move(value).str();
    }

    std::stringstream value;
};
```

Note that the lvalue overload of function call operator `()` in above class cannot move `value`
because caller would expect the `function_object.value` to be non-modified, whereas the rvalue
overload can make use of rvalue overload of `stringstream::str` method because
`function_object.value` will not be used anymore.  

Consider following usage:

```cpp
function_object fn_object{};
fn_object.value << "Store some data in stream";
// fn_object is xlvalue in next expression, so we expect to use `std::string function_object::operator()&&`
auto value = wrapper(std::move(fn_object));
```

If we don't use `std::forward` on `function` input argument of `wrapper` function, we end up using
lvalue overload of our function_object `()` operator, even if we expected to use rvalue function
object overload! Example - [godbolt](https://godbolt.org/z/8Pf98xz6W).

Following clang-tidy check suggestion, we get the expected result -
[godbolt](https://godbolt.org/z/ase5v8EoK). This is because we pass *rvalueness* of function object
correctly from the wrapper function.

## Conclusion

Always use `std::forward` if object is taken using universal reference. Also, use static analysis
tools like clang-tidy which catch such issues and help improve code quality.
