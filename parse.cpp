
/*****
 * Part of LnkDump2000
 * Licence: GPL, version 3 or later (see COPYING file or https://www.gnu.org/licenses/gpl-3.0.txt)
 *****/

#include "encoding.h"
#include "parse.h"
#include <array>
#include <list>
#include <string>
#include <vector>
#include <fstream>
#include <cstdarg>
#include <limits>
#include <iostream>

namespace LnkParser {

const size_t MAX_FILE_SIZE = 1024L * 1024;

Error
Error::format(const char *fmt, ...)
{
    va_list argp;
    va_start(argp, fmt);
    char msg[1024] = "";
    vsnprintf(msg, sizeof(msg), fmt, argp);
    Error rv(msg);
    va_end(argp);
    return rv;
}

//! check if (a+b) will give a different result than if both args were converted to a wider int
template <typename T, typename U>
static bool add_overflows(T a, U b)
{
    decltype(a+b) xa(a);
    decltype(a+b) xb(b);
    if (a > 0 && b > 0) {
        // signedness does not matter here, xa == a && xb == b, but (a+b) can be of wider type
        // result of addition will be > 0, so check if result does not overflow type of (a+b)
        return (xb > std::numeric_limits<decltype(a+b)>::max() - xa);
    } else if (a < 0 && b < 0) {
        // both integers are signed and type of (a+b) is also signed
        // result of addition will be < 0, so check if result does not underflow type of (a+b)
        return (xb < std::numeric_limits<decltype(a+b)>::min() - xa);
    } else if constexpr (std::is_integral_v<decltype(a)>) {
        if (a < 0 && xa > 0 && b >= 0) {
            // signed a is being converted to unsigned, because unsigned b is wider
            // check that (a+b) does not underflow at 0
            decltype(a) ya = -a;
            return (decltype(a+b))b < ya;
        }
    } else if constexpr (std::is_integral_v<decltype(b)>) {
        if (b < 0 && xb > 0 && a >= 0) {
            // same thing, other way around
            decltype(b) yb = -b;
            return (decltype(a+b))a < yb;
        }
    }
    return false;
}

//! how many bytes in a unicode string if it NUL-terminated
static size_t u16s0_nbytes(const std::u16string& s)
{
    return (s.size() + 1) * 2;
}

//! how many unicode chars in bytes, including numeric_limits
static size_t u16_nchars(size_t bytes)
{
    return bytes / 2;
}

// the file is comprised of little endian numeric fields and strings
// read the whole file and implement a basic API for reading relevant types
class FileStream
{
private:
    std::vector<char>       m_buffer;
    size_t                  m_pos;

private:
    void int_overflow()
    {
        throw Error::format("Integer overflow while reading stream.");
    }
public:
    FileStream (const std::string& filename)
    {
        std::ifstream in;
        in.open(filename, std::ios::in | std::ios::binary);
        in.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        auto begin = std::istreambuf_iterator<char>(in);
        auto end = std::istreambuf_iterator<char>();
        for (size_t counter = 0; begin != end && counter < MAX_FILE_SIZE; begin++, counter++) {
            m_buffer.push_back(*begin);
        }
        in.close();
        m_pos = 0;
    }
    bool is_eof() const
    {
        return m_buffer.size() <= 0 || m_pos >= m_buffer.size() - 1;
    }
    char getc()
    {
        if (add_overflows(m_pos, 1)) {
            int_overflow();
        }
        return m_buffer.at(m_pos++);
    }
    char peek() const
    {
        return m_buffer.at(m_pos);
    }
    void ignore(size_t len)
    {
        if (add_overflows(m_pos, len)) {
            int_overflow();
        }
        m_pos += len;
    }
    void seekg(size_t n)
    {
        m_pos = n;
    }
    size_t tellg() const
    {
        return m_pos;
    }
    //! always gets n bytes, or crashes
    void read(char* buf, size_t n)
    {
        for (size_t i = 0; i < n; i++) {
            buf[i] = getc();
        }
    }
    void operator >>(uint8_t & i)
    {
        i = getc();
    }
    void operator >>(uint16_t& i)
    {
        unsigned char tmp[2];
        read((char*)tmp, sizeof(tmp));
        i = uint16_t(tmp[0]) | uint16_t(tmp[1]) << 8;
    }
    void operator >>(uint32_t& i)
    {
        unsigned char tmp[4];
        read((char*)tmp, sizeof(tmp));
        i = uint32_t(tmp[0]) | uint32_t(tmp[1]) << 8
            | uint32_t(tmp[2]) << 16 | uint32_t(tmp[3]) << 24;
    }
    void operator >>(uint64_t& i)
    {
        uint32_t tmp[2];
        this->operator>>(tmp[0]);
        this->operator>>(tmp[1]);
        i = uint64_t(tmp[0]) | uint64_t(tmp[1]) << 32;
    }
    void operator >>(int16_t & i)
    {
        uint16_t tmp;
        this->operator>>(tmp);
        i = (tmp < 0x8000) ? ((int16_t)tmp) : -((int16_t)((~tmp)+1));
    }
    void operator >>(LnkStruct::Guid &i)
    {
        uint8_t tmp[16];
        read((char*)tmp, sizeof(tmp));
        i = LnkStruct::Guid(std::to_array(tmp));
    }
    std::string read_ansi(size_t max)
    {
        // read NUL-terminated string
        std::string r;
        r.reserve(16);
        for (size_t i = 0; i < max; i++) {
            char c = getc();
            if (c == 0) {
                return r;
            }
            r.push_back(c);
        }
        return r;
    }
    //! reads at most 'max' number of 16bit characters, including NUL
    std::u16string read_unicode(size_t max)
    {
        // .lnk uses UTF16 for unicode
        std::u16string r;
        for (size_t i = 0; i < max; i++) {
            uint16_t c;
            this->operator>>(c);
            if (c == 0) {
                return r;
            }
            r.push_back(c);
        }
        return r;
    }
    //! reads exactly 'len' number of bytes
    std::string read_exact(size_t len)
    {
        // strings that are exact number of bytes in the format, but cut off on \0
        size_t pos = tellg();
        std::string r = read_ansi(len);
        seekg(pos);
        ignore(len);
        return r;
    }
    //! reads exactly 'len' number of bytes
    std::u16string read_exact_unicode(size_t len)
    {
        // strings that are exact number of bytes in the format, but cut off on \0
        size_t pos = tellg();
        std::u16string r = read_unicode(u16_nchars(len));
        seekg(pos);
        ignore(len);
        return r;
    }
    std::vector<uint8_t> read_binary(size_t len)
    {
        std::vector<uint8_t> tmp(len);
        read((char*)tmp.data(), len);
        return tmp;
    }
};

// sections start {{{

template <class T>
class Section
{
protected:
    T                       m_data;
    std::vector<Error>      m_warnings;
    LnkOutput::StreamPtr    m_out;

    template <class... Args>
    void warn(const char *fmt, Args... args)
    {
        auto w = Error::format(fmt, args...);
        m_warnings.push_back(w);
    }

public:
    Section(): m_out(LnkOutput::Stream::make()) { }
    T& data() { return m_data; }
    operator T& () { return m_data; }
    std::vector<Error>& warnings() { return m_warnings; }
    LnkOutput::StreamPtr output() { return std::move(m_out); }
};

class BoundsChecker
{
protected:
    size_t m_struct_start;
    size_t m_struct_end;   // this should be offset 1 byte beyond struct

public:
    size_t
    struct_start(size_t start)
    {
        m_struct_start = start;
        return m_struct_start;
    }

