#include <fmt/format.h>

#include <cstdio>

template <typename S, typename... Args>
void print(S const& format, Args&&... args) {
    fmt::print(format, std::forward<Args>(args)...);
    std::fflush(stdout);
}
