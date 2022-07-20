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
                // assertEq(sv, ref);
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
                // assertEq(sv, ref);
            },
            [&] {
                auto tmp = Svector();
                sv = tmp;
                auto tmp_ref = Ref();
                ref = tmp_ref;
                counts.check_all_done();
                // assertEq(sv, ref);
            },
            [&] {
                sv = Svector();
                ref = Ref();
                counts.check_all_done();
                // assertEq(sv, ref);
            },
            [&] {
                std::destroy_at(&sv);
                std::destroy_at(&ref);
                counts.check_all_done();

                auto ary = std::array{Counter::Obj{1, counts}, Counter::Obj{2, counts}, Counter::Obj{3, counts}};
                new (&sv) Svector(ary.begin(), ary.end());
                new (&ref) Ref(ary.begin(), ary.end());
                // assertEq(sv, ref);
            },
            [&] {
                auto count = p.bounded(10);
                sv.assign(count, Counter::Obj{1234, counts});
                ref.assign(count, Counter::Obj{1234, counts});
                // assertEq(sv, ref);
            },
            [&] {
                auto count = p.bounded(10);
                sv.assign(count, Counter::Obj(312, counts));
                ref.assign(count, Counter::Obj(312, counts));
                // assertEq(sv, ref);
            },
            [&] {
                auto count = p.bounded(10);
                sv.resize(count);
                ref.resize(count);
                for (auto& c : sv) {
                    c.set(counts);
                }
                for (auto& c : ref) {
                    c.set(counts);
                }
                // assertEq(sv, ref);
            },
            [&] {
                auto count = p.bounded(10);
                sv.resize(count, Counter::Obj{555, counts});
                ref.resize(count, Counter::Obj{555, counts});
                // assertEq(sv, ref);
            },
            [&] {
                auto count = p.bounded(10);
                sv.reserve(count);
                ref.reserve(count);
                // assertEq(sv, ref);
            },
            [&] {
                sv.emplace_back(val, counts);
                ref.emplace_back(val, counts);
                ++val;
                // assertEq(sv, ref);
            },
            [&] {
                auto it = sv.rbegin();
                auto it_ref = ref.rbegin();
                while (it != sv.rend()) {
                    REQUIRE(*it == *it_ref);
                    ++it;
                    ++it_ref;
                }
            },
            [&] {
                if (!sv.empty()) {
                    REQUIRE(sv.front() == ref.front());
                    REQUIRE(sv.back() == ref.back());
                }
            },
            [&] {
                REQUIRE(sv.empty() == ref.empty());
                sv.clear();
                ref.clear();
                REQUIRE(sv.empty() == ref.empty());
                counts.check_all_done();
                // assertEq(sv, ref);
            },
            [&] {
                if (!sv.empty()) {
                    auto pos = p.bounded(sv.size());
                    REQUIRE(sv[pos] == ref[pos]);
                }
            },
            [&] {
                if (!sv.empty()) {
                    auto idx = p.bounded(sv.size());
                    auto it_idx = sv.erase(sv.begin() + idx);
                    auto it_ref = ref.erase(ref.begin() + idx);
                    REQUIRE(std::distance(it_idx, sv.end()) == std::distance(it_ref, ref.end()));
                    // assertEq(sv, ref);
                }
            },
            [&] {
                if (!sv.empty()) {
                    sv.pop_back();
                    ref.pop_back();
                    // assertEq(sv, ref);
                }
            },
            [&] {
                sv.shrink_to_fit();
                ref.shrink_to_fit();
                // assertEq(sv, ref);
            },
            [&] {
                auto pos = p.bounded(sv.size());
                sv.insert(sv.cbegin() + pos, Counter::Obj{987, counts});
                ref.insert(ref.cbegin() + pos, Counter::Obj{987, counts});
                // assertEq(sv, ref);
            },
            [&] {
                auto pos = p.bounded(sv.size());
                auto count = p.bounded(10);
                sv.insert(sv.cbegin() + pos, count, Counter::Obj{987, counts});
                ref.insert(ref.cbegin() + pos, count, Counter::Obj{987, counts});
                // assertEq(sv, ref);
            },
            [&] {
                auto pos = p.bounded(sv.size());
                auto ary = std::array{Counter::Obj{17, counts}, Counter::Obj{27, counts}, Counter::Obj{37, counts}};
                sv.insert(sv.cbegin() + pos, ary.begin(), ary.end());
                ref.insert(ref.cbegin() + pos, ary.begin(), ary.end());
                // assertEq(sv, ref);
            },
            [&] {
                if (!sv.empty()) {
                    auto pos = p.bounded(sv.size());
                    auto it = sv.erase(sv.begin() + pos);
                    auto it_ref = ref.erase(ref.begin() + pos);
                    REQUIRE(std::distance(it, sv.end()) == std::distance(it_ref, ref.end()));
                    // assertEq(sv, ref);
                }
            },
            [&] {
                if (!sv.empty()) {
                    auto pos1 = p.bounded(sv.size());
                    auto pos2 = p.bounded(sv.size());
                    if (pos1 > pos2) {
                        std::swap(pos1, pos2);
                    }
                    auto it = sv.erase(sv.begin() + pos1, sv.begin() + pos2);
                    auto it_ref = ref.erase(ref.begin() + pos1, ref.begin() + pos2);
                    REQUIRE(std::distance(it, sv.end()) == std::distance(it_ref, ref.end()));
                    // assertEq(sv, ref);
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