    size_t
    struct_start() const
    {
        return m_struct_start;
    }

    size_t
    struct_end() const
    {
        return m_struct_end;
    }

    //! set struct_end(). struct_start needs to be initalized before.
    void
    struct_len(size_t len, const char* name)
    {
        if (add_overflows(m_struct_start, len)) {
            throw Error::format("Field '%s' has bad length %llu: integer overflow", name, len);
        }
        m_struct_end = m_struct_start + len;
    }

    //! set struct_end(). struct_start needs to be initalized before.
    bool
    struct_len_nothrow(size_t len)
    {
        if (add_overflows(m_struct_start, len)) {
            return false;
        }
        m_struct_end = m_struct_start + len;
        return true;
    }

    //! move start point by a number of bytes
    bool
    struct_pop_nothrow(size_t len)
    {
        if (add_overflows(m_struct_start, len) || m_struct_start + len > m_struct_end) {
            return false;
        } else {
            m_struct_start += len;
            return true;
        }
    }

    //! check if at least 1 byte can be read from given offset
    void
    check_offsets(size_t off1, size_t off2, const char *field_name) const
    {
        if (add_overflows(m_struct_start, off1) || add_overflows(m_struct_start + off1, off2)) {
            throw Error::format("Field '%s' has bad offset %llu+%llu+%llu: integer overflow",
                                field_name, uint64_t(m_struct_start), uint64_t(off1),
                                uint64_t(off2));
        }
        if (m_struct_start + off1 + off2 >= m_struct_end) {
            throw Error::format("Field '%s' offset beyond end of structure %llu+%llu+%llu>%llu",
                                field_name, uint64_t(m_struct_start), uint64_t(off1),
                                uint64_t(off2), uint64_t(m_struct_end));
        }
    }

    //! check if at least 1 byte can be read from given offset
    bool
    check_offsets_nothrow(size_t off1, size_t off2) const
    {
        if (add_overflows(m_struct_start, off1) || add_overflows(m_struct_start + off1, off2)) {
            return false;
        }
        if (m_struct_start + off1 + off2 >= m_struct_end) {
            return false;
        }
        return true;
    }

    //! maximum length of string from given offset to end
    size_t
    maxlen(size_t off1 = 0, size_t off2 = 0) const
    {
        if (add_overflows(m_struct_start, off1) || add_overflows(m_struct_start + off1, off2) ||
            m_struct_start + off1 + off2 > m_struct_end)
        {
            return 0;
        }
        else
        {
            return m_struct_end - off2 - off1 - m_struct_start;
        }
    }

    //! check if number of bytes can be read from given offset
    bool
    check_read_nothrow(size_t off, size_t bytes) const
    {
        return maxlen(off) >= bytes;
    }

    BoundsChecker(): m_struct_start(0), m_struct_end(0) { }
};

// section 2.1
class Header: public Section<LnkStruct::ShellLinkHeader>
{
public:
    Header(FileStream &in)
    {
        LnkStruct::ShellLinkHeader r;
        in >> r.HeaderSize;
        if (r.HeaderSize != 0x4C) {
            throw Error::format("Wrong header size, should be 0x4C, got %#X", r.HeaderSize);
        }
        // magic number
        LnkStruct::Guid guid;
        static const char *magic = "00021401-0000-0000-C000-000000000046";
        in >> guid;
        if (guid != magic) {
            throw Error::format("Wrong magic number, expected %s, got %s",
                                magic, guid.string().c_str());
        }
        in >> r.LinkFlags;
        if (!r.LinkFlags.verify()) {
            // Invalid link flags fatal, because these define further structure of the file
            throw Error::format("Link flags are not valid: %#X, invalid bits are %#X",
                                r.LinkFlags.value(), r.LinkFlags.get_invalid_bits());
        }
        m_out->put("LinkFlags", r.LinkFlags);
        in >> r.FileAttributes;
        m_out->put("FileAttributes", r.FileAttributes);
        in >> r.CreationTime;
        m_out->put("CreationTime", r.CreationTime);
        in >> r.AccessTime;
        m_out->put("AccessTime", r.AccessTime);
        in >> r.WriteTime;
        m_out->put("WriteTime", r.WriteTime);
        in >> r.FileSize;
        m_out->put("FileSize", r.FileSize, LnkOutput::IntegerValue::FileSize);
        in >> r.IconIndex;
        m_out->put_debug("IconIndex", r.IconIndex);
        in >> r.ShowCommand;
        m_out->put_debug("ShowCommand", r.ShowCommand);
        in >> r.HotKeyLow;
        m_out->put_debug("HotKeyLow", r.HotKeyLow);
        in >> r.HotKeyHigh;
        m_out->put_debug("HotKeyHigh", r.HotKeyHigh);
        in >> r.Reversed1;
        in >> r.Reserved2;
        in >> r.Reserved3;
        m_data = r;
    }
};

// section 2.2
class LinkTargetIdList: public Section<LnkStruct::LinkTargetIdList>, protected BoundsChecker
{
private:
    FileStream &m_in;

    bool
    ext_BEEF0004(BoundsChecker& b, LnkOutput::StreamPtr& o, LnkStruct::ShellId_BeefBase z)
    {
        LnkStruct::ShellId_Beef0004 e;
        if (!b.struct_pop_nothrow(sizeof(e.CreationTime)+sizeof(e.AccessTime)+
                                    sizeof(e.WindowsVersion)))
        {
            return false;
        }
        m_in >> e.CreationTime;
        m_in >> e.AccessTime;
        m_in >> e.WindowsVersion;
        o->put("CreationTime", e.CreationTime);
        o->put("AccessTime", e.AccessTime);
        o->put_debug("WindowsVersion", e.WindowsVersion);
        if (z.Version >= 7) {
            if (!b.struct_pop_nothrow(sizeof(e.Unknown1)+sizeof(e.FileReference)+
                                        sizeof(e.Unknown2)))
            {
                return false;
            }
            m_in >> e.Unknown1;
            m_in >> e.FileReference;
            o->put_debug("MFTEntryIndex", get_bits<0, 47>(e.FileReference));
            o->put_debug("Sequence", get_bits<48, 63>(e.FileReference));
            m_in >> e.Unknown2;
        }
        if (z.Version >= 3) {
            if (!b.struct_pop_nothrow(sizeof(e.LongStringSize))) {
                return false;
            }
            m_in >> e.LongStringSize;
        }
        if (z.Version >= 9) {
            if (!b.struct_pop_nothrow(sizeof(e.Unknown3))) {
                return false;
            }
            m_in >> e.Unknown3;
        }
        if (z.Version >= 8) {
            if (!b.struct_pop_nothrow(sizeof(e.Unknown4))) {
                return false;
            }
            m_in >> e.Unknown4;
        }
        if (z.Version >= 3) {
            std::u16string s = m_in.read_unicode(u16_nchars(b.maxlen()));
            if (!b.struct_pop_nothrow(u16s0_nbytes(s))) {
                return false;
            }
            e.LongName = utf16le_to_utf8(s);
            o->put("LongName", e.LongName, true);
        }
        if (z.Version >= 3 && e.LongStringSize > 0) {
            std::string s = m_in.read_ansi(b.maxlen());
            if (!b.struct_pop_nothrow(s.size()+1)) {
                return false;
            }
            o->put("LocalizedName", e.LocalizedName, false);
        }
        if (z.Version >= 7 && e.LongStringSize > 0) {
            std::u16string s = m_in.read_unicode(b.maxlen());
            if (!b.struct_pop_nothrow(u16s0_nbytes(s))) {
                return false;
            }
            e.LocalizedName = utf16le_to_utf8(s);
            o->put("LocalizedNameU", e.LocalizedName, true);
        }
        return true;
    }

