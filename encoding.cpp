
/*****
 * Part of LnkDump2000
 * Licence: GPL, version 3 or later (see COPYING file or https://www.gnu.org/licenses/gpl-3.0.txt)
 *****/

#include "encoding.h"

//! append codepoint to an utf-8 string
void
utf8_append(std::string &s, codepoint_t c)
{
    if (c <= 0x7F) {
        // 7 bit
        s.push_back(char(c));
    } else if (c <= 0x7FF) {
        // 11 bit = 5 + 6 bits
        char c1 = get_bits<6, 10>(c) | 0b11000000;
        char c2 = get_bits<0,  5>(c) | 0b10000000;
        s.push_back(c1);
        s.push_back(c2);
    } else if (c <= 0xFFFF) {
        // 16 bit = 4 + 6 + 6 bits
        char c1 = get_bits<12, 15>(c) | 0b11100000;
        char c2 = get_bits< 6, 11>(c) | 0b10000000; 
        char c3 = get_bits< 0,  5>(c) | 0b10000000;
        s.push_back(c1);
        s.push_back(c2);
        s.push_back(c3);
    } else if (c >= 0x10000 && c <= 0x10FFFF) {
        // 21 bit = 3 + 6 + 6 + 6 bits
        char c1 = get_bits<18, 20>(c) | 0b11110000;
        char c2 = get_bits<12, 17>(c) | 0b10000000;
        char c3 = get_bits< 6, 11>(c) | 0b10000000;
        char c4 = get_bits< 0,  5>(c) | 0b10000000;
        s.push_back(c1);
        s.push_back(c2);
        s.push_back(c3);
        s.push_back(c4);
    } else {
        utf8_append(s, invalid_repl);
    }
}

//! convert from utf16le to utf8.
std::string
utf16le_to_utf8(const std::u16string& uni)
{
    std::string r;
    size_t i = 0;
    const auto len = uni.length();
    while (i < len) {
        uint16_t c1 = uni.at(i);
        i++;
        if (c1 <= 0xD7FF || (c1 >= 0xE000)) {
            utf8_append(r, c1);
        } else if (c1 >= 0xDC00 && c1 <= 0xDFFF) {
            // unpaired low surrogate - replace and skip
            utf8_append(r, invalid_repl);
        } else if (c1 >= 0xD800 && c1 <= 0xDBFF) {
            // high surrogate
            if (i < len) {
                uint16_t c2 = uni.at(i);
                i++;
                if (c2 >= 0xDC00 && c2 >= 0xDFFF) {
                    // paired high surrogate
                    codepoint_t c = ((c1 - 0xD800) << 10) + (c2 - 0xDC00) + 0x10000;
                    utf8_append(r, c);
                } else {
                    // unpaired high surrogate
                    utf8_append(r, invalid_repl);
                    i--;
                }
            } else {
                // unpaired high surrogate (end of string)
                utf8_append(r, invalid_repl);
            }
        }
    }
    return r;
}

//! first value is codepoint at pos, second value is number of bytes taken.
//! second value is 0 if pos >= length of string.
std::pair<codepoint_t, size_t>
utf8_codepoint(const std::string_view& s, size_t pos)
{
    const size_t slen = s.length();
    if (pos >= slen) {
        return std::make_pair(invalid_repl, 0);
    } else {
        uint8_t c = s.at(pos);
        const size_t nb = (c <= 0b01111111 ? 1 :
            (c <= 0b11011111 ? 2 : (c <= 0b11101111 ? 3 : (c <= 0b11110111 ? 4 : 0))));
        if (nb == 0) {
            // invalid first character
            return std::make_pair(invalid_repl, 1);
        } else if (slen - pos < nb) {
            // incomplete code due to string length, eat rest of the string
            return std::make_pair(invalid_repl, slen - pos);
        } else {
            // decode first byte, nb is 1..4
            codepoint_t r = c & (nb == 1 ? 0xFF : (nb == 2 ? 0b11111 : (nb == 3 ? 0b1111 : 0b111)));
            // decode bytes 2..4
            pos++;
            for (size_t i = 2; i <= nb; i++, pos++) {
                uint8_t d = s.at(pos);
                if (get_bits<6, 7>(d) == 0b10) {
                    // ok, bytes begin with 10
                    r <<= 6;
                    r |= get_bits<0, 5>(d);
                } else {
                    // error, eat string up to previous character
                    return std::make_pair(invalid_repl, i - 1);
                }
            }
            // ok, return the codepoint
            return std::make_pair(r, nb);
        }
    }
}

