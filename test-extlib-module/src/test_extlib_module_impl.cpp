#include "test_extlib_module_impl.h"

#include <algorithm>
#include <vector>

#include "lib/libstrutil.h"

// std::string here IS the UTF-8 byte sequence, so reaching strutil's
// `const char*` interface costs no conversion (the old Qt plugin round-tripped
// through QString::toUtf8() / fromUtf8() to get to the same bytes).
//
// What DOES need care is that strutil counts and reorders BYTES while this
// module's contract is stated in CHARACTERS (see the header). The helpers
// below are the whole of the difference; each method says what it delegates.

namespace {

// UTF-8 continuation bytes are exactly those with the top bits 10; every other
// byte starts a character. This is the only property of the encoding any of
// the code below relies on, and it holds for every well-formed UTF-8 string.
inline bool isContinuationByte(char c)
{
    return (static_cast<unsigned char>(c) & 0xC0) == 0x80;
}

inline bool startsCharacter(char c)
{
    return !isContinuationByte(c);
}

}  // namespace

std::string TestExtlibModuleImpl::reverseString(const std::string& input)
{
    // Step 1 — the external C library does the reversal, exactly as before.
    std::vector<char> buf(input.size() + 1, '\0');
    strutil_reverse(input.c_str(), buf.data());
    std::string out(buf.data());

    // Step 2 — repair. Reversing bytes reverses the characters (what we want)
    // AND the bytes inside each multi-byte character (what we do not): "é" is
    // C3 A9 going in and comes back out as A9 C3, which is not valid UTF-8 at
    // all. Such a sequence appears in the reversed buffer as a run of
    // continuation bytes followed by its lead byte, so re-reversing each of
    // those runs restores reading order and nothing else moves.
    for (size_t i = 0; i < out.size();) {
        if (!isContinuationByte(out[i])) {
            ++i;
            continue;
        }
        size_t end = i;
        while (end < out.size() && isContinuationByte(out[end])) ++end;
        // out[end] is the lead byte of this character; include it in the flip.
        // (It is missing only if the input was not valid UTF-8, in which case
        // flipping just the continuation run is as good an answer as exists.)
        if (end < out.size()) ++end;
        std::reverse(out.begin() + static_cast<std::ptrdiff_t>(i),
                     out.begin() + static_cast<std::ptrdiff_t>(end));
        i = end;
    }
    return out;
}

std::string TestExtlibModuleImpl::uppercaseString(const std::string& input)
{
    // Entirely strutil's work; ASCII-only by construction (see header).
    std::vector<char> buf(input.size() + 1, '\0');
    strutil_uppercase(input.c_str(), buf.data());
    return std::string(buf.data());
}

std::string TestExtlibModuleImpl::lowercaseString(const std::string& input)
{
    std::vector<char> buf(input.size() + 1, '\0');
    strutil_lowercase(input.c_str(), buf.data());
    return std::string(buf.data());
}

int64_t TestExtlibModuleImpl::countChars(const std::string& input)
{
    // strutil_count_chars is strlen — it answers in BYTES. One character is
    // one lead byte plus its continuation bytes, so discounting the
    // continuation bytes turns that byte count into a character count.
    const int bytes = strutil_count_chars(input.c_str());
    int64_t characters = 0;
    for (int i = 0; i < bytes; ++i) {
        if (startsCharacter(input[static_cast<size_t>(i)])) ++characters;
    }
    return characters;
}

int64_t TestExtlibModuleImpl::countChar(const std::string& input, const std::string& ch)
{
    if (ch.empty()) return 0;

    // Single ASCII needle: strutil_count_char's byte comparison cannot produce
    // a false match, because a byte < 0x80 never appears inside a multi-byte
    // UTF-8 sequence. Keep it on the C library.
    if (ch.size() == 1 && static_cast<unsigned char>(ch[0]) < 0x80) {
        return static_cast<int64_t>(strutil_count_char(input.c_str(), ch[0]));
    }

    // Anything longer: count non-overlapping occurrences of the WHOLE needle,
    // accepting a match only where a character starts. The boundary test is
    // what makes "é" match the character and not the tail of some other one.
    int64_t count = 0;
    for (size_t i = 0; i + ch.size() <= input.size();) {
        if (startsCharacter(input[i]) && input.compare(i, ch.size(), ch) == 0) {
            ++count;
            i += ch.size();
        } else {
            ++i;
        }
    }
    return count;
}

std::string TestExtlibModuleImpl::libVersion()
{
    const char* ver = strutil_version();
    return ver ? std::string(ver) : std::string();
}