    LnkOutput::StreamPtr
    x1f_root_folder(BoundsChecker b)
    {
        auto o = LnkOutput::Stream::make();
        LnkStruct::ShellId_x1F_SortIndex_t sort_idx;
        LnkStruct::Guid folder;
        if (!b.struct_pop_nothrow(sizeof(sort_idx) + sizeof(folder))) {
            return o;
        }
        m_in >> sort_idx;
        o->put_debug("SortIndex", sort_idx);
        m_in >> folder;
        const char* desc = LnkStruct::shell_folder_guid_describe(folder.string().c_str());
        if (desc) {
            o->put("ShellFolder", desc, true);
            o->put_debug("ShellFolderGuid", folder);
        } else {
            o->put("ShellFolderGuid", folder);
        }
        return o;
    }

    LnkOutput::StreamPtr
    x20_volume(const LnkStruct::LinkTargetIdList::ID& id)
    {
        // found no documentation on this
        auto o = LnkOutput::Stream::make();
        uint8_t flags = (id.Data.data()[0] & (~0x70));
        o->put("Flags", flags, LnkOutput::IntegerValue::Hex);
        return o;
    }

    LnkOutput::StreamPtr
    x30_file(const LnkStruct::LinkTargetIdList::ID& id, BoundsChecker b)
    {
        auto o = LnkOutput::Stream::make();
        LnkStruct::ShellId_x30_Struct f;
        f.Flags = (uint8_t)(id.Data.data()[0] & (~0x70));
        o->put_debug("Flags", f.Flags);
        size_t saved_itemid_offset = b.struct_start() - 1;  // for pre-xp / post-xp heuristic
        if (!b.struct_pop_nothrow(sizeof(f.Unknown1)+sizeof(f.FileSize)+sizeof(f.ModifiedTime)+
                                  sizeof(f.Attributes)))
        {
            return o;
        }
        m_in >> f.Unknown1;
        m_in >> f.FileSize;
        o->put("FileSize", f.FileSize, LnkOutput::IntegerValue::FileSize);
        m_in >> f.ModifiedTime;
        o->put("ModifiedTime", f.ModifiedTime);
        m_in >> f.Attributes;
        LnkStruct::FileAttributes_t a{f.Attributes};  // it's only 16 bits in this struct
        o->put("Attributes", a);
        if (b.maxlen() <= 0) {
            return o;
        }
        if (f.is_unicode()) {
            std::u16string u = m_in.read_unicode(u16_nchars(b.maxlen()));
            if (!b.struct_pop_nothrow(u16s0_nbytes(u))) {
                return o;
            }
            f.Name = utf16le_to_utf8(u);
            o->put("Name", f.Name, true);
        } else {
            std::string a = m_in.read_ansi(b.maxlen());
            if (!b.struct_pop_nothrow(a.size()+1)) {
                return o;
            }
            f.Name = a;
            o->put("Name", f.Name, false);
        }
        // check for alignment byte
        if (b.maxlen() <= 0) {
            return o;
        }
        //std::cout << "offset at end of name " << b.struct_start() << std::endl;// TODO
        if (m_in.peek() == 0) {
            m_in.ignore(sizeof(char));
            b.struct_pop_nothrow(sizeof(char));
        }
        if (b.maxlen() < sizeof(uint16_t)) {
            return o;
        }
        // try to detect pre-xp / post-xp
        uint16_t maybe_size;
        uint16_t maybe_offset;
        m_in >> maybe_size;
        size_t version_offset = m_in.tellg();
        //std::cout << "version offset = " << version_offset << ", b.offset = " << b.struct_start() << std::endl; TODO
        m_in.seekg(b.struct_end() - sizeof(maybe_offset));
        m_in >> maybe_offset;
        // std::cout << "maybe_size=" << maybe_size << " maybe_offset=" << maybe_offset << // TODO
        //                 " struct_offset=" << version_offset - saved_itemid_offset << std::endl;
        if (b.maxlen() >= maybe_size &&  // extension size includes itself
            maybe_offset == version_offset - saved_itemid_offset)  // should point back here
        {
            //std::cout << "post-xp" << std::endl;
            // post-xp
            LnkStruct::ShellId_BeefBase z;
            if (!b.struct_pop_nothrow(sizeof(z.Size))) {
                return o;
            }
            m_in.seekg(b.struct_start());
            if (!b.struct_pop_nothrow(sizeof(z.Version)+sizeof(z.Signature))) {
                return o;
            }
            z.Size = maybe_size;
            m_in >> z.Version;
            o->put_debug("Version", z.Version);
            m_in >> z.Signature;
            o->put_debug("Signature", z.Signature, LnkOutput::IntegerValue::Hex);
            if (z.Signature == LnkStruct::ShellId_Beef0004::Signature) {
                ext_BEEF0004(b, o, z);
            }
        }
        else
        {
            // pre-xp
            m_in.seekg(b.struct_start());  // after string / align byte
            if (f.is_unicode()) {
                std::u16string u = m_in.read_unicode(u16_nchars(b.maxlen()));
                if (!b.struct_pop_nothrow(u16s0_nbytes(u))) {
                    return o;
                }
                f.SecondaryName = utf16le_to_utf8(u);
                o->put("SecondaryName", f.SecondaryName, true);
            } else {
                std::string a = m_in.read_ansi(b.maxlen());
                if (!b.struct_pop_nothrow(a.size()+1)) {
                    return o;
                }
                f.SecondaryName = a;
                o->put("SecondaryName", f.SecondaryName, false);
            }
        }
        return o;
    }

    LnkOutput::StreamPtr
    x40_network(const LnkStruct::LinkTargetIdList::ID& id, BoundsChecker b)
    {
        auto o = LnkOutput::Stream::make();
        LnkStruct::ShellId_x40_Struct f;
        f.Type = id.Data.data()[0] & (~0x70);
        o->put("Type", f.Type);
        if (!b.struct_pop_nothrow(sizeof(f.Unknown1)+sizeof(f.Flags))) {
            return o;
        }
        m_in >> f.Unknown1;
        m_in >> f.Flags;
        o->put_debug("Flags", f.Flags);
        if (b.maxlen() <= 0) {
            return o;
        }
        f.Location = m_in.read_ansi(b.maxlen());
        if (!b.struct_pop_nothrow(f.Location.size()+1) || b.maxlen() <= 0) {
            return o;
        }
        o->put("Location", f.Location, false);
        if (f.has_description()) {
            f.Description = m_in.read_ansi(b.maxlen());
            if (!b.struct_pop_nothrow(f.Description.size()+1) || b.maxlen() <= 0) {
                return o;
            }
            o->put("Description", f.Description, false);
        }
        if (f.has_comments()) {
            f.Comments = m_in.read_ansi(b.maxlen());
            o->put("Comments", f.Comments, false);
        }
        return o;
    }

