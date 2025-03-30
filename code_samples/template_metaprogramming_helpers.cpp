#include <type_traits>
#include <tuple>
#include <cstddef>

template<typename... Args>
consteval auto count() -> size_t {
    return sizeof...(Args);
}

template<template <typename T> typename Predicate, typename... Args>
consteval auto count_if() -> size_t {
    return (... + Predicate<Args>());
}

template<template <typename T> typename Predicate, typename... Args>
consteval auto all_of() -> bool {
    return (Predicate<Args>() && ...);
}

template<template <typename T> typename Predicate, typename... Args>
consteval auto any_of() -> bool {
    return (Predicate<Args>() || ...);
}

template<template <typename T> typename Predicate, typename... Args>
consteval auto none_of() -> bool {
    return !any_of<Predicate, Args...>();
}

template<template <typename T> typename Func, typename... Args>
consteval void for_each() {
    (Func<Args>(), ...);
}

template<template <typename T> typename Func, typename... Args>
consteval void for_each_tuple_type_arg(std::tuple<Args...>) {
    (Func<Args>(), ...);
}

template<template <typename T> typename Func, typename Tuple>
consteval void for_each_tuple_type() {
    auto evaluator = []<typename TupleLambda, template <typename T> typename FuncLambda, size_t... I>(std::index_sequence<I...>) {
        (FuncLambda<std::tuple_element_t<I, TupleLambda>>(), ...);
    };
    evaluator.template operator() <Tuple, Func>
    (std::make_index_sequence<std::tuple_size_v<Tuple>>());
}

template<typename T>
struct is_signed_type {
    consteval operator bool() const {
        return std::is_signed_v<T>;
    }
};

template<typename T>
struct assert_arithmetic {
    consteval void operator ()() const {
        static_assert(std::is_arithmetic_v<T>, "T must be arithmetic type");
    }
};

int main()
{
    for_each<assert_arithmetic, int, float, unsigned int>();
    using some_numeric_types = std::tuple<int, float, unsigned int>;
    for_each_tuple_type_arg<assert_arithmetic>(some_numeric_types{});
    for_each_tuple_type<assert_arithmetic, some_numeric_types>();
    return count<int, int, float>() 
    + count_if<is_signed_type, int, unsigned int, double>()
    + all_of<is_signed_type, int, double>()
    + any_of<is_signed_type, int, size_t>()
    + none_of<is_signed_type, unsigned int, size_t>();

}
