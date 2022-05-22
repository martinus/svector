#include <ankerl/svector.h>
#include <app/Counter.h>

#include <doctest.h>
#include <fmt/format.h>

#include <stdexcept>

template <class T, size_t N>
class VecTester {
    std::vector<T> m_v{};
    ankerl::svector<T, N> m_s{};

    void assert_eq() {
        if (m_v.size() != m_s.size()) {
            throw std::runtime_error(fmt::format("vec size != svec size: {} != {}", m_v.size(), m_s.size()));
        }
        if (!std::equal(m_v.begin(), m_v.end(), m_s.begin(), m_s.end())) {
            throw std::runtime_error(
                fmt::format("vec content != svec content:\n[{}]\n[{}]", fmt::join(m_v, ","), fmt::join(m_s, ",")));
        }
    }

public:
    template <class... Args>
    void emplace_back(Args&&... args) {
        m_v.emplace_back(std::forward<Args>(args)...);
        m_s.emplace_back(std::forward<Args>(args)...);
        assert_eq();
    }

    template <class... Args>
    void emplace_at(size_t idx, Args&&... args) {
        m_v.emplace(m_v.begin() + idx, std::forward<Args>(args)...);
        m_s.emplace(m_s.cbegin() + idx, std::forward<Args>(args)...);
        assert_eq();
    }

    [[nodiscard]] auto size() const -> size_t {
        return m_v.size();
    }
};

TEST_CASE("emplace") {
    auto counts = Counter();
    INFO(counts);

    auto a = ankerl::svector<Counter::Obj, 3>();

    a.emplace(a.cend(), 123, counts);
    REQUIRE(a.size() == 1);
    REQUIRE(a[0].get() == 123);
}

TEST_CASE("emplace_checked") {
    auto counts = Counter();
    auto vc = VecTester<Counter::Obj, 4>();

    for (size_t s = 0; s < 6; ++s) {
        for (size_t ins = 0; ins < s; ++ins) {
            vc.emplace_back(ins, counts);
        }

        for (size_t i = 0; i <= vc.size(); ++i) {
            auto x = vc;
            x.emplace_at(i, 999, counts);
        }
    }
}