    LnkOutput::StreamPtr
    x50_zip_folder(BoundsChecker b)
    {
        auto o = LnkOutput::Stream::make();
        LnkStruct::ShellId_x50_Struct f;
        if (!b.struct_pop_nothrow(sizeof(f.Unknown1)+sizeof(f.Unknown2)+sizeof(f.Unknown3)+
                                  sizeof(f.Unknown4)+sizeof(f.Unknown5)+sizeof(f.Unknown6)+
                                  sizeof(f.Timestamp)+sizeof(f.Unknown7)+sizeof(f.Timestamp2)))
        {
            return o;
        }
        m_in >> f.Unknown1;
        m_in >> f.Unknown2;
        m_in >> f.Unknown3;
        m_in >> f.Unknown4;
        m_in >> f.Unknown5;
        m_in >> f.Unknown6;
        m_in >> f.Timestamp;
        o->put("Timestamp", f.Timestamp);
        m_in >> f.Unknown7;
        m_in >> f.Timestamp2;
        if (f.Timestamp2 != 0) {
            o->put("Timestamp2", f.Timestamp2);
        }
        if (!b.struct_pop_nothrow(sizeof(f.FullPathSize))) {
            return o;
        }
        m_in >> f.FullPathSize;  // ignore
        if (b.maxlen() <= 0) {
            return o;
        }
        std::u16string tmp = m_in.read_unicode(u16_nchars(b.maxlen()));
        if (!b.struct_pop_nothrow(u16s0_nbytes(tmp))) {
            return o;
        }
        f.FullPath = utf16le_to_utf8(tmp);
        o->put("FullPath", f.FullPath, true);
        return o;
    }

    LnkOutput::StreamPtr
    x60_uri(const LnkStruct::LinkTargetIdList::ID& id, BoundsChecker b)
    {
        auto o = LnkOutput::Stream::make();
        LnkStruct::ShellId_x60_Struct f;
        if (!b.struct_pop_nothrow(sizeof(f.Flags))) {
            return o;
        }
        m_in >> f.Flags;
        o->put_debug("Flags", f.Flags);
        if ((id.Data.data()[0] & (~0x70)) == 0x01 && (f.Flags & (~0x80)) == 0x00) {
            // seems to only contain 1 byte flags, 4 bytes zero and a string
            if (!b.struct_pop_nothrow(sizeof(f.Unknown1))) {
                return o;
            }
            m_in >> f.Unknown1;
            if (f.is_unicode()) {
                auto u = m_in.read_unicode(u16_nchars(b.maxlen()));
                f.URI = utf16le_to_utf8(u);
                if (f.URI.size() > 0) {
                    o->put("URI", f.URI, true);
                }
            } else {
                f.URI = m_in.read_ansi(b.maxlen());
                if (f.URI.size() > 0) {
                    o->put("URI", f.URI, false);
                }
            }
            return o;
        }
        if (!b.struct_pop_nothrow(sizeof(f.DataSize))) {
            return o;
        }
        m_in >> f.DataSize;
        if (f.DataSize > 0) {
            if (!b.struct_pop_nothrow(sizeof(f.Unknown1)+sizeof(f.Unknown2)+sizeof(f.Timestamp)+
                                      sizeof(f.Unknown4)+sizeof(f.Unknown5)+sizeof(f.Unknown6)+
                                      sizeof(f.Unknown7)+sizeof(f.Unknown8)+sizeof(f.String1Bytes)))
            {
                return o;
            }
            m_in >> f.Unknown1;
            m_in >> f.Unknown2;
            m_in >> f.Timestamp;
            o->put("Timestamp", f.Timestamp);
            m_in >> f.Unknown4;
            m_in >> f.Unknown5;
            m_in >> f.Unknown6;
            m_in >> f.Unknown7;
            m_in >> f.Unknown8;
            m_in >> f.String1Bytes;
            if (!b.struct_pop_nothrow(f.String1Bytes)) {
                return o;
            }
            if (f.is_unicode()) {
                std::u16string u = m_in.read_exact_unicode(f.String1Bytes);
                f.FTPHostname = utf16le_to_utf8(u);
                if (f.FTPHostname.size() > 0) {
                    o->put("FTPHostName", f.FTPHostname, true);
                }
            } else {
                f.FTPHostname = m_in.read_exact(f.String1Bytes);
                if (f.FTPHostname.size() > 0) {
                    o->put("FTPHostName", f.FTPHostname, false);
                }
            }
            if (!b.struct_pop_nothrow(sizeof(f.String2Bytes))) {
                return o;
            }
            m_in >> f.String2Bytes;
            if (!b.struct_pop_nothrow(f.String2Bytes)) {
                return o;
            }
            if (f.is_unicode()) {
                std::u16string u = m_in.read_exact_unicode(f.String2Bytes);
                f.FTPUser = utf16le_to_utf8(u);
                if (f.FTPUser.size() > 0) {
                    o->put("FTPUser", f.FTPUser, true);
                }
            } else {
                f.FTPUser = m_in.read_exact(f.String2Bytes);
                if (f.FTPUser.size() > 0) {
                    o->put("FTPUser", f.FTPUser, false);
                }
            }
            if (!b.struct_pop_nothrow(sizeof(f.String3Bytes))) {
                return o;
            }
            m_in >> f.String3Bytes;
            if (!b.struct_pop_nothrow(f.String3Bytes)) {
                return o;
            }
            if (f.is_unicode()) {
                std::u16string u = m_in.read_exact_unicode(f.String3Bytes);
                f.FTPPassword = utf16le_to_utf8(u);
                if (f.FTPUser.size() > 0) {
                    o->put("FTPPassword", f.FTPPassword, true);
                }
            } else {
                f.FTPPassword = m_in.read_exact(f.String3Bytes);
                if (f.FTPUser.size() > 0) {
                    o->put("FTPPassword", f.FTPPassword, false);
                }
            }
        }
        if (b.maxlen() <= 0) {
            return o;
        }
        if (f.is_unicode()) {
            std::u16string u = m_in.read_unicode(u16_nchars(b.maxlen()));
            f.URI = utf16le_to_utf8(u);
            if (f.URI.size() > 0) {
                o->put("URI", f.URI, true);
            }
        } else {
            f.URI = m_in.read_ansi(b.maxlen());
            if (f.URI.size() > 0) {
                o->put("URI", f.URI, false);
            }
        }
        // there are more data following, including maybe block BEEF0014
        return o;
    }

