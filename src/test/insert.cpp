#include <ankerl/svector.h>
#include <app/Counter.h>
#include <app/VecTester.h>

#include <doctest.h>
#include <fmt/format.h>

#include <stdexcept>
#include <string>

TEST_CASE("insert_single") {
    // extremely similar code to "emplace"

    for (size_t s = 0; s < 6; ++s) {
        auto vec_a = ankerl::svector<std::string, 4>();
        auto vec_b = std::vector<std::string>();
        for (size_t ins = 0; ins < s; ++ins) {
            vec_a.emplace_back(std::to_string(ins));
            vec_b.emplace_back(std::to_string(ins));
        }

        for (size_t i = 0; i <= vec_a.size(); ++i) {
            auto va = vec_a;
            auto vb = vec_b;

            // moved
            auto ita = va.insert(va.cbegin() + i, "999");
            auto itb = vb.insert(vb.cbegin() + i, "999");
            REQUIRE(*ita == *itb);
            *ita += 'x';
            *itb += 'x';

            // constexpr => copied
            std::string x = "asdf";
            ita = va.insert(va.cbegin() + i, x);
            itb = vb.insert(vb.cbegin() + i, x);
            REQUIRE(*ita == x);
            *ita += 'g';
            *itb += 'g';

            assert_eq(va, vb);
        }
    }
}
