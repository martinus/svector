#pragma once

#include <ankerl/svector.h>

#include <doctest.h>
#include <fmt/format.h>
#include <fmt/ranges.h> // fmt::join

#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

/**
 * @brief count distinct strings, each long enough that it really allocates.
 *
 * Some bugs only show up on a type whose move assignment does something. std::string qualifies,
 * but only while it is too long for the small string optimization, otherwise a self-move leaves
 * the bytes in place and the damage is invisible. See issue #67.
 */
inline auto make_long_strings(size_t count) -> std::vector<std::string> {
    auto v = std::vector<std::string>();
    v.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        v.emplace_back(40, static_cast<char>('a' + (i % 26)));
    }
    return v;
}

template <typename VecA, typename VecB>
void assert_eq(VecA const& a, VecB const& b) {
    if (a.size() != b.size()) {
        throw std::runtime_error(fmt::format("vec size != svec size: {} != {}", a.size(), b.size()));
    }
    if (!std::equal(a.begin(), a.end(), b.begin(), b.end())) {
        throw std::runtime_error(
            fmt::format("vec content != svec content:\n[{}]\n[{}]", fmt::join(a, ","), fmt::join(b, ",")));
    }
}

template <class T, size_t N>
class VecTester {
    std::vector<T> m_v{};
    ankerl::svector<T, N> m_s{};

public:
    template <class... Args>
    void emplace_back(Args&&... args) {
        m_v.emplace_back(std::forward<Args>(args)...);
        m_s.emplace_back(std::forward<Args>(args)...);
        assert_eq(m_v, m_s);
    }

    template <class... Args>
    void emplace_at(size_t idx, Args&&... args) {
        auto it_v = m_v.emplace(m_v.begin() + idx, std::forward<Args>(args)...);
        auto it_s = m_s.emplace(m_s.cbegin() + idx, std::forward<Args>(args)...);
        REQUIRE(*it_v == *it_s);
        assert_eq(m_v, m_s);
    }

    [[nodiscard]] auto size() const -> size_t {
        return m_v.size();
    }
};