    LnkOutput::StreamPtr
    x70_control_panel(BoundsChecker b)
    {
        auto o = LnkOutput::Stream::make();
        LnkStruct::ShellId_x70_Struct f;
        if (!b.struct_pop_nothrow(sizeof(f.SortOrder)+sizeof(f.Unknown1)+sizeof(f.Unknown2)+
                                  sizeof(f.Unknown3)+sizeof(f.GUID)))
        {
            return o;
        }
        m_in >> f.SortOrder;
        o->put_debug("SortOrder", f.SortOrder, LnkOutput::IntegerValue::Hex);
        m_in >> f.Unknown1;
        m_in >> f.Unknown2;
        m_in >> f.Unknown3;
        m_in >> f.GUID;
        const char* desc = LnkStruct::control_panel_guid_describe(f.GUID.string().c_str());
        if (desc != nullptr) {
            o->put("Category", desc, true);
        }
        o->put("GUID", f.GUID);
        return o;
    }

    LnkOutput::StreamPtr
    x74_user_folder_delegate(BoundsChecker b)
    {
        auto o = LnkOutput::Stream::make();
        LnkStruct::ShellId_x74_Struct f;
        BoundsChecker outer = b;
        if (!b.struct_pop_nothrow(sizeof(f.Unknown1)+sizeof(f.DelegateOffset)+
                                  sizeof(f.SubShellItemSignature)+sizeof(f.SubShellItemSize)))
        {
            return o;
        }
        BoundsChecker inner = b;
        m_in >> f.Unknown1;
        m_in >> f.DelegateOffset;
        // offset+3, to exclude size of Unknown1 and DelegateOffset itself
        if (!outer.check_offsets_nothrow(3, f.DelegateOffset)) {
            return o;
        }
        m_in >> f.SubShellItemSignature;
        m_in >> f.SubShellItemSize;
        if (f.SubShellItemSignature != f.Signature ||
            !b.struct_pop_nothrow(f.SubShellItemSize))
        {
            return o;
        }
        {
            // inner item
            auto& s = f.SubShellItem;
            inner.struct_len(f.SubShellItemSize, "SubShellItem");
            if (inner.struct_end() > outer.struct_end() ||
                inner.struct_end() > outer.struct_start() + f.DelegateOffset + 3 ||
                !inner.struct_pop_nothrow(sizeof(s.ClsType)+sizeof(s.Unknown1)+
                                          sizeof(s.ModifiedTime)+sizeof(s.FileAttributes)))
            {
                return o;
            }
            m_in >> s.ClsType;
            if (s.ClsType != 0x31) {
                return o;
            }
            m_in >> s.Unknown1;
            m_in >> s.FileSize;
            o->put("FileSize", s.FileSize, LnkOutput::IntegerValue::FileSize);
            m_in >> s.ModifiedTime;
            o->put("ModifiedTime", s.ModifiedTime);
            m_in >> s.FileAttributes;
            LnkStruct::FileAttributes_t a{s.FileAttributes};
            o->put("FileAttributes", a);
            s.PrimaryName = m_in.read_ansi(inner.maxlen());
            o->put("PrimaryName", s.PrimaryName, false);
        }
        m_in.seekg(outer.struct_start() + 3 + f.DelegateOffset);
        // delegate item
        if (!b.struct_pop_nothrow(sizeof(f.DelegateGuid)+sizeof(f.DelegateClass))) {
            return o;
        }
        m_in >> f.DelegateGuid;
        o->put_debug("DelegateGuid", f.DelegateGuid);
        m_in >> f.DelegateClass;
        const char* desc = LnkStruct::shell_folder_guid_describe(f.DelegateClass.string().c_str());
        if (desc != nullptr) {
            o->put_debug("DelegateClass", desc, true);
        }
        o->put_debug("DelegateClassGuid", f.DelegateClass);
        // extension block BEEF0004 follows
        LnkStruct::ShellId_BeefBase z;
        if (!b.struct_pop_nothrow(sizeof(z.Size)+sizeof(z.Version)+sizeof(z.Signature))) {
            return o;
        }
        m_in >> z.Size;
        m_in >> z.Version;
        m_in >> z.Signature;
        if (z.Signature == LnkStruct::ShellId_Beef0004::Signature) {
            ext_BEEF0004(b, o, z);
        }
        return o;
    }

    void
    unknown_shellid(const LnkStruct::LinkTargetIdList::ID& id)
    {
        auto o = LnkOutput::Stream::make();
        o->put("Bytes", id.Data);
        m_out->put_debug("UnknownShellId", std::move(o));
    }

public:
    LinkTargetIdList(FileStream &in):
        m_in(in)
    {
        // the problem with this struct is that it is so poorly documented.
        // check bounds on each read and return if it would go past the end of the struct.
        // avoid throwing errors, just ignore them in this case.
        m_in >> m_data.IdListSize;
        struct_start(m_in.tellg());  // IdListSize does not include size of itself
        struct_len(m_data.IdListSize, "LinkTargetIdList");
        while (true) {
            LnkStruct::LinkTargetIdList::ID id;
            BoundsChecker item_bounds;
            item_bounds.struct_start(struct_start());
            if (!check_read_nothrow(0, sizeof(id.ItemIdSize))) {
                m_in.seekg(struct_end());
                return;
            }
            m_in >> id.ItemIdSize;  // ItemIdSize does include size of itself
            if (id.ItemIdSize == 0) {
                // terminal item
                break;
            }
            if (!check_read_nothrow(0, id.ItemIdSize) ||
                !item_bounds.struct_len_nothrow(id.ItemIdSize) ||
                !item_bounds.struct_pop_nothrow(sizeof(id.ItemIdSize)))
            {
                //std::cerr << "size " << id.ItemIdSize << " too big, seek to structend " << struct_end() << std::endl; // TODO
                m_in.seekg(struct_end());
                return;
            }
            if (!check_read_nothrow(0, id.ItemIdSize - sizeof(id.ItemIdSize))) {
                m_in.seekg(struct_end());
                return;
            }
            id.Data = in.read_binary(id.ItemIdSize - sizeof(id.ItemIdSize));
            m_in.seekg(item_bounds.struct_start());  // after ItemIdSize
            uint8_t clstype;
            if (!item_bounds.struct_pop_nothrow(sizeof(clstype))) {
                m_in.seekg(struct_end());
                return;
            }
            m_in >> clstype;
            if (clstype == 0x1F) {
                auto o = x1f_root_folder(item_bounds);
                m_out->put("FolderShellId", std::move(o));
            } else if ((clstype & 0x70) == 0x20) {
                auto o = x20_volume(id);
                o->put_debug("Bytes", id.Data);
                m_out->put("VolumeShellId", std::move(o));
            } else if ((clstype & 0x70) == 0x30) {
                auto o = x30_file(id, item_bounds);
                o->put_debug("Bytes", id.Data);
                m_out->put("FileShellId", std::move(o));
            } else if ((clstype & 0x70) == 0x40) {
                auto o = x40_network(id, item_bounds);
                o->put_debug("Bytes", id.Data);
                m_out->put("NetworkLocationShellId", std::move(o));
            } else if ((clstype & 0x70) == 0x50) {
                auto o = x50_zip_folder(item_bounds);
                o->put_debug("Bytes", id.Data);
                m_out->put("ZipFolderShellId", std::move(o));
            } else if ((clstype & 0x70) == 0x60) {
                auto o = x60_uri(id, item_bounds);
                o->put_debug("Bytes", id.Data);
                m_out->put("URIShellId", std::move(o));
            } else if (clstype == 0x74) {
                auto o = x74_user_folder_delegate(item_bounds);
                o->put_debug("Bytes", id.Data);
                m_out->put("UserFolderDelegate", std::move(o));
            } else if ((clstype & 0x70) == 0x70) {
                auto o = x70_control_panel(item_bounds);
                o->put_debug("Bytes", id.Data);
                m_out->put("ControlPanelShellId", std::move(o));
            } else {
                unknown_shellid(id);
            }
            struct_pop_nothrow(id.ItemIdSize);
            m_in.seekg(struct_start());
            m_data.IdList.push_back(std::move(id));
        }
        m_in.seekg(struct_end());
    }
};

// section 2.3
class LinkInfo: public Section<LnkStruct::LinkInfo>, protected BoundsChecker
{
private:
    FileStream&  m_in;

