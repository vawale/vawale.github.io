#include <algorithm>
#include <cstddef>
#include <deque>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

class ProgressBar
{
public:
    ProgressBar(std::streambuf *buffer, std::string prefix)
        : m_out{ buffer }
        , m_prefix{ std::move(prefix) }
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

    template<typename Tuple>
    back_emplace_iterator &operator=(Tuple &&tuple_args)
    {
        std::apply([this](auto &&...args) { m_container->emplace_back(std::forward<decltype(args)>(args)...); },
                   std::forward<Tuple>(tuple_args));
        return *this;
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

using Tasks = std::vector<std::string>;
Tasks get_tasks(size_t num_of_tasks)
{
    Tasks tasks;
    tasks.reserve(num_of_tasks);
    std::generate_n(std::back_inserter(tasks), num_of_tasks, [i = 0]() mutable {
        return "task" + std::to_string(i++);
    });
    return tasks;
}

void log_progress(std::deque<ProgressBar> &bars)
{
    std::for_each(begin(bars), end(bars), [](auto &bar) {
        bar.tick();
        bar.write_progress("step");
    });
}

void do_tasks_for_loop(size_t num_of_tasks)
{
    auto tasks = get_tasks(num_of_tasks);

    std::deque<ProgressBar> bars;
    for(const auto &task : tasks)
    {
        bars.emplace_back(std::cout.rdbuf(), task);
    }

    log_progress(bars);
}

void do_tasks_algorithm(size_t num_of_tasks)
{
    auto tasks = get_tasks(num_of_tasks);
    std::deque<ProgressBar> bars;
    std::transform(cbegin(tasks), cend(tasks), back_emplacer(bars), [](const auto &task) {
        return std::make_tuple(std::cout.rdbuf(), task);
    });
    log_progress(bars);
}

int main()
{
    do_tasks_for_loop(5);
    do_tasks_algorithm(5);
    return 0;
}
