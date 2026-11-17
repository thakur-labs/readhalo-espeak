// SPDX-License-Identifier: GPL-3.0-or-later
// Punctuation and output normalization adapted from Phonemizer (GPL-3.0-or-later),
// copyright 2015–2021 Mathieu Bernard. See NOTICE for the pinned source.
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int init(const char *path);
const char *raw(const char *text, const char *accent);

enum { CAPACITY = 65536 };
static char output[CAPACITY];
static int ready;

int rh_espeak_abi(void)
{
    return 1;
}

const char *rh_espeak_version(void)
{
    return "1.52.0-r1";
}

void *rh_espeak_alloc(size_t size)
{
    return malloc(size);
}

void rh_espeak_free(void *pointer)
{
    free(pointer);
}

int rh_espeak_init(void)
{
    if (!ready)
        ready = init("/data");
    return ready > 0 ? 0 : -1;
}

const char *rh_espeak_phonemize_raw(int accent, const char *text)
{
    if (!text || (accent != 0 && accent != 1) || strlen(text) >= CAPACITY || rh_espeak_init() != 0)
        return NULL;
    return raw(text, accent == 1 ? "en-gb" : "en-us");
}

static const char *next_character(const char *text, uint32_t *character)
{
    unsigned char leading = (unsigned char)*text++;
    if (leading < 128) {
        *character = leading;
        return text;
    }
    int count = leading < 0xe0 ? 1 : leading < 0xf0 ? 2 : 3;
    uint32_t value = leading & (0x7f >> (count + 1));
    for (int i = 0; i < count && ((unsigned char)*text & 0xc0) == 0x80; ++i)
        value = (value << 6) | ((unsigned char)*text++ & 0x3f);
    *character = value;
    return text;
}

static int is_space(uint32_t c)
{
    return c == ' ' || (c >= 9 && c <= 13) || (c >= 0x1c && c <= 0x1f) ||
           c == 0x85 || c == 0xa0 || c == 0x1680 || (c >= 0x2000 && c <= 0x200a) ||
           c == 0x2028 || c == 0x2029 || c == 0x202f || c == 0x205f || c == 0x3000;
}

static int is_mark(uint32_t c)
{
    return (c < 128 && strchr(";:,.!?\"(){}[]", (int)c) != NULL) ||
           c == 0xa1 || c == 0xbf || c == 0x2014 || c == 0x2026 ||
           c == 0xab || c == 0xbb || c == 0x201c || c == 0x201d;
}

static const char *trim_start(const char *start, const char *end)
{
    while (start < end) {
        uint32_t c;
        const char *next = next_character(start, &c);
        if (!is_space(c))
            break;
        start = next;
    }
    return start;
}

static const char *trim_end(const char *start, const char *end)
{
    while (end > start) {
        const char *last = end - 1;
        while (last > start && ((unsigned char)*last & 0xc0) == 0x80)
            --last;
        uint32_t c;
        next_character(last, &c);
        if (!is_space(c))
            break;
        end = last;
    }
    return end;
}

static int append(char **at, const char *bytes, size_t length)
{
    if (length >= (size_t)(output + CAPACITY - *at))
        return 0;
    memcpy(*at, bytes, length);
    *at += length;
    return 1;
}

static int append_chunk(int accent, const char *start, const char *end, char **at)
{
    if (start == end)
        return 1;
    size_t length = end - start;
    char *input = malloc(length + 1);
    if (!input)
        return 0;
    memcpy(input, start, length);
    input[length] = 0;
    const char *ipa = rh_espeak_phonemize_raw(accent, input);
    free(input);
    if (!ipa)
        return 0;

    // Phonemizer's eSpeak post-processing folds newlines, pairs of spaces,
    // and stray separators. The tie bar becomes the separator Misaki expects.
    const char *limit = ipa + strlen(ipa);
    ipa = trim_start(ipa, limit);
    limit = trim_end(ipa, limit);
    static char normalized[CAPACITY];
    char *write = normalized;
    while (ipa < limit) {
        uint32_t c;
        const char *next = next_character(ipa, &c);
        if (c == ' ' || c == '\n') {
            *write++ = ' ';
            if (next < limit && (*next == ' ' || *next == '\n'))
                ++next;
        } else if (c == '_') {
            while (next < limit && *next == '_')
                ++next;
            if (next == limit || (*next != ' ' && *next != '\n'))
                *write++ = '_';
        } else if (c == 0x361) {
            *write++ = '^';
        } else {
            memcpy(write, ipa, next - ipa);
            write += next - ipa;
        }
        ipa = next;
    }

    const char *word = normalized;
    for (const char *p = normalized; p <= write; ++p) {
        if (p != write && *p != ' ')
            continue;
        const char *first = trim_start(word, p);
        const char *last = trim_end(first, p);
        if (!append(at, first, last - first) || (p != write && !append(at, " ", 1)))
            return 0;
        word = p + 1;
    }
    return 1;
}

const char *rh_espeak_phonemize(int accent, const char *text)
{
    if (!text || (accent != 0 && accent != 1) || strlen(text) >= CAPACITY)
        return NULL;

    const char *chunk = text;
    const char *p = text;
    char *at = output;
    // A punctuation run contains at least one mark and absorbs adjacent
    // whitespace. Speak the intervening chunks independently, then rejoin.
    while (*p) {
        const char *start = p;
        int has_mark = 0;
        while (*p) {
            uint32_t c;
            const char *next = next_character(p, &c);
            if (!is_space(c) && !is_mark(c))
                break;
            has_mark |= is_mark(c);
            p = next;
        }
        if (p == start) {
            uint32_t c;
            p = next_character(p, &c);
        } else if (has_mark) {
            if (!append_chunk(accent, chunk, start, &at) || !append(&at, start, p - start))
                return NULL;
            chunk = p;
        }
    }
    if (!append_chunk(accent, chunk, p, &at))
        return NULL;
    const char *first = trim_start(output, at);
    const char *last = trim_end(first, at);
    memmove(output, first, last - first);
    output[last - first] = 0;
    return output;
}
