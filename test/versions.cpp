#include <ankerl/svector.h>
#include <app/print.h>

#include <doctest.h>
#include <fmt/format.h>

#include <filesystem>
#include <fstream>
#include <regex>
#include <stdexcept>
#include <string>

namespace {

[[nodiscard]] auto read_file(std::filesystem::path const& filename) -> std::string {
    auto f = std::ifstream(filename);
    if (!f.is_open()) {
        throw std::runtime_error(fmt::format("could not read '{}'", filename.string()));
    }
    return {(std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>()};
}

/**
 * Loads the given file and searches with the regex and checks all matches against the expected match result.
 */
void check(std::filesystem::path const& filename,
           std::string_view reg,
           std::vector<std::string> const& expected_matches,
           size_t num_expected_matches) {
    INFO("checking version in \"" << filename.string() << "\"");
    auto str = read_file(filename);
    REQUIRE(!str.empty());

    auto search_start = str.cbegin();
    auto num_found = size_t();
    auto match = std::smatch();
    auto regex = std::regex(reg.begin(), reg.end());
    while (std::regex_search(search_start, str.cend(), match, regex)) {
        INFO("in \"" << filename.string() << "\" we found \"" << match[0].str() << "\"");

        REQUIRE(match.size() == expected_matches.size() + 1);
        for (size_t i = 0; i < expected_matches.size(); ++i) {
            REQUIRE(match[i + 1].str() == expected_matches[i]);
        }

        ++num_found;
        search_start = match.suffix().first;
    }

    REQUIRE(num_found == num_expected_matches);
}

} // namespace

TEST_CASE("versions") {
    auto const root = std::filesystem::path(__FILE__).parent_path().parent_path();

    auto major_minor_patch = std::vector{std::to_string(ANKERL_SVECTOR_VERSION_MAJOR),
                                         std::to_string(ANKERL_SVECTOR_VERSION_MINOR),
                                         std::to_string(ANKERL_SVECTOR_VERSION_PATCH)};

    check(root / "meson.build", R"(version: '(\d+)\.(\d+)\.(\d+)')", major_minor_patch, 1);
    check(root / "include/ankerl/svector.h", R"(Version (\d+)\.(\d+)\.(\d+)\n)", major_minor_patch, 1);
}
