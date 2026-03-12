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

    int len = (int)strlen(input);
    for (int i = 0; i < len; i++) {
        output[i] = (char)toupper((unsigned char)input[i]);
    }
    output[len] = '\0';
}

void strutil_lowercase(const char* input, char* output)
{
    if (!input || !output) return;

    int len = (int)strlen(input);
    for (int i = 0; i < len; i++) {
        output[i] = (char)tolower((unsigned char)input[i]);
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
