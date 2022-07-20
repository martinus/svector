#include <ankerl/svector.h>

#include <doctest.h>

using namespace std::literals;

TEST_CASE("assign_size_value") {
    auto sv = ankerl::svector<char, 7>();
    sv.push_back('x');

    sv.assign(1000, 'a');
    REQUIRE(sv.size() == 1000);
    for (auto c : sv) {
        REQUIRE(c == 'a');
    }
}

TEST_CASE("assign_iterators") {
    auto str = "hello world!"sv;

    auto sv = ankerl::svector<char, 7>(100, 'x');
    sv.assign(str.begin(), str.end());

    REQUIRE(sv.size() == str.size());
    REQUIRE(str == std::string_view(sv.data(), sv.size()));
}

TEST_CASE("assign_initializer_list") {
    auto sv = ankerl::svector<char, 7>(100, 'x');
    sv.assign({'a', 'b', 'c'});
    REQUIRE(sv.size() == 3);
    REQUIRE(sv[0] == 'a');
    REQUIRE(sv[1] == 'b');
    REQUIRE(sv[2] == 'c');
}