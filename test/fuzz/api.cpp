#include <ankerl/svector.h>
#include <app/Counter.h>

#include "Provider.h"

#if defined(FUZZ)
#    define REQUIRE(x) ::fuzz::Provider::require(x)
#else
#    include <doctest.h>
#endif

#include <initializer_list> // for initializer_list
#include <iterator>         // for distance, __iterator_traits<>::d...
#include <new>              // for operator new
#include <tuple>            // for forward_as_tuple
#include <utility>          // for swap, pair, piecewise_construct
#include <vector>           // for vector

namespace fuzz {

template <typename A, typename B>
inline void assertEq(A const& a, B const& b) {
    REQUIRE(a.size() == b.size());
    for (size_t i = 0; i < a.size(); ++i) {
        REQUIRE(a[i] == b[i]);
    }
}

void api(uint8_t const* data, size_t size) {
    auto p = fuzz::Provider(data, size);
    Counter counts;

    {
        using Svector = ankerl::svector<Counter::Obj, 5>;
        using Ref = std::vector<Counter::Obj>;
        auto sv = Svector();
        auto ref = Ref();
        auto val = size_t();
        p.repeat_oneof(
            [&] {
                std::destroy_at(&sv);
                std::destroy_at(&ref);
                counts.check_all_done();

                new (&sv) Svector();
                new (&ref) Ref();
                assertEq(sv, ref);
            },
            [&] {
                std::destroy_at(&sv);
                std::destroy_at(&ref);
                counts.check_all_done();

                auto count = p.bounded(20);
                new (&sv) Svector(count);
                new (&ref) Ref(count);
                for (auto& c : sv) {
                    c.set(counts);
                }
                for (auto& c : ref) {
                    c.set(counts);
                }
                assertEq(sv, ref);
            },
            [&] {
                auto tmp = Svector();
                sv = tmp;
                auto tmp_ref = Ref();
                ref = tmp_ref;
                counts.check_all_done();
                assertEq(sv, ref);
            },
            [&] {
                sv = Svector();
                ref = Ref();
                counts.check_all_done();
                assertEq(sv, ref);
            },
            [&] {
                auto count = p.bounded(10);
                sv.assign(count, Counter::Obj(312, counts));
                ref.assign(count, Counter::Obj(312, counts));
                assertEq(sv, ref);
            },
            [&] {
                std::destroy_at(&sv);
                std::destroy_at(&ref);
                counts.check_all_done();

                auto ary = std::array{Counter::Obj{1, counts}, Counter::Obj{2, counts}, Counter::Obj{3, counts}};
                new (&sv) Svector(ary.begin(), ary.end());
                new (&ref) Ref(ary.begin(), ary.end());
                assertEq(sv, ref);
            },
            [&] {
                sv.emplace_back(val, counts);
                ref.emplace_back(val, counts);
                assertEq(sv, ref);
            },
            [&] {
                if (!sv.empty()) {
                    auto idx = p.bounded(sv.size());
                    auto it_idx = sv.erase(sv.begin() + idx);
                    auto it_ref = ref.erase(ref.begin() + idx);
                    REQUIRE(std::distance(it_idx, sv.end()) == std::distance(it_ref, ref.end()));
                    assertEq(sv, ref);
                }
            },
            [&] {
                if (!sv.empty()) {
                    sv.pop_back();
                    ref.pop_back();
                    assertEq(sv, ref);
                }
            });
        assertEq(sv, ref);
    }
}

} // namespace fuzz

#if defined(FUZZ)
extern "C" auto LLVMFuzzerTestOneInput(uint8_t const* data, size_t size) -> int {
    fuzz::api(data, size);
    return 0;
}
#endif
