#include <ankerl/svector.h>

#include <chrono>
#include <doctest.h>
#include <fmt/format.h>

#include <vector>

static_assert(sizeof(ankerl::svector<char, 7>) == 8);
static_assert(sizeof(ankerl::svector<uint32_t, 5>) == 24);

TEST_CASE("simple") {
    auto sv = ankerl::svector<char, 7>();
    REQUIRE(sv.size() == 0);
    REQUIRE(sv.capacity() == 7);

    sv.push_back('h');
    REQUIRE(sv.size() == 1);
}

TEST_CASE("push_back") {
    auto sv = ankerl::svector<uint32_t, 5>();
    for (uint32_t i = 0; i < 1000; ++i) {
        REQUIRE(sv.size() == i);
        sv.push_back(i);
        REQUIRE(sv.size() == i + 1);
        REQUIRE(sv.front() == 0);
        REQUIRE(sv.back() == i);

        for (uint32_t j = 0; j < i; ++j) {
            REQUIRE(sv[j] == j);
        }
    }

    uint32_t i = 0;
    for (auto const& val : sv) {
        REQUIRE(val == i);
        ++i;
    }
}

// tests if objects are properly moved
TEST_CASE("iterating_string") {
    auto sv = ankerl::svector<std::string, 7>();
    for (auto i = 0; i < 50; ++i) {
        sv.push_back(std::to_string(i));
    }

    auto i = 0;
    for (auto const& str : sv) {
        REQUIRE(str == std::to_string(i));
        ++i;
    }
}

TEST_CASE("emplace_back") {
    auto sv = ankerl::svector<std::string, 3>();
    sv.emplace_back("hello");
    REQUIRE(sv.size() == 1);
    REQUIRE(sv.front() == "hello");

    sv.clear();
    REQUIRE(sv.empty());
    REQUIRE(sv.size() == 0);
}

TEST_CASE("bench_push_back" * doctest::skip()) {
    auto before = std::chrono::steady_clock::now();
    auto sv = ankerl::svector<uint8_t, 7>();
    // auto sv = std::vector<uint8_t>();
    for (size_t i = 0; i < 10000000; ++i) {
        sv.push_back(static_cast<uint8_t>(i));
    }

    size_t s = 0;
    for (size_t i = 0; i < sv.size(); ++i) {
        s += sv[i];
    }
    REQUIRE(sv.size() == 10000000);
    auto after = std::chrono::steady_clock::now();
    fmt::print("{}s {}\n", std::chrono::duration<double>(after - before).count(), s);
}