    //! reads a NUL-terminated 8-bit string at m_struct_start+off1+off2
    std::string offset_ansi(size_t off1, size_t off2, const char* field_name)
    {
        check_offsets(off1, off2, field_name);
        m_in.seekg(m_struct_start + off1 + off2);
        return m_in.read_ansi(maxlen(off1, off2));
    }

    std::string offset_uni_cvt(size_t off1, size_t off2, const char* field_name)
    {
        check_offsets(off1, off2, field_name);
        m_in.seekg(m_struct_start + off1 + off2);
        auto tmp16 = m_in.read_unicode(maxlen(off1, off2));
        auto tmp8 = utf16le_to_utf8(tmp16);
        return tmp8;
    }

    void header()
    {
        struct_start(m_in.tellg());
        auto &h = m_data.header;
        m_in >> h.LinkInfoSize;
        m_in >> h.LinkInfoHeaderSize;
        struct_len(h.LinkInfoSize, "LinkInfo");
        m_in >> h.LinkInfoFlags;
        m_out->put_debug("LinkInfoFlags", h.LinkInfoFlags);
        m_in >> h.VolumeIDOffset;
        m_in >> h.LocalBasePathOffset;
        m_in >> h.CommonNetworkRelativeLinkOffset;
        m_in >> h.CommonPathSuffixOffset;
        switch (h.has_optional_fields()) {
            case 1:
                m_in >> h.LocalBasePathOffsetUnicode;
                m_in >> h.CommonPathSuffixOffsetUnicode;
                break;
            case 0:
                h.LocalBasePathOffsetUnicode = 0;
                h.CommonPathSuffixOffsetUnicode = 0;
                break;
            default:
                throw Error::format("Wrong Link Info Header size, expected 0x1C or >=0x24, got %#X",
                                    h.LinkInfoHeaderSize);
        }
        if (add_overflows(h.LinkInfoSize, m_struct_start)) {
            throw Error::format("Link Info size is wrong, got %d bytes", h.LinkInfoSize);
        }
    }

    void volume_id()
    {
        auto volid_offset = m_data.header.VolumeIDOffset;
        check_offsets(volid_offset, 0x10, "VolumeID");  // 0x10 is the minimum size of VolumeID
        m_in.seekg(m_struct_start + volid_offset);
        auto &vi = m_data.data.VolumeID;
        m_in >> vi.Size;
        check_offsets(volid_offset, vi.Size, "VolumeIDSize");
        m_in >> vi.DriveType;
        m_out->put("DriveType", vi.DriveType);
        m_in >> vi.DriveSerialNumber;
        m_out->put_debug("DriveSerialNumber", vi.DriveSerialNumber);
        m_in >> vi.VolumeLabelOffset;
        m_in >> vi.VolumeLabelOffsetUnicode;
        if (vi.has_unicode_label()) {
            vi.VolumeLabelUnicode = offset_uni_cvt(volid_offset, vi.VolumeLabelOffsetUnicode,
                                                   "VolumeLabelUnicode");
            m_out->put("VolumeLabel", vi.VolumeLabelUnicode, true);
        } else {
            vi.VolumeLabel = offset_ansi(volid_offset, vi.VolumeLabelOffset, "VolumeLabel");
            m_out->put("VolumeLabel", vi.VolumeLabel, false);
        }
    }

    void common_network_relative_link()
    {
        auto cnrl_offset = m_data.header.CommonNetworkRelativeLinkOffset;
        check_offsets(cnrl_offset, 0x14, "CommonNetworkRelativeLinkOffset");
        m_in.seekg(m_struct_start + cnrl_offset);
        auto &cnrl = m_data.data.CommonNetworkRelativeLink;
        m_in >> cnrl.Size;
        check_offsets(cnrl_offset, cnrl.Size, "CommonNetworkRelativeLinkSize");
        m_in >> cnrl.Flags;
        if (!cnrl.Flags.verify()) {
            // fatal, required to detect presence of offsets
            throw Error::format("CommonNetworkRelativeLink flags are not valid: %#X, "
                                "invalid bits are %#X", cnrl.Flags.value(),
                                cnrl.Flags.invalid_bits);
        }
        m_out->put("CommonNetworkRelativeLinkFlags", cnrl.Flags);
        m_in >> cnrl.NetNameOffset;
        m_in >> cnrl.DeviceNameOffset;
        m_in >> cnrl.NetworkProviderType;
        m_out->put("NetworkProviderType", cnrl.NetworkProviderType);
        if (cnrl.has_optional_fields()) {
            m_in >> cnrl.NetNameOffsetUnicode;
            m_in >> cnrl.DeviceNameOffsetUnicode;
        } else {
            cnrl.NetNameOffsetUnicode = 0;
            cnrl.DeviceNameOffsetUnicode = 0;
        }
        if (cnrl.has_optional_fields()) {
            cnrl.NetNameUnicode = offset_uni_cvt(cnrl_offset, cnrl.NetNameOffsetUnicode,
                                                 "NetNameUnicode");
            m_out->put("NetName", cnrl.NetNameUnicode, true);
            if (cnrl.has_device_name()) {
                cnrl.DeviceNameUnicode = offset_uni_cvt(cnrl_offset, cnrl.DeviceNameOffsetUnicode,
                                                        "DeviceNameUnicode");
                m_out->put("DeviceName", cnrl.DeviceNameUnicode, true);
            }
        } else {
            cnrl.NetName = offset_ansi(cnrl_offset, cnrl.NetNameOffset, "NetName");
            m_out->put("NetName", cnrl.NetName, false);
            if (cnrl.has_device_name()) {
                cnrl.DeviceName = offset_ansi(cnrl_offset, cnrl.DeviceNameOffset, "DeviceName");
                m_out->put("DeviceName", cnrl.DeviceName, false);
            }
        }
    }

public:
    LinkInfo(FileStream &in): m_in(in)
    {
        header();
        if (m_data.header.has_volume_id_and_local_base_path()) {
            auto &h = m_data.header;
            auto &d = m_data.data;
            volume_id();

            if (m_data.header.has_optional_fields()) {
                d.LocalBasePathUnicode = offset_uni_cvt(h.LocalBasePathOffsetUnicode, 0,
                                                        "LocalBasePathUnicode");
                d.CommonPathSuffixUnicode = offset_uni_cvt(h.CommonPathSuffixOffsetUnicode, 0,
                                                           "LocalBasePathUnicode");
                m_out->put("LocalBasePath", d.LocalBasePathUnicode, true);
                m_out->put("CommonPathSuffix", d.CommonPathSuffixUnicode, true);
            } else {
                d.LocalBasePath = offset_ansi(h.LocalBasePathOffset, 0, "LocalBasePath");
                d.CommonPathSuffix = offset_ansi(h.CommonPathSuffixOffset, 0, "CommonPathSuffix");
                m_out->put("LocalBasePath", d.LocalBasePath, false);
                m_out->put("CommonPathSuffix", d.CommonPathSuffix, false);
            }
        }
        if (m_data.header.has_common_network_relative_link()) {
            common_network_relative_link();
        }
        m_in.seekg(m_struct_end);
    }
};

// section 2.4
class StringData: public Section<LnkStruct::StringData>
{
private:
    FileStream& m_in;

