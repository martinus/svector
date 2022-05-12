#include <wordle/IsSingleWordValid.h>
#include <wordle/parseDict.h>
#include <wordle_util.h>

#include <doctest.h>

namespace wordle {

constexpr bool isWordValid(Word const& guessWord, State const& state, Word const& checkWord) {
    auto b = IsSingleWordValid(guessWord, state);
    return b(checkWord);
}

TEST_CASE("val") {
    static_assert(!isWordValid("awake"_word, "00000"_state, "awake"_word));
    static_assert(!isWordValid("awake"_word, "00000"_state, "focal"_word));
    static_assert(isWordValid("awake"_word, "00000"_state, "floss"_word));

    static_assert(isWordValid("abcde"_word, "02200"_state, "xbcxx"_word));
    static_assert(!isWordValid("abcde"_word, "02200"_state, "xcbxx"_word));
    static_assert(isWordValid("abcde"_word, "02200"_state, "bbcbc"_word));
    static_assert(isWordValid("abcde"_word, "02200"_state, "xbcxx"_word));
    static_assert(!isWordValid("abcde"_word, "02200"_state, "xxcxx"_word));
    static_assert(!isWordValid("abcde"_word, "02200"_state, "xbcxa"_word));
    static_assert(isWordValid("abcde"_word, "02200"_state, "xbcxb"_word));
    static_assert(isWordValid("abcde"_word, "02200"_state, "xbcxc"_word)); // there could be another c, we don't know yet
    static_assert(!isWordValid("abcde"_word, "02200"_state, "xbcxd"_word));
    static_assert(!isWordValid("abcde"_word, "02200"_state, "xbcxe"_word));
    static_assert(!isWordValid("abcde"_word, "02200"_state, "ebcxx"_word));

    static_assert(!isWordValid("abcde"_word, "00010"_state, "abcde"_word));

    static_assert(isWordValid("zanza"_word, "01000"_state, "shark"_word));
    static_assert(!isWordValid("zanza"_word, "01000"_state, "quiet"_word));
    static_assert(isWordValid("zanza"_word, "01000"_state, "axxxx"_word));
    static_assert(!isWordValid("zanza"_word, "01000"_state, "xaxxx"_word));
    static_assert(isWordValid("zanza"_word, "01000"_state, "xxaxx"_word));
    static_assert(isWordValid("zanza"_word, "01000"_state, "xxxax"_word));
    static_assert(!isWordValid("zanza"_word, "01000"_state, "xxxxa"_word));
}

} // namespace wordle
