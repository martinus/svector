#include <ankerl/svector.h>

#include <cstdio>

// Deliberately shallow: this checks that the CMake target carries the include directory and
// the C++17 requirement, not that svector behaves. The meson suite owns behaviour.
auto main() -> int {
    auto v = ankerl::svector<int, 7>();
    for (int i = 0; i < 100; ++i) {
        v.push_back(i);
    }
    if (v.size() != 100 || v.back() != 99) {
        return 1;
    }

    std::printf("consumed svector %d.%d.%d through add_subdirectory\n",
                ANKERL_SVECTOR_VERSION_MAJOR,
                ANKERL_SVECTOR_VERSION_MINOR,
                ANKERL_SVECTOR_VERSION_PATCH);
    return 0;
}
