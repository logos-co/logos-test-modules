#include "test_extlib_module_impl.h"

#include <vector>

#include "lib/libstrutil.h"

// The old Qt plugin round-tripped through QString::toUtf8() / fromUtf8() to
// reach the C library's `const char*` interface. std::string already IS the
// UTF-8 byte sequence, so the conversion disappears; the bytes handed to
// strutil are byte-for-byte the same ones as before.

std::string TestExtlibModuleImpl::reverseString(const std::string& input)
{
    std::vector<char> buf(input.size() + 1, '\0');
    strutil_reverse(input.c_str(), buf.data());
    return std::string(buf.data());
}

std::string TestExtlibModuleImpl::uppercaseString(const std::string& input)
{
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
    return static_cast<int64_t>(strutil_count_chars(input.c_str()));
}

int64_t TestExtlibModuleImpl::countChar(const std::string& input, const std::string& ch)
{
    // Same degenerate-input behaviour as the Qt version: an empty `ch`
    // becomes NUL, which strutil_count_char never finds in a NUL-terminated
    // string, so the answer is 0 rather than a crash.
    const char c = ch.empty() ? '\0' : ch[0];
    return static_cast<int64_t>(strutil_count_char(input.c_str(), c));
}

std::string TestExtlibModuleImpl::libVersion()
{
    const char* ver = strutil_version();
    return ver ? std::string(ver) : std::string();
}
