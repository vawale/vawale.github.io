---
title: "Emplace back iterator"
author: Vaibhav Awale
date: "2024-08-03"
description: "Writing iterator adapter that calls container's emplace_back member function"
tags:
    - c++
    - functional programming
    - C++
---

# Motivation

Recently I came across a use-case to create a collection of non-copyable and non-movable objects.
Size of the collection will only be known at runtime, so this rules out using `std::array`. I cannot
use `std::vector` as well, since the objects are non-copyable and non-movable.
[std::vector::emplace_back](https://en.cppreference.com/w/cpp/container/vector/emplace_back)
requires the type to be [MoveInsertable](https://en.cppreference.com/w/cpp/named_req/MoveInsertable)
to allow for reallocation to maintain contiguous memory guarantee. I need to use `std::deque` for
such cases, since
[std::deque::emplace_back](https://en.cppreference.com/w/cpp/container/deque/emplace_back) does not
need to reallocate and hence does not have `MoveInsertable` requirement on the type. 

Consider following example of non-copyable, non-movable type:

```cpp
class ProgressBar
{
public:
    ProgressBar(std::streambuf* buffer, std::string prefix) 
    : m_out{buffer},
      m_prefix{std::move(prefix)}
    {}

    void write_progress(std::string_view data)
    {
        m_out << m_prefix << '[' << m_progress << ']' << ' ' << data << '\n';
    }

    void tick()
    {
        m_progress += 10;
    }

private:
    std::ostream m_out;
    std::string m_prefix;
    double m_progress = 0;
};

static_assert(!std::is_copy_constructible_v<ProgressBar>);
static_assert(!std::is_move_constructible_v<ProgressBar>);
```

Since `std::ostream` is not copy constructible and has protected move constructor, class
`ProgressBar` is not copy constructible or move constructible.

Now I want to have a collection of `ProgressBar` objects to indicate progress of different tasks. I
can't create a `std::vector` because `ProgressBar` is not MoveInsertable. So I need to create a
`deque`. So for each task, I create a progress bar. This can be done quite easily with `for` loop:

```cpp
auto tasks = get_tasks(num_of_tasks);
std::deque<ProgressBar> bars;
for (const auto& task: tasks)
{
    bars.emplace_back(std::cout.rdbuf(), task);
}
```

However if you are like me and have seen Sean Parent's C++ seasoning talk<sup>[3]</sup>, you might
cringe a little at the for loops and wonder if we can get rid of the for loop and replace it with
one of the STL algorithms. In this case however, the `for` loop version is actually quite nice, so
the technique I demonstrate further in article is purely for my academic interest.

The most natural algorithm to create one container based on another is `std::transform`. So I went
ahead and refactored above code to:

```cpp
std::transform(cbegin(tasks), cend(tasks), std::back_inserter(bars), [](const auto &task) -> ProgressBar {
    return { std::cout.rdbuf(), task };
});
```

However above code won't compile, because
[std::back_inserter](https://en.cppreference.com/w/cpp/iterator/back_inserter) needs the
[std::deque::push_back](https://en.cppreference.com/w/cpp/container/deque/push_back) operation and
`push_back` needs the type to be
[CopyInsertable](https://en.cppreference.com/w/cpp/named_req/CopyInsertable) or
[MoveInsertable](https://en.cppreference.com/w/cpp/named_req/MoveInsertable). The compiler error
message from clang for this case is quite verbose:

```text
In file included from ../code_samples/emplace_back_iterator.cpp:2:
In file included from /usr/bin/../lib/gcc/x86_64-redhat-linux/13/../../../../include/c++/13/iostream:41:
In file included from /usr/bin/../lib/gcc/x86_64-redhat-linux/13/../../../../include/c++/13/ostream:40:
In file included from /usr/bin/../lib/gcc/x86_64-redhat-linux/13/../../../../include/c++/13/ios:44:
In file included from /usr/bin/../lib/gcc/x86_64-redhat-linux/13/../../../../include/c++/13/bits/ios_base.h:41:
In file included from /usr/bin/../lib/gcc/x86_64-redhat-linux/13/../../../../include/c++/13/bits/locale_classes.h:40:
In file included from /usr/bin/../lib/gcc/x86_64-redhat-linux/13/../../../../include/c++/13/string:43:
In file included from /usr/bin/../lib/gcc/x86_64-redhat-linux/13/../../../../include/c++/13/bits/allocator.h:46:
In file included from /usr/bin/../lib/gcc/x86_64-redhat-linux/13/../../../../include/c++/13/x86_64-redhat-linux/bits/c++allocator.h:33:
/usr/bin/../lib/gcc/x86_64-redhat-linux/13/../../../../include/c++/13/bits/new_allocator.h:191:23: error: call to implicitly-deleted copy constructor of 'ProgressBar'
  191 |         { ::new((void *)__p) _Up(std::forward<_Args>(__args)...); }
      |                              ^   ~~~~~~~~~~~~~~~~~~~~~~~~~~~
/usr/bin/../lib/gcc/x86_64-redhat-linux/13/../../../../include/c++/13/bits/alloc_traits.h:538:8: note: in instantiation of function template specialization 'std::__new_allocator<ProgressBar>::construct<ProgressBar, ProgressBar>' requested here
  538 |           __a.construct(__p, std::forward<_Args>(__args)...);
      |               ^
/usr/bin/../lib/gcc/x86_64-redhat-linux/13/../../../../include/c++/13/bits/deque.tcc:170:21: note: in instantiation of function template specialization 'std::allocator_traits<std::allocator<ProgressBar>>::construct<ProgressBar, ProgressBar>' requested here
  170 |             _Alloc_traits::construct(this->_M_impl,
      |                            ^
/usr/bin/../lib/gcc/x86_64-redhat-linux/13/../../../../include/c++/13/bits/stl_deque.h:1554:9: note: in instantiation of function template specialization 'std::deque<ProgressBar>::emplace_back<ProgressBar>' requested here
 1554 |       { emplace_back(std::move(__x)); }
      |         ^
/usr/bin/../lib/gcc/x86_64-redhat-linux/13/../../../../include/c++/13/bits/stl_iterator.h:747:13: note: in instantiation of member function 'std::deque<ProgressBar>::push_back' requested here
  747 |         container->push_back(std::move(__value));
      |                    ^
/usr/bin/../lib/gcc/x86_64-redhat-linux/13/../../../../include/c++/13/bits/stl_algo.h:4309:12: note: in instantiation of member function 'std::back_insert_iterator<std::deque<ProgressBar>>::operator=' requested here
 4309 |         *__result = __unary_op(*__first);
      |                   ^
../code_samples/emplace_back_iterator.cpp:86:10: note: in instantiation of function template specialization 'std::transform<__gnu_cxx::__normal_iterator<const std::basic_string<char> *, std::vector<std::basic_string<char>>>, std::back_insert_iterator<std::deque<ProgressBar>>, (lambda at ../code_samples/emplace_back_iterator.cpp:86:74)>' requested here
   86 |     std::transform(cbegin(tasks), cend(tasks), std::back_inserter(bars), [](const auto& task) -> ProgressBar {
      |          ^
../code_samples/emplace_back_iterator.cpp:30:18: note: copy constructor of 'ProgressBar' is implicitly deleted because field 'm_out' has a deleted copy constructor
   30 |     std::ostream m_out;
      |                  ^
/usr/bin/../lib/gcc/x86_64-redhat-linux/13/../../../../include/c++/13/ostream:438:7: note: 'basic_ostream' has been explicitly marked deleted here
  438 |       basic_ostream(const basic_ostream&) = delete;
      |       ^
1 error generated.
```

What I need for this case is something like `std::back_emplace_iterator` adaptor that will invoke
`std::deque::emplace_back` and create objects in place instead of requiring copy/move. So lets try
to write a iterator adaptor similar to `std::back_insert_iterator`.

# STL iterator adapters

STL iterator library is quite extensive and provides many [iterator
adaptors](https://en.cppreference.com/w/cpp/iterator#Iterator_adaptors). Refer to the
[back_insert_iterator](https://en.cppreference.com/w/cpp/iterator/back_insert_iterator) reference
page to check its member types and member functions. In case of `base_insert_iterator`, adapted
container's `push_back` method is called whenever the iterator is assigned to, i.e., whenever
`back_insert_iterator::operator=` is invoked.

# Writing iterator adapter for emplace_back

I will define an interface similar to `std::back_insert_iterator` for back_emplace_iterator adapter:

```cpp
template<typename Container>
class back_emplace_iterator
{
public:
    // iterator traits
    using iterator_category = std::output_iterator_tag;
    using value_type = void;
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    using reference = void;
    using container_type = std::remove_cv_t<std::remove_reference_t<Container>>;

    explicit back_emplace_iterator(Container &container)
        : m_container{ std::addressof(container) }
    {}

    back_emplace_iterator &operator*()
    {
        return *this;
    }
    back_emplace_iterator &operator++()
    {
        return *this;
    }
    back_emplace_iterator operator++(int)
    {
        return *this;
    }

    template<typename... Args>
    back_emplace_iterator &operator=(Args &&...args)
    {
        // How to do this?
    }

private:
    Container *m_container;
};

// helper function to create back_emplace_iterator from a container
template<typename Container>
back_emplace_iterator<Container> back_emplacer(Container &c)
{
    return back_emplace_iterator<Container>{ c };
}
```

The big question here is how to define assignment operator `=` for `back_emplace_iterator`. I want
this operator to call `emplace_back` and forward all the input arguments. The assignment operator in
most algorithms will be called like:

```cpp
*d_first = unary_op(*first1);
```

So the unary operation for `std::transform` needs to return arguments required to construct a
`ProgressBar`. It is not possible in C++ for a function to return multiple arguments. The workaround
is to return `std::tuple` type of different objects from the function. So the transform function
will look something like:

```cpp
std::transform(cbegin(tasks), cend(tasks), back_emplacer(bars), [](const auto &task) {
    return std::make_tuple(std::cout.rdbuf(), task);
});
```

The assignment operator of `back_emplace_iterator` needs to take in tuple of arguments as input and
forward them to `emplace_back` function of the Container. Following is my attempt at achieving this:

```cpp
template<typename Tuple>
back_emplace_iterator &operator=(Tuple &&tuple_args)
{
    std::apply([this](auto &&...args) { m_container->emplace_back(std::forward<decltype(args)>(args)...); },
                std::forward<Tuple>(tuple_args));
    return *this;
}
```

In above assignment operator, I accept the tuple of arguments and use `std::apply` to forward the
tuple values to `emplace_back` function. The code uses C++ generic lambdas (introduced in C++14
standard) to define a callable taking in tuple arguments. Using lambda here is quite helpful.
Alternative for C++11 would be to use `std::bind` that binds first argument of
`Container::emplace_back` to `m_container`. 

One improvement to above code is to constraint `Tuple` template type such that:
- `Tuple` is a tuple-like type
- Types contained inside `Tuple` can construct `Container::value_type`. 

The full code is available on [godbolt](https://godbolt.org/z/3a3TPehj8).

One advantage of using `std::transform` in this case instead of for loop is that, if the task is
that we can make use parallel execution policiy from C++17 STL. This makes it very easy to make
trivially parallelizable code run concurrently without additional effort.

# References


1. [STL iterator library](https://en.cppreference.com/w/cpp/iterator)
2. [Using emplace with
   algorithms](https://stackoverflow.com/questions/12129760/using-emplace-with-algorithms-such-as-stdfill)
3. [Sean Parent C++ seasoning talk](https://www.youtube.com/watch?v=W2tWOdzgXHA)