    // the strings in this section are prefixed with size
    //! read ansi string at current offset (as defined in 2.4 - 16bit length, then chars)
    std::string ansi()
    {
        uint16_t n_chars;
        m_in >> n_chars;
        return m_in.read_exact(n_chars);
    }

    //! read unicode string at current offset (as defined in 2.4 - 16bit length, then chars)
    std::string uni_cvt()
    {
        uint16_t n_chars;
        m_in >> n_chars;
        std::u16string u = m_in.read_exact_unicode(n_chars*sizeof(uint16_t));
        return utf16le_to_utf8(u);
    }

public:
    StringData(FileStream &in, LnkStruct::ShellLinkHeader& h): m_in(in)
    {
        auto& s = m_data;
        bool unicode = h.has_unicode_strings();
        if (h.has_name_string()) {
            s.Name = unicode ? uni_cvt() : ansi();
            m_out->put("Name", s.Name, unicode);
        }
        if (h.has_relpath_string()) {
            s.RelativePath = unicode ? uni_cvt() : ansi();
            m_out->put("RelativePath", s.RelativePath, unicode);
        }
        if (h.has_workdir_string()) {
            s.WorkingDir = unicode ? uni_cvt() : ansi();
            m_out->put("WorkingDir", s.WorkingDir, unicode);
        }
        if (h.has_args_string()) {
            s.CommandLine = unicode ? uni_cvt() : ansi();
            m_out->put("CommandLine", s.CommandLine, unicode);
        }
        if (h.has_iconloc_string()) {
            s.IconLocation = unicode ? uni_cvt() : ansi();
            m_out->put("IconLocation", s.IconLocation, unicode);
        }
        s.UnicodeFlag = unicode;
    }
};

// section 2.5
class ExtraData: public Section<LnkStruct::ExtraDataPH>
{
private:
    FileStream &m_in;
    void console_data()
    {
        auto x = std::make_unique<LnkStruct::ConsoleDataBlock>();
        auto o = LnkOutput::Stream::make();
        m_in >> x->FillAttributes;
        o->put("FillAttributes", x->FillAttributes);
        m_in >> x->PopupFillAttributes;
        o->put("PopupFillAttributes", x->PopupFillAttributes);
        m_in >> x->ScreenBufferSizeX;
        o->put("ScreenBufferSizeX", x->ScreenBufferSizeX);
        m_in >> x->ScreenBufferSizeY;
        o->put("ScreenBufferSizeY", x->ScreenBufferSizeY);
        m_in >> x->WindowSizeX;
        o->put("WindowSizeX", x->WindowSizeX);
        m_in >> x->WindowSizeY;
        o->put("WindowSizeY", x->WindowSizeY);
        m_in >> x->WindowOriginX;
        o->put("WindowOriginX", x->WindowOriginX);
        m_in >> x->WindowOriginY;
        o->put("WindowOriginY", x->WindowOriginY);
        m_in >> x->FontSize;
        o->put("FontSize", x->FontSize);
        m_in >> x->FontFamily;
        o->put("FontFamily", x->FontFamily.family());
        o->put("FontPitch", x->FontFamily.pitch());
        m_in >> x->FontWeight;
        o->put("FontWeight", x->FontWeight);
        x->FaceName = utf16le_to_utf8(m_in.read_exact_unicode(64));
        o->put("FaceName", x->FaceName, true);
        m_in >> x->CursorSize;
        o->put("CursorSize", x->CursorSize);
        m_in >> x->FullScreen;
        o->put("FullScreen", x->FullScreen);
        m_in >> x->QuickEdit;
        o->put("QuickEdit", x->QuickEdit);
        m_in >> x->InsertMode;
        o->put("InsertMode", x->InsertMode);
        m_in >> x->AutoPosition;
        o->put("AutoPosition", x->AutoPosition);
        m_in >> x->HistoryBufferSize;
        o->put("HistoryBufferSize", x->HistoryBufferSize);
        m_in >> x->NumberOfHistoryBuffers;
        o->put("NumberOfHistoryBuffers", x->NumberOfHistoryBuffers);
        m_in >> x->HistoryNoDup;
        o->put("HistoryNoDup", x->HistoryNoDup);
        m_out->put("ConsoleDataBlock", std::move(o));
    }
    void console_fe_data()
    {
        auto x = std::make_unique<LnkStruct::ConsoleFeDataBlock>();
        auto o = LnkOutput::Stream::make();
        m_in >> x->CodePage;
        o->put("CodePage", x->CodePage);
        m_out->put("ConsoleFeDataBlock", std::move(o));
    }
    void darwin_data()
    {
        auto x = std::make_unique<LnkStruct::DarwinDataBlock>();
        auto o = LnkOutput::Stream::make();
        x->DarwinDataAnsi = m_in.read_exact(260);
        // spec says to ignore DarwinDataAnsi
        x->DarwinDataUnicode = utf16le_to_utf8(m_in.read_exact_unicode(260));
        o->put("DarwinDataUnicode", x->DarwinDataUnicode, true);
        m_out->put("DarwinDataBlock", std::move(o));
    }
    void env_var_data()
    {
        auto x = std::make_unique<LnkStruct::EnvVarDataBlock>();
        auto o = LnkOutput::Stream::make();
        x->TargetAnsi = m_in.read_exact(260);
        o->put("TargetAnsi", x->TargetAnsi, false);
        x->TargetUnicode = utf16le_to_utf8(m_in.read_exact_unicode(260));
        o->put("TargetUnicode", x->TargetUnicode, true);
        m_out->put("EnvironmentVariableDataBlock", std::move(o));
    }
    void icon_env_data()
    {
        auto x = std::make_unique<LnkStruct::IconEnvDataBlock>();
        auto o = LnkOutput::Stream::make();
        x->TargetAnsi = m_in.read_exact(260);
        o->put("TargetAnsi", x->TargetAnsi, false);
        x->TargetUnicode = utf16le_to_utf8(m_in.read_exact_unicode(260));
        o->put("TargetUnicode", x->TargetUnicode, true);
        m_out->put("IconEnvironmentDataBlock", std::move(o));
    }
    void known_folder_data()
    {
        auto x = std::make_unique<LnkStruct::KnownFolderDataBlock>();
        auto o = LnkOutput::Stream::make();
        m_in >> x->KnownFolderId;
        o->put("KnownFolderId", x->KnownFolderId);
        m_in >> x->Offset;
        o->put("Offset", x->Offset);
        m_out->put("KnownFolderDataBlock", std::move(o));
    }
    void property_store(const LnkStruct::ExtraDataBlockHeader &h)  // TODO
    {
        auto o = LnkOutput::Stream::make();
        auto b = m_in.read_binary(h.BlockSize - 8);
        o->put("Bytes", b);
        m_out->put_debug("PropertyStoreDataBlock", std::move(o));
    }
    void shim_data(const LnkStruct::ExtraDataBlockHeader &h)
    {
        auto x = std::make_unique<LnkStruct::ShimDataBlock>();
        auto o = LnkOutput::Stream::make();
        size_t len = h.BlockSize - 8;
        x->LayerName = utf16le_to_utf8(m_in.read_exact_unicode(len));
        o->put("LayerName", x->LayerName, true);
        m_out->put("ShimDataBlock", std::move(o));
    }
    void special_folder()
    {
        auto x = std::make_unique<LnkStruct::SpecialFolderDataBlock>();
        auto o = LnkOutput::Stream::make();
        m_in >> x->SpecialFolderId;
        o->put("SpecialFolderId", x->SpecialFolderId);
        m_in >> x->Offset;
        o->put("Offset", x->Offset);
        m_out->put("SpecialFolderDataBlock", std::move(o));
    }
    void tracker_data()
    {
        auto x = std::make_unique<LnkStruct::TrackerDataBlock>();
        auto o = LnkOutput::Stream::make();
        m_in >> x->Length;
        m_in >> x->Version;
        x->MachineID = m_in.read_exact(16);
        o->put("MachineID", x->MachineID, false);
        m_in >> x->Droid1;  // TODO fix these, this representation is probably not correct, size ok
        m_in >> x->Droid2;
        // o->put("Droid", x->Droid1);
        // o->put("Droid", x->Droid2);
        m_in >> x->DroidBirth1;
        m_in >> x->DroidBirth2;
        // o->put("DroidBirth", x->DroidBirth1);
        // o->put("DroidBirth", x->DroidBirth2);
        m_out->put("TrackerDataBlock", std::move(o));
    }
    void vista_block(const LnkStruct::ExtraDataBlockHeader &h)  // TODO
    {
        auto o = LnkOutput::Stream::make();
        auto b = m_in.read_binary(h.BlockSize - 8);
        o->put("Bytes", b);
        m_out->put_debug("VistaAndAboveIDListDataBlock", std::move(o));
    }
    void unknown_block(const LnkStruct::ExtraDataBlockHeader &h)
    {
        auto o = LnkOutput::Stream::make();
        auto b = m_in.read_binary(h.BlockSize - 8);
        o->put("Bytes", b);
        m_out->put_debug("UnknownExtraDataBlock", std::move(o));
    }
public:
    ExtraData(FileStream &in):
        m_in(in)
    {
        if (m_in.is_eof()) {
            return;
        }
        while (true) {
            size_t pos = m_in.tellg();
            LnkStruct::ExtraDataBlockHeader h;
            in >> h.BlockSize;
            // spec says <4 but we need to read the signature unconditionally?
            if (h.BlockSize < 8) {
                break;
            }
            in >> h.BlockSignature;
            switch (h.BlockSignature) {
                case LnkStruct::ConsoleDataBlock::Signature:
                    console_data();
                    break;
                case LnkStruct::ConsoleFeDataBlock::Signature:
                    console_fe_data();
                    break;
                case LnkStruct::DarwinDataBlock::Signature:
                    darwin_data();
                    break;
                case LnkStruct::EnvVarDataBlock::Signature:
                    env_var_data();
                    break;
                case LnkStruct::IconEnvDataBlock::Signature:
                    icon_env_data();
                    break;
                case LnkStruct::KnownFolderDataBlock::Signature:
                    known_folder_data();
                    break;
                case LnkStruct::PropertyStoreDataBlock::Signature:
                    property_store(h);
                    break;
                case LnkStruct::ShimDataBlock::Signature:
                    shim_data(h);
                    break;
                case LnkStruct::SpecialFolderDataBlock::Signature:
                    special_folder();
                    break;
                case LnkStruct::TrackerDataBlock::Signature:
                    tracker_data();
                    break;
                case LnkStruct::VistaAndAboveIDListDataBlock::Signature:
                    vista_block(h);
                    break;
                default:
                    unknown_block(h);
                    break;
            }
            m_in.seekg(pos + h.BlockSize);
        };
    }
};

// end of sections }}}

struct ParserPriv
{
    std::unique_ptr<FileStream> m_in;
    std::list<Error>            m_warnings;
    LnkStruct::All              m_lnk;
    LnkOutput::StreamPtr        m_output;
};

Parser::Parser(const std::string &file_name)
{
    auto p = new ParserPriv();
    p->m_in = std::make_unique<FileStream>(file_name);
    p->m_output = std::make_unique<LnkOutput::Stream>();
    this->p = p;
}

void
Parser::parse()
{
    // output will be arranged in a different order from how the data is in the file
    // this is because LinkTargetIdList is 2nd and not interesting in most cases.
    auto p = (ParserPriv*)this->p;
    Header h(*p->m_in);
    LnkOutput::StreamPtr o_shid;
    LnkOutput::StreamPtr o_str;
    std::move(h.warnings().begin(), h.warnings().end(), p->m_warnings.end());
    p->m_lnk.header = std::move(h.data());
    // put the header first
    p->m_output->put("ShellLinkHeader", h.output());
    if (p->m_lnk.header.has_link_target_id_list()) {
        LinkTargetIdList idlist(*p->m_in);
        std::move(idlist.warnings().begin(), idlist.warnings().end(), p->m_warnings.end());
        // leave idlist for later
        o_shid = idlist.output();
        p->m_lnk.id_list = std::move(idlist);
    }
    if (p->m_lnk.header.has_link_info()) {
        LinkInfo li(*p->m_in);
        // put linkinfo second
        p->m_output->put("LinkInfo", li.output());
        std::move(li.warnings().begin(), li.warnings().end(), p->m_warnings.end());
        p->m_lnk.info = std::move(li.data());
    }
    StringData s(*p->m_in, p->m_lnk.header);
    o_str = s.output();
    // put stringdata third
    if (o_str->size() > 0) {
        p->m_output->put("StringData", std::move(o_str));
    }
    std::move(s.warnings().begin(), s.warnings().end(), p->m_warnings.end());
    p->m_lnk.string_data = std::move(s.data());
    // put shellids fourth
    if (o_shid != nullptr && o_shid->size() > 0) {
        p->m_output->put("LinkTargetIdList", std::move(o_shid));
    }
    ExtraData e(*p->m_in);
    if (p->m_output->size() > 0) {
        p->m_output->put("ExtraData", e.output());
    }
    std::move(e.warnings().begin(), e.warnings().end(), p->m_warnings.end());
}

Parser::~Parser()
{
    auto p = (ParserPriv*)this->p;
    delete p;
}

LnkStruct::All&
Parser::data()
{
    auto p = (ParserPriv*)this->p;
    return p->m_lnk;
}

const LnkOutput::StreamPtr
Parser::output()
{
    auto p = (ParserPriv*)this->p;
    return std::move(p->m_output);
}

};  // namespace LnkFile
