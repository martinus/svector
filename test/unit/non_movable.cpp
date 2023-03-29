#include <ankerl/svector.h>
#include <app/Counter.h>
#include <app/VecTester.h>

#include <doctest.h>

#include <vector>

class non_movable {
    int m_i;

public:
    explicit non_movable(int i)
        : m_i(i) {}

    non_movable(non_movable const&) = default;
    auto operator=(non_movable const&) -> non_movable& = default;
    non_movable(non_movable&&) = delete;
    auto operator=(non_movable&&) -> non_movable& = delete;
    ~non_movable() = default;

    [[nodiscard]] auto i() const noexcept -> int {
        return m_i;
    }
};

TEST_CASE("emplace_non_movable") {
    auto a = ankerl::svector<non_movable, 3>();
    for (int i = 0; i < 100; ++i) {
        a.emplace_back(i);
    }

    for (int i = 0; i < 100; ++i) {
        REQUIRE(a[i].i() == i);
    }
}