struct DoublesDef
{
    uint8_t                     leadingByte;
    uint8_t                     trailingStart;
    size_t                      length;
    const uint16_t*             data;
};

struct CodecDef
{
    const uint16_t*             singlesMap;
    const DoublesDef*           doublesMap;
    size_t                      doublesLength;
};

// Codepage definitions
#include "enc_single.inc"
#include "enc_asian.inc"

constexpr std::array<std::pair<const char*, const void*>, 15> codec_defs {{
    {"874 - Thai", &cp874},
    {"932 - Japanese (Shift-JIS)", &cp932}, // mb
    {"936 - Chinese Simplified (GBK)", &cp936}, // mb
    {"949 - Korean (Hangul)", &cp949}, // mb
    {"950 - Chinese (Big5)", &cp950}, // mb
    {"1250 - Eastern European", &cp1250},
    {"1251 - Cyrillic", &cp1251},
    {"1252 - Latin 1", &cp1252},
    {"1253 - Greek", &cp1253},
    {"1254 - Turkish", &cp1254},
    {"1255 - Hebrew", &cp1255},
    {"1256 - Arabic", &cp1256},
    {"1257 - Baltic", &cp1257},
    {"1258 - Vietnam", &cp1258},
    {"1361 - Korean (Johab)", &cp1361} // mb
}};

// Codecs
class CodecImpl
{
private:
    const uint16_t*                     m_singles;
    std::array<const DoublesDef*, 256>  m_doubles;
public:
    CodecImpl(size_t index)
    {
        const CodecDef *def = (CodecDef*)codec_defs[index].second;
        m_singles = def->singlesMap;
        std::fill(m_doubles.begin(), m_doubles.end(), nullptr);
        for (size_t i = 0; i < def->doublesLength; i++) {
            uint8_t leading = def->doublesMap[i].leadingByte;
            m_doubles[leading] = &def->doublesMap[i];
        }
    }

    std::pair<codepoint_t, size_t>
    decode_char(const std::string& s, size_t pos)
    {
        uint8_t c1 = s.at(pos);
        if (m_doubles[c1] != nullptr) {
            if ((pos + 1) < s.length()) {
                uint8_t c2 = s.at(pos + 1);
                const DoublesDef *def = m_doubles[c1];
                if (def && c2 >= def->trailingStart && c2 < def->trailingStart + def->length) {
                    auto d = def->data[c2 - def->trailingStart];
                    if (d != 0) {
                        return std::pair<codepoint_t, size_t>(d, 2);
                    } else {
                        return std::pair<codepoint_t, size_t>(invalid_repl, 2);
                    }
                } else {
                    return std::pair<codepoint_t, size_t>(invalid_repl, 2);
                }
            } else {
                return std::pair<codepoint_t, size_t>(invalid_repl, 1);
            }
        } else {
            auto d = m_singles[c1];
            if (d != 0) {
                return std::pair<codepoint_t, size_t>(d, 1);
            } else {
                return std::pair<codepoint_t, size_t>(invalid_repl, 1);
            }
        }
    }

    std::string
    decode_string(const std::string &s)
    {
        size_t pos = 0;
        std::string r;
        while (pos < s.length()) {
            auto [cp, l] = decode_char(s, pos);
            if (cp <= 0xD7FF || (cp >= 0xE000)) {
                utf8_append(r, cp);
            } else {
                utf8_append(r, invalid_repl);
            }
            pos += l;
        }
        return r;
    }
};

Codec::Codec(size_t index)
{
    p = new CodecImpl(index);
    m_index = index;
}

Codec::~Codec()
{
    CodecImpl *i = (CodecImpl*)p;
    delete i;
}

std::string
Codec::string(const std::string& s) const
{
    CodecImpl *i = (CodecImpl*)p;
    std::string r = i->decode_string(s);
    return r;
}


