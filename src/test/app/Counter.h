#pragma once

#include <cstddef>
#include <functional>
#include <string_view>

class CountObj;

struct Counter {
    // Obj for only swaps & equals. Used for optimizing.
    // Can't use static counters here because I want to do it in parallel.
    class Obj {
    public:
        // required for operator[]
        Obj();
        Obj(const size_t& data, Counter& counts);
        Obj(const Obj& o);
        Obj(Obj&& o) noexcept;
        ~Obj();

        auto operator==(const Obj& o) const -> bool;
        auto operator<(const Obj& o) const -> bool;
        auto operator=(const Obj& o) -> Obj&;
        auto operator=(Obj&& o) noexcept -> Obj&;

        [[nodiscard]] auto get() const -> size_t const&;
        auto get() -> size_t&;

        void swap(Obj& other);
        [[nodiscard]] auto getForHash() const -> size_t;

    private:
        size_t mData;
        Counter* mCounts;
    };

    Counter();

    void reset();

    size_t ctor{};
    size_t defaultCtor{};
    size_t copyCtor{};
    size_t dtor{};
    size_t equals{};
    size_t less{};
    size_t assign{};
    size_t swaps{};
    size_t get{};
    size_t constGet{};
    size_t hash{};
    size_t moveCtor{};
    size_t moveAssign{};

    static void printHeaderOnce();
    void printCounts(std::string_view title) const;

    static size_t staticDefaultCtor;
    static size_t staticDtor;
};

namespace std {

template <>
struct hash<Counter::Obj> {
    [[nodiscard]] auto operator()(const Counter::Obj& c) const noexcept -> size_t {
        return hash<size_t>{}(c.getForHash());
    }
};

} // namespace std
