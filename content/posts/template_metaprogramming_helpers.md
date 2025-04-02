---
title: "Tiny C++ template metaprogramming helpers"
author: Vaibhav Awale
date: "2025-03-30"
description: "Tiny helpers for C++ template metaprogramming"
tags:
    - c++
    - templates
    - metaprogramming
    - fold expressions
    - functional programming
---

# Using fold expressions for common tasks

In functional programming, `fold` is a standard operator that encapsulates a simple pattern of
recursion for processing lists<sup>1</sup>. In the following sections, I explore usage of fold
operator for some common operations on template parameter pack. The template parameter pack can be
thought of as a heterogeneous sequence of types.

## count

```cpp
template<typename... Args>
consteval auto count() {
    return sizeof...(Args);
}
```
Above we use `sizeof...` operator available since C++11, see [sizeof...](https://en.cppreference.com/w/cpp/language/sizeof...)

## count_if

```cpp
template<template <typename T> typename Predicate, typename... Args>
consteval auto count_if() -> size_t {
    return (0 + ... + Predicate<Args>{}());
}
```

The `Predicate` passed above needs to be a template type that defines a compile time (`consteval`)
function call `operator() -> bool` operator. Above functions folds over sum `+` binary operator.
Example of such a `Predicate`:

```cpp
template<typename T>
struct is_signed_type {
    consteval bool operator ()() const {
        return std::is_signed_v<T>;
    }
};
```

Example usage of `count_if`:

```cpp
// evaluate to 2 at compile time
count_if<is_signed_type, int, unsigned int, double>(); 
```

## all_of

```cpp
template<template <typename T> typename Predicate, typename... Args>
consteval auto all_of() -> bool {
    return (Predicate<Args>() && ...);
}
```
Folds over the binary and (`&&`) operator.

## any_of

```cpp
template<template <typename T> typename Predicate, typename... Args>
consteval auto any_of() -> bool {
    return (Predicate<Args>() || ...);
}
```
Folds over binary or (`||`) operator.

## none_of

```cpp
template<template <typename T> typename Predicate, typename... Args>
consteval auto none_of() -> bool {
    return !any_of<Predicate, Args...>();
}
```

## for_each

```cpp
template<template <typename T> typename Func, typename... Args>
consteval void for_each() {
    (Func<Args>(), ...);
}
```
The `Func` passed above needs to be a template type that defines a compile time (`consteval`)
function call `operator() -> void` operator. Here, we use comma operator `,` as the binary operator to fold over. Example of such a `Func`:

```cpp
template<typename T>
struct assert_arithmetic {
    consteval void operator ()() const {
        static_assert(std::is_arithmetic_v<T>, "T must be arithmetic type");
    }
};
```

## Traversing tuple types

### When tuple is passed as input argument

```cpp
template<template <typename T> typename Func, typename... Args>
consteval void for_each_tuple_type(std::tuple<Args...>) {
    (Func<Args>(), ...);
}
```

I apply fold over comma operator `,` to call `Func` for each type in the tuple. This function
however requires passing the tuple as input argument even though the argument is unused. The
argument only serves to deduce the types in the tuple.

### When tuple is passed as template argument

```cpp
template<template <typename T> typename Func, template Tuple>
consteval void for_each_tuple_type() {
    auto evaluator = []<typename Tuple, template <typename T> typename Func, size_t... I>(std::index_sequence<I...>) {
        (Func<std::tuple_element_t<I, Tuple>>{}(), ...);
    };
    evaluator.template operator() <Tuple, Func>
    (std::make_index_sequence<std::tuple_size_v<Tuple>>());
}
```

This function uses C++20 feature for template lambdas. The template lambda function here is useful
to pass an index sequence to index into tuple to get the types. The fold operation remains same as
before, i.e., fold over comma `,` operator.

Explore the example usage of all above helpers on [compiler explorer](https://godbolt.org/z/6Ysr8cf3f).

# References

1. [A tutorial on the universality and expressiveness of fold](https://people.cs.nott.ac.uk/pszgmh/fold.pdf)
2. [Not getting lost in translations - Daniela Engert - NDC TechTown 2024](https://www.youtube.com/watch?v=rxBQIQFQirE)
