#include "libstrutil.h"
#include <string.h>
#include <ctype.h>

void strutil_reverse(const char* input, char* output)
{
    if (!input || !output) return;

    int len = (int)strlen(input);
    for (int i = 0; i < len; i++) {
        output[i] = input[len - 1 - i];
    }
    output[len] = '\0';
}

void strutil_uppercase(const char* input, char* output)
{
    if (!input || !output) return;

    /* ASCII-only, and deliberately NOT toupper(): toupper() is LOCALE-DEPENDENT.
       In en_US.UTF-8 — the locale the module actually runs in — it maps 32 of
       the 128 high bytes, including the UTF-8 lead bytes 0xC3/0xE4/0xF0, so it
       corrupts 2-, 3- and 4-byte characters into invalid UTF-8. That made the
       call fail outright (dispatch_failed) for 23 of 503 non-ASCII code points
       tested, and silently changed 7 more. Bytes >= 0x80 are now genuinely
       untouched, in every locale, so a UTF-8 string survives unchanged apart
       from its ASCII letters. */
    int len = (int)strlen(input);
    for (int i = 0; i < len; i++) {
        unsigned char b = (unsigned char)input[i];
        output[i] = (char)((b >= 'a' && b <= 'z') ? (b - 'a' + 'A') : b);
    }
    output[len] = '\0';
}

void strutil_lowercase(const char* input, char* output)
{
    if (!input || !output) return;

    /* ASCII-only — see strutil_uppercase. tolower() was the worse of the two:
       in en_US.UTF-8 it altered 30 of 128 high bytes and returned invalid UTF-8
       for 480 of 503 non-ASCII code points, e.g. lowercaseString("HÉLLO"). */
    int len = (int)strlen(input);
    for (int i = 0; i < len; i++) {
        unsigned char b = (unsigned char)input[i];
        output[i] = (char)((b >= 'A' && b <= 'Z') ? (b - 'A' + 'a') : b);
    }
    output[len] = '\0';
}

int strutil_count_chars(const char* input)
{
    if (!input) return 0;
    return (int)strlen(input);
}

int strutil_count_char(const char* input, char ch)
{
    if (!input) return 0;

    int count = 0;
    for (const char* p = input; *p; p++) {
        if (*p == ch) count++;
    }
    return count;
}

const char* strutil_version(void)
{
    return "1.0.0";
}
