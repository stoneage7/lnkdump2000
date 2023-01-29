
/*****
 * Part of LnkDump2000
 * Licence: GPL, version 3 or later (see COPYING file or https://www.gnu.org/licenses/gpl-3.0.txt)
 *****/

#ifndef __ENCODING_H__
#define __ENCODING_H__

#include <array>
#include <memory>
#include <string>

//! unicode code point
typedef uint32_t codepoint_t;

//! convert from utf16le to utf8.
std::string utf16le_to_utf8(const std::u16string& uni);

//! first value is codepoint at pos, second value is number of bytes taken.
//! second value is 0 if pos >= length of string.
std::pair<codepoint_t, size_t> utf8_codepoint(const std::string_view& s, size_t pos);

//! append codepoint to utf-8 string
void utf8_append(std::string &s, codepoint_t c);

//! code point which replaces invalid coding
const codepoint_t invalid_repl = 0xFFFD;

//! list of {name, definition}
extern const std::array<std::pair<const char*, const void*>, 15> codec_defs;

class Codec
{
private:
    void *p;
    size_t m_index;

public:
    Codec(size_t index);
    Codec(const Codec&) { };
    Codec& operator=(const Codec&) = delete;
    ~Codec();

    size_t index() const { return m_index; }
    std::string string(const std::string &s) const;
};

typedef std::shared_ptr<Codec> CodecPtr;

//! create Codec objects with shared code page data
class CodecFactory
{
private:
    std::array<std::shared_ptr<Codec>, codec_defs.size()> m_managed;

public:
    CodecPtr
    get(size_t index) {
        std::shared_ptr<Codec> m = m_managed[index];
        if (m) {
            return m;
        } else {
            m = std::make_shared<Codec>(index);
            m_managed[index] = m;
            return m;
        }
        return {};
    }

    CodecPtr
    get(const std::string &name) {
        size_t r = -1;
        for (size_t i = 0; i < codec_defs.size(); i++) {
            if (std::string_view(codec_defs[i].first).find(name) == 0) {
                if (r != (size_t)-1) {
                    return {};  // not unique, fail
                } else {
                    r = i;
                }
            }
        }
        return get(r);
    }
};

//! (utility) extract bits between From and To from integer and shift them right.
template<unsigned From, unsigned To, typename T>
inline T get_bits(T n)
{
    static_assert(To >= From);
    static_assert(((T(1) << To) & (~T(0))) != 0);
    T mask = (((T(1) << (To - From)) - T(1)) << T(1) | T(1)) << From;
    T r = (n & mask) >> From;
    return r;
}

#endif // __ENCODING_H__
