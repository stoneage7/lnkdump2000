
/*****
 * Part of LnkDump2000
 * Licence: GPL, version 3 or later (see COPYING file or https://www.gnu.org/licenses/gpl-3.0.txt)
 *****/

#ifndef __LNK_STRUCT__
#define __LNK_STRUCT__

#include <cinttypes>
#include <array>
#include <vector>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include <cstdio>  // for format

namespace LnkStruct {

// data types {{{

// GUID components are encoded as LE-LE-LE-BE-BE
class Guid
{
protected:
    std::array<uint8_t, 16> bytes;
    uint32_t comp1() const { return bytes[3] << 24 | bytes[2] << 16 | bytes[1] << 8 | bytes[0]; }
    uint16_t comp2() const { return bytes[5] << 8 | bytes[4]; }
    uint16_t comp3() const { return bytes[7] << 8 | bytes[6]; }
    uint16_t comp4() const { return bytes[8] << 8 | bytes[9]; }
    uint64_t comp5() const
    {
        uint64_t q = 0;
        q |= (uint64_t)bytes[10] << 40 | (uint64_t)bytes[11] << 32 | (uint64_t)bytes[12] << 24;
        q |= (uint64_t)bytes[13] << 16 | (uint64_t)bytes[14] <<  8 | (uint64_t)bytes[15];
        return q;
    }
public:
    constexpr Guid(std::array<uint8_t, 16> b): bytes(b) { }
    constexpr Guid(): bytes() { }
    std::string string() const
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "%08X-%04X-%04X-%04X-%012lX",
                 comp1(), comp2(), comp3(), comp4(), comp5());
        return std::string(buf);
    }
    bool operator==(const char *other) { return string() == other; }
};

class MSTimeProperty
{
protected:
    uint64_t data;
public:
    time_t unix_time() const
    {
        time_t u = (data / 10000000) - 11644473600;
        return u;
    }
    operator uint64_t& ()
    {
        return data;
    }
    operator uint64_t() const
    {
        return uint64_t(data);
    }
};

//! absolutely requires that the key-value pairs are sorted by key
//  i could constexpr that but there are only a few
template <class T>
class EnumeratedProperty
{
    using pair_type_ref = decltype(T::description.at(0));
    using pair_type = typename std::remove_reference<pair_type_ref>::type;
    using key_type = decltype(T::description.at(0).first);
protected:
    typename T::data_type value;
public:
    EnumeratedProperty(): value(0) { }
    EnumeratedProperty(typename T::data_type v): value(v) { }
    constexpr const char* describe(typename T::data_type x) const
    {
        for (size_t i = 0; i < T::description.size(); i++) {
            if (x == T::description[i].first) {
                return T::description[i].second;
            }
        }
        return nullptr;
    }
    bool valid()
    {
        for (auto p : T::description) {
            if (p.first == value)
                return true;
        }
        return false;
    }
    typename T::data_type get_value() const
    {
        return value;
    }
    operator typename T::data_type& ()
    {
        return value;
    }
};

template <class T>
class BitfieldProperty
{
protected:
    typename T::data_type bits;
public:
    BitfieldProperty(): bits(0) { }
    BitfieldProperty(typename T::data_type v): bits(v) { }
    static const typename T::data_type invalid_bits = 0;
    const char* describe(int bit) const
    {
        return T::description.at(bit);
    }
    constexpr static int find(const char* name)
    {
        std::string_view a{name};
        for (size_t i = 0; i < T::description.size(); i++) {
            auto d = T::description.at(i);
            if (d && a == std::string_view{d}) {
                return i;
            }
        }
        return -1;
    }
    constexpr int num_bits() const
    {
        return sizeof(bits) * 8;
    }
    typename T::data_type value() const
    {
        return bits;
    }
    bool value_of(int bit) const
    {
        return bit >= 0 ? (bits & (1 << bit)) : 0;
    }
    constexpr bool is_valid_bit(int bit) const
    {
        return (invalid_bits & (1 << bit)) == 0;
    }
    typename T::data_type get_invalid_bits() const
    {
        return (bits & T::invalid_bits);
    }
    bool verify() const
    {
        return ((bits & T::invalid_bits) == 0);
    }
    operator typename T::data_type& ()
    {
        return bits;
    }
};
// }}}

// section 2.1 (header) {{{
class ShowCommandTmpl
{
public:
    typedef uint32_t data_type;
    constexpr static std::array<std::pair<uint32_t, const char*>, 3> description = {{
        { 0x1, "SHOWNORMAL" },
        { 0x3, "SHOWMAXIMIZED" },
        { 0x7, "SHOWMINNOACTIVE" }
    }};
};
typedef EnumeratedProperty<ShowCommandTmpl> ShowCommand_t;

// section 2.1.3
class HotKeyLowTmpl
{
public:
    typedef uint8_t data_type;
    constexpr static std::array<std::pair<uint8_t, const char*>, 62> description = {{ 
        { 0x00, "None" },
        { 0x30, "0" },
        { 0x31, "1" },
        { 0x32, "2" },
        { 0x33, "3" },
        { 0x34, "4" },
        { 0x35, "5" },
        { 0x36, "6" },
        { 0x37, "7" },
        { 0x38, "8" },
        { 0x39, "9" },
        { 0x41, "A" },
        { 0x42, "B" },
        { 0x43, "C" },
        { 0x44, "D" },
        { 0x45, "E" },
        { 0x46, "F" },
        { 0x47, "G" },
        { 0x48, "H" },
        { 0x49, "I" },
        { 0x4A, "J" },
        { 0x4B, "K" },
        { 0x4C, "L" },
        { 0x4D, "M" },
        { 0x4E, "N" },
        { 0x4F, "O" },
        { 0x50, "P" },
        { 0x51, "Q" },
        { 0x52, "R" },
        { 0x53, "S" },
        { 0x54, "T" },
        { 0x55, "U" },
        { 0x56, "V" },
        { 0x57, "W" },
        { 0x58, "X" },
        { 0x59, "Y" },
        { 0x70, "F1" },
        { 0x71, "F2" },
        { 0x72, "F3" },
        { 0x73, "F4" },
        { 0x74, "F5" },
        { 0x75, "F6" },
        { 0x76, "F7" },
        { 0x77, "F8" },
        { 0x78, "F9" },
        { 0x79, "F10" },
        { 0x7A, "F11" },
        { 0x7B, "F12" },
        { 0x7C, "F13" },
        { 0x7D, "F14" },
        { 0x7E, "F15" },
        { 0x7F, "F16" },
        { 0x80, "F17" },
        { 0x81, "F18" },
        { 0x82, "F19" },
        { 0x83, "F20" },
        { 0x84, "F21" },
        { 0x85, "F22" },
        { 0x86, "F23" },
        { 0x87, "F24" },
        { 0x88, "NUM_LOCK" },
        { 0x89, "SCROLL_LOCK" }
    }};
};

class HotKeyHiTmpl
{
public:
    typedef uint8_t data_type;
    static const uint8_t invalid_bits = (0b11111U<<3);
    constexpr static std::array<const char *, 8> description = {
        "SHIFT",                                // 0
        "CONTROL",                              // 1
        "ALT",                                  // 2
        nullptr,                                // 3
        nullptr,                                // 4
        nullptr,                                // 5
        nullptr,                                // 6
        nullptr                                 // 7
    };
};

typedef EnumeratedProperty<HotKeyLowTmpl> HotKeyLow_t;
typedef BitfieldProperty<HotKeyHiTmpl> HotKeyHi_t;

// section 2.1.1
class LinkFlagsTmpl
{
public:
    typedef uint32_t data_type;
    static const uint32_t invalid_bits = (1U<<11) | (0b111111U<<26);
    constexpr static std::array<const char *, 32> description = {
        "HasLinkTargetIdList",                  // 0
        "HasLinkInfo",                          // 1
        "HasName",                              // 2
        "HasRelativePath",                      // 3
        "HasWorkingDir",                        // 4
        "HasArguments",                         // 5
        "HasIconLocation",                      // 6
        "IsUnicode",                            // 7
        "ForceNoLinkInfo",                      // 8
        "HasExpString",                         // 9
        "RunInSeparateProcess",                 // 10
        "Unused1",                              // 11
        "HasDarwinId",                          // 12
        "RunAsUser",                            // 13
        "HasExpIcon",                           // 14
        "NoPidIAlias",                          // 15
        "Unused2",                              // 16
        "RunWithShimLayer",                     // 17
        "ForceNoLinkTrack",                     // 18
        "EnableTargetMetadata",                 // 19
        "DisableLinkPathTracking",              // 20
        "DisableKnownFolderTracking",           // 21
        "DisableKnownFolderAlias",              // 22
        "AllowLinkToLink",                      // 23
        "UnaliasOnSave",                        // 24
        "PreferEnvironmentPath",                // 25
        nullptr,                                // 26
        nullptr,                                // 27
        nullptr,                                // 28
        nullptr,                                // 29
        nullptr,                                // 30
        nullptr                                 // 31
    };
};

// section 2.1.1
typedef BitfieldProperty<LinkFlagsTmpl> LinkFlags_t;

class FileAttributesTmpl
{
public:
    typedef uint32_t data_type;
    static const uint32_t invalid_bits = (1U<<3) | (1U<<6) | (0b11111111111111111U<<15);
    constexpr static std::array<const char *, 32> description = {
        "READONLY",                             // 0
        "HIDDEN",                               // 1
        "SYSTEM",                               // 2
        "Reserved1",                            // 3
        "DIRECTORY",                            // 4
        "ARCHIVE",                              // 5
        "Reserved2",                            // 6
        "NORMAL",                               // 7
        "TEMPORARY",                            // 8
        "SPARSE_FILE",                          // 9
        "REPARSE_POINT",                        // 10
        "COMPRESSED",                           // 11
        "OFFLINE",                              // 12
        "NOT_CONTENT_INDEXED",                  // 13
        "ENCRYPTED",                            // 14
        nullptr,                                // 15
        nullptr,                                // 16
        nullptr,                                // 17
        nullptr,                                // 18
        nullptr,                                // 19
        nullptr,                                // 20
        nullptr,                                // 21
        nullptr,                                // 22
        nullptr,                                // 23
        nullptr,                                // 24
        nullptr,                                // 25
        nullptr,                                // 26
        nullptr,                                // 27
        nullptr,                                // 28
        nullptr,                                // 29
        nullptr,                                // 30
        nullptr                                 // 31
    };
};
// section 2.1.2
typedef BitfieldProperty<FileAttributesTmpl> FileAttributes_t;

// section 2.1
struct ShellLinkHeader
{
    uint32_t            HeaderSize;
    //uint32_t            LinkCLSId[4];
    LinkFlags_t         LinkFlags;
    FileAttributes_t    FileAttributes;
    MSTimeProperty      CreationTime;
    MSTimeProperty      AccessTime;
    MSTimeProperty      WriteTime;
    uint32_t            FileSize;
    uint32_t            IconIndex;
    ShowCommand_t       ShowCommand;
    HotKeyLow_t         HotKeyLow;
    HotKeyHi_t          HotKeyHigh;
    uint16_t            Reversed1;
    uint32_t            Reserved2;
    uint32_t            Reserved3;
    bool has_link_info() const 
    {
        constexpr int info_bit = decltype(LinkFlags)::find("HasLinkInfo");
        static_assert(info_bit >= 0);
        return LinkFlags.value_of(info_bit);
    }
    bool has_link_target_id_list() const 
    {
        constexpr int lt_bit = decltype(LinkFlags)::find("HasLinkTargetIdList");
        static_assert(lt_bit >= 0);
        return LinkFlags.value_of(lt_bit);
    }
    bool has_unicode_strings() const
    {
        constexpr int uni_bit = decltype(LinkFlags)::find("IsUnicode");
        static_assert(uni_bit >= 0);
        return LinkFlags.value_of(uni_bit);
    }
    bool has_name_string() const
    {
        constexpr int name_bit = decltype(LinkFlags)::find("HasName");
        static_assert(name_bit >= 0);
        return LinkFlags.value_of(name_bit);
    }
    bool has_relpath_string() const
    {
        constexpr int relpath_bit = decltype(LinkFlags)::find("HasRelativePath");
        static_assert(relpath_bit >= 0);
        return LinkFlags.value_of(relpath_bit);
    }
    bool has_workdir_string() const
    {
        constexpr int workdir_bit = decltype(LinkFlags)::find("HasWorkingDir");
        static_assert(workdir_bit >= 0);
        return LinkFlags.value_of(workdir_bit);
    }
    bool has_args_string() const
    {
        constexpr int args_bit = decltype(LinkFlags)::find("HasArguments");
        static_assert(args_bit >= 0);
        return LinkFlags.value_of(args_bit);
    }
    bool has_iconloc_string() const
    {
        constexpr int iconloc_bit = decltype(LinkFlags)::find("HasIconLocation");
        static_assert(iconloc_bit >= 0);
        return LinkFlags.value_of(iconloc_bit);
    }
};
// end of section 2.1 }}}

// section 2.2, (shellids) {{{
struct LinkTargetIdList {
    uint16_t            IdListSize;
    struct ID {
        uint16_t                ItemIdSize;
        std::vector<uint8_t>    Data;
    };
    std::list<ID>       IdList;
};

/*
taken from:
https://github.com/libyal/libfwsi/blob/main/documentation/Windows%20Shell%20Item%20format.asciidoc
*/

class ShellId_x1F_SortIndex_Tmpl
{
public:
    typedef uint8_t data_type;
    constexpr static std::array<std::pair<uint8_t, const char*>, 9> description = {{
        { 0x00, "Internet Explorer" },
        { 0x42, "Libraries" },
        { 0x44, "Users" },
        { 0x48, "My Documents" },
        { 0x50, "My Computer" },
        { 0x58, "My Network Places" },
        { 0x60, "Recycle Bin" },
        { 0x68, "Internet Explorer" },
        { 0x80, "My Games" }
    }};
};
typedef EnumeratedProperty<ShellId_x1F_SortIndex_Tmpl> ShellId_x1F_SortIndex_t;

class ShellId_x30_Flags_Tmpl
{
public:
    typedef uint8_t data_type;
    static const uint8_t invalid_bits = 0b01111000;
    constexpr static std::array<const char *, 8> description = {
        "IsDirectory",                          // 0
        "IsFile",                               // 1
        "HasUnicodeStrings",                    // 2
        nullptr,                                // 3
        nullptr,                                // 4
        nullptr,                                // 5
        nullptr,                                // 6
        "HasClassId"                            // 7
    };
};
typedef BitfieldProperty<ShellId_x30_Flags_Tmpl> ShellId_x30_Flags_t;

class FATTime
{
private:
    uint32_t m_fat;
public:
    FATTime(uint32_t fat_time): m_fat(fat_time) { }
    FATTime(): m_fat(0) { }
    operator uint32_t&() { return m_fat; }
    time_t unix_time();
};

struct ShellId_x30_Struct
{
    ShellId_x30_Flags_t     Flags;
    uint8_t                 Unknown1;
    uint32_t                FileSize;
    FATTime                 ModifiedTime;
    uint16_t                Attributes;
    std::string             Name;
    std::string             SecondaryName;
    Guid                    ShellFolder;
    bool is_unicode() const
    {
        constexpr int uni_bit = decltype(Flags)::find("HasUnicodeStrings");
        static_assert(uni_bit >= 0);
        return Flags.value_of(uni_bit);
    }
};

class ShellId_x40_Type_Tmpl
{
public:
    typedef uint8_t data_type;
    constexpr static std::array<std::pair<uint8_t, const char*>, 7> description = {{
        { 0x01, "Domain/Workgroup Name" },
        { 0x02, "Server UNC Path" },
        { 0x03, "Share UNC Path" },
        { 0x06, "Microsoft Windows Network" },
        { 0x07, "Entire Network" },
        { 0x0D, "Network Places / Generic" },
        { 0x0E, "Network Places / Root" }
    }};
};
typedef EnumeratedProperty<ShellId_x40_Type_Tmpl> ShellId_x40_Type_t;

class ShellId_x40_Flags_Tmpl
{
public:
    typedef uint8_t data_type;
    static const uint8_t invalid_bits = 0b00111111;
    constexpr static std::array<const char *, 8> description = {
        nullptr,                                // 0
        nullptr,                                // 1
        nullptr,                                // 2
        nullptr,                                // 3
        nullptr,                                // 4
        nullptr,                                // 5
        "HasComments",                          // 6
        "HasDescription"                        // 7
    };
};
typedef BitfieldProperty<ShellId_x40_Flags_Tmpl> ShellId_x40_Flags_t;

struct ShellId_x40_Struct
{
    ShellId_x40_Type_t      Type;
    uint8_t                 Unknown1;
    ShellId_x40_Flags_t     Flags;
    std::string             Location;
    std::string             Description;
    std::string             Comments;
    bool has_comments() const
    {
        constexpr int combit = decltype(Flags)::find("HasComments");
        static_assert(combit >= 0);
        return Flags.value_of(combit);
    }
    bool has_description() const
    {
        constexpr int dscbit = decltype(Flags)::find("HasDescription");
        static_assert(dscbit >= 0);
        return Flags.value_of(dscbit);
    }
};

struct ShellId_x50_Struct
{
    uint8_t                 Unknown1;
    uint16_t                Unknown2;
    uint32_t                Unknown3;
    uint64_t                Unknown4;
    uint32_t                Unknown5;
    uint32_t                Unknown6;
    FATTime                 Timestamp;
    uint32_t                Unknown7;
    FATTime                 Timestamp2;
    uint64_t                Unknown8;
    uint32_t                Unknown9;   // string size?
    std::string             Unknown10;  // utf-16le
    uint32_t                Unknown11;  // string size?
    std::string             Unknown12;  // utf-16le
    uint32_t                FullPathSize;
    std::string             FullPath;   // utf-16le
    uint32_t                Unknown13;
    std::string             Unknown14;
};

class ShellId_x60_Flags_Tmpl
{
public:
    typedef uint8_t data_type;
    static const uint8_t invalid_bits = 0b00000000;
    constexpr static std::array<const char *, 8> description = {
        "Flag0x01",                             // 0
        "Flag0x02",                             // 1
        nullptr,                                // 2
        nullptr,                                // 3
        nullptr,                                // 4
        nullptr,                                // 5
        nullptr,                                // 6
        "IsUnicode"                             // 7
    };
};
typedef BitfieldProperty<ShellId_x60_Flags_Tmpl> ShellId_x60_Flags_t;

struct ShellId_x60_Struct
{
    ShellId_x60_Flags_t     Flags;
    uint16_t                DataSize;
    // if DataSize > 0
    uint32_t                Unknown1;
    uint32_t                Unknown2;
    MSTimeProperty          Timestamp;
    uint32_t                Unknown4;
    uint32_t                Unknown5;
    uint32_t                Unknown6;
    uint32_t                Unknown7;
    uint32_t                Unknown8;
    uint32_t                String1Bytes;
    std::string             FTPHostname;
    uint32_t                String2Bytes;
    std::string             FTPUser;
    uint32_t                String3Bytes;
    std::string             FTPPassword;
    std::string             URI;
    // there could be more data after this, including block BEEF0014
    bool is_unicode() const
    {
        constexpr int unibit = decltype(Flags)::find("IsUnicode");
        static_assert(unibit >= 0);
        return Flags.value_of(unibit);
    }

};

struct ShellId_x70_Struct
{
    uint8_t                 SortOrder;
    uint32_t                Unknown1;  // 10 bytes of unknowns, not exactly these types
    uint32_t                Unknown2;
    uint16_t                Unknown3;
    Guid                    GUID;
};

struct ShellId_x74_Struct
{
    static const uint32_t   Signature = 0x46534643;
    uint8_t                 Unknown1;
    uint16_t                DelegateOffset;
    uint32_t                SubShellItemSignature;
    uint16_t                SubShellItemSize;
    struct SubShellItem_ {
        uint8_t                 ClsType;
        uint8_t                 Unknown1;
        uint32_t                FileSize;
        FATTime                 ModifiedTime;
        uint16_t                FileAttributes;
        std::string             PrimaryName;
        uint16_t                Unknown2;
    }                       SubShellItem;
    Guid                    DelegateGuid;
    Guid                    DelegateClass;
    // continues with 0xBEEF0004
};

const char* shell_folder_guid_describe(const char* guid);  // clstype 0x1F, 0x30
const char* control_panel_guid_describe(const char* guid); // clstype 0x70

struct ShellId_BeefBase
{
    uint16_t                Size;
    uint16_t                Version;
    uint32_t                Signature;
};

class ShellId_Beef_Winver_Tmpl
{
public:
    typedef uint16_t data_type;
    constexpr static std::array<std::pair<uint16_t, const char*>, 4> description = {{
        { 0x0014, "Windows XP or 2003" },
        { 0x0026, "Windows Vista" },
        { 0x002A, "Windows 7, 8.0" },
        { 0x002E, "Windows 8.1, 10" }
    }};
};
typedef EnumeratedProperty<ShellId_Beef_Winver_Tmpl> ShellId_Beef_Winver_t;

struct ShellId_Beef0004
{
    static const uint32_t   Signature = 0xBEEF0004;
    FATTime                 CreationTime;
    FATTime                 AccessTime;
    ShellId_Beef_Winver_t   WindowsVersion;
    // version >= 7
    uint16_t                Unknown1;
    uint64_t                FileReference;
    uint64_t                Unknown2;
    // version >= 3
    uint16_t                LongStringSize;
    // version >= 9
    uint32_t                Unknown3;
    // version >= 8
    uint32_t                Unknown4;
    // version >= 3
    std::string             LongName;
    // version >= 3 && LongStringSize > 0
    std::string             LocalizedName;

};
// end of section 2.2 }}}

// section 2.3 (linkinfo) {{{
class LinkInfoFlagsTmpl
{
public:
    typedef uint32_t data_type;
    static const uint32_t invalid_bits = 0xFFFFFFFC;  // all but low 2 bits
    constexpr static std::array<const char *, 32> description = {
        "VolumeIDAndLocalBasePath",             // 0
        "CommonNetworkRelativeLinkAndPathSuffix"// 1
    };
};

typedef BitfieldProperty<LinkInfoFlagsTmpl> LinkInfoFlags_t;

// section 2.3
struct LinkInfoHeader
{
    uint32_t            LinkInfoSize;
    uint32_t            LinkInfoHeaderSize;
    LinkInfoFlags_t     LinkInfoFlags;
    uint32_t            VolumeIDOffset;
    uint32_t            LocalBasePathOffset;
    uint32_t            CommonNetworkRelativeLinkOffset;
    uint32_t            CommonPathSuffixOffset;
    uint32_t            LocalBasePathOffsetUnicode;         // optional
    uint32_t            CommonPathSuffixOffsetUnicode;      // optional

    int has_optional_fields() const
    {
        if (LinkInfoHeaderSize == 0x1C) {
            return 0;
        } else if (LinkInfoHeaderSize >= 0x24) {
            return 1;
        } else {
            return -1;
        }
    }
    bool has_volume_id_and_local_base_path() const
    {
        constexpr int volid_bit = decltype(LinkInfoFlags)::find("VolumeIDAndLocalBasePath");
        static_assert(volid_bit >= 0);
        return LinkInfoFlags.value_of(volid_bit) == 1;
    }
    bool has_common_network_relative_link() const
    {
        constexpr int cnr_bit =
            decltype(LinkInfoFlags)::find("CommonNetworkRelativeLinkAndPathSuffix");
        static_assert(cnr_bit >= 0);
        return LinkInfoFlags.value_of(cnr_bit) == 1;
    }
};

// section 2.3.1
class DriveTypeTmpl
{
public:
    typedef uint32_t data_type;
    constexpr static std::array<std::pair<uint32_t, const char*>, 7> description = {{
        { 0x0, "UNKNOWN" },
        { 0x1, "NO_ROOT_DIR" },
        { 0x2, "REMOVABLE" },
        { 0x3, "FIXED" },
        { 0x4, "REMOTE" },
        { 0x5, "CDROM" },
        { 0x6, "RAMDISK" }
    }};
};
typedef EnumeratedProperty<DriveTypeTmpl> DriveType_t;

// section 2.3.2, field CommonNetworkRelativeLinkFlags
class CNRLinkFlagsTmpl
{
public:
    typedef uint32_t data_type;
    static const uint32_t invalid_bits = 0xFFFFFFFC;  // all but low 2 bits
    constexpr static std::array<const char *, 32> description = {
        "ValidDevice",                          // 0
        "ValidNetType"                          // 1
    };
};
typedef BitfieldProperty<CNRLinkFlagsTmpl> CNRLinkFlags_t;

// section 2.3.2, field NetworkProviderType
class NetworkProviderTypeTmpl
{
public:
    typedef uint32_t data_type;
    constexpr static std::array<std::pair<uint32_t, const char*>, 41> description = {{
        { 0x001A0000, "AVID" },
        { 0x001B0000, "DOCUSPACE" },
        { 0x001C0000, "MANGOSOFT" },
        { 0x001D0000, "SERNET" },
        { 0X001E0000, "RIVERFRONT1" },
        { 0x001F0000, "RIVERFRONT2" },
        { 0x00200000, "DECORB" },
        { 0x00210000, "PROTSTOR" },
        { 0x00220000, "FJ_REDIR" },
        { 0x00230000, "DISTINCT" },
        { 0x00240000, "TWINS" },
        { 0x00250000, "RDR2SAMPLE" },
        { 0x00260000, "CSC" },
        { 0x00270000, "3IN1" },
        { 0x00290000, "EXTENDNET" },
        { 0x002A0000, "STAC" },
        { 0x002B0000, "FOXBAT" },
        { 0x002C0000, "YAHOO" },
        { 0x002D0000, "EXIFS" },
        { 0x002E0000, "DAV" },
        { 0x002F0000, "KNOWARE" },
        { 0x00300000, "OBJECT_DIRE" },
        { 0x00310000, "MASFAX" },
        { 0x00320000, "HOB_NFS" },
        { 0x00330000, "SHIVA" },
        { 0x00340000, "IBMAL" },
        { 0x00350000, "LOCK" },
        { 0x00360000, "TERMSRV" },
        { 0x00370000, "SRT" },
        { 0x00380000, "QUINCY" },
        { 0x00390000, "OPENAFS" },
        { 0X003A0000, "AVID1" },
        { 0x003B0000, "DFS" },
        { 0x003C0000, "KWNP" },
        { 0x003D0000, "ZENWORKS" },
        { 0x003E0000, "DRIVEONWEB" },
        { 0x003F0000, "VMWARE" },
        { 0x00400000, "RSFX" },
        { 0x00410000, "MFILES" },
        { 0x00420000, "MS_NFS" },
        { 0x00430000, "GOOGLE" }
    }};
};
typedef EnumeratedProperty<NetworkProviderTypeTmpl> NetProvider_t;

// section 2.3, rest of the link info structure
struct LinkInfoData
{
    struct VOLUMEID {
        uint32_t        Size;
        DriveType_t     DriveType;
        uint32_t        DriveSerialNumber;
        uint32_t        VolumeLabelOffset;
        uint32_t        VolumeLabelOffsetUnicode;
        std::string     VolumeLabel;
        std::string     VolumeLabelUnicode;  // optional
        bool has_unicode_label() const { return VolumeLabelOffset == 0x14; }
    }               VolumeID;
    std::string     LocalBasePath;
    struct CNR {
        uint32_t        Size;
        CNRLinkFlags_t  Flags;
        uint32_t        NetNameOffset;
        uint32_t        DeviceNameOffset;
        NetProvider_t   NetworkProviderType;
        uint32_t        NetNameOffsetUnicode;
        uint32_t        DeviceNameOffsetUnicode;
        std::string     NetName;
        std::string     DeviceName;
        std::string     NetNameUnicode;     // optional
        std::string     DeviceNameUnicode;  // optional
        bool has_device_name() const
        {
            constexpr int device_bit = decltype(Flags)::find("ValidDevice");
            return Flags.value_of(device_bit);
        }
        bool has_provider() const
        {
            constexpr int prov_bit = decltype(Flags)::find("ValidNetType");
            return Flags.value_of(prov_bit);
        }
        bool has_optional_fields() const { return NetNameOffset > 0x14; }
    }               CommonNetworkRelativeLink;
    std::string     CommonPathSuffix;
    std::string     LocalBasePathUnicode;
    std::string     CommonPathSuffixUnicode;
};

struct LinkInfo {
    LinkInfoHeader  header;
    LinkInfoData    data;
};
// end of section 2.3 }}}

// section 2.4 (stringdata) {{{
struct StringData {
    std::string     Name;
    std::string     RelativePath;
    std::string     WorkingDir;
    std::string     CommandLine;
    std::string     IconLocation;
    bool            UnicodeFlag;
};
// end of section 2.4 }}}

// section 2.5 (extradata) {{{
struct ExtraDataBlockHeader
{
    uint32_t        BlockSize;
    uint32_t        BlockSignature;
};

class FillAttributesTmpl
{
public:
    typedef uint16_t data_type;
    static const uint16_t invalid_bits = (0b1111111100000000U);
    constexpr static std::array<const char *, 16> description = {
        "FOREGROUND_BLUE",                      // 0
        "FOREGROUND_GREEN",                     // 1
        "FOREGROUND_RED",                       // 2
        "FOREGROUND_INTENSITY",                 // 3
        "BACKGROUND_BLUE",                      // 4
        "BACKGROUND_GREEN",                     // 5
        "BACKGROUND_RED",                       // 6
        "BACKGROUND_INTENSITY",                 // 7
        nullptr,                                // 8
        nullptr,                                // 9
        nullptr,                                // 10
        nullptr,                                // 11
        nullptr,                                // 12
        nullptr,                                // 13
        nullptr,                                // 14
        nullptr                                 // 15
    };
};
typedef BitfieldProperty<FillAttributesTmpl> FillAttributes_t;

class FontFamilyTmpl
{
public:
    typedef uint32_t data_type;
    constexpr static std::array<std::pair<uint32_t, const char*>, 6> description = {{
        { 0x0000, "DONTCARE" },
        { 0x0010, "ROMAN" },
        { 0x0020, "SWISS" },
        { 0x0030, "MODERN" },
        { 0x0040, "SCRIPT" },
        { 0x0050, "DECORATIVE" }
    }};
};
typedef EnumeratedProperty<FontFamilyTmpl> FontFamily_t;

class FontPitchTmpl
{
public:
    typedef uint32_t data_type;
    constexpr static std::array<std::pair<uint32_t, const char*>, 5> description = {{
        { 0x0000, "NONE" },
        { 0x0001, "FIXED_PITCH" },
        { 0x0002, "VECTOR" },
        { 0x0004, "TRUETYPE" },
        { 0x0008, "DEVICE" }
    }};
};
typedef EnumeratedProperty<FontPitchTmpl> FontPitch_t;

// amazing file format. font-family is both enumerated and bitwise-or with font-pitch.
// needs conversion to font-family enum and to font-pitch enum.
struct FontFamily2
{
public:
    uint32_t value;
    operator uint32_t&()  { return value; }
    FontFamily_t family() { return FontFamily_t(value & 0xFFFFFF00); }
    FontPitch_t  pitch()  { return FontPitch_t (value & 0x000000FF); }
};

struct ConsoleDataBlock
{
    static const uint32_t Signature = 0xA0000002;
    FillAttributes_t            FillAttributes;
    FillAttributes_t            PopupFillAttributes;
    int16_t                     ScreenBufferSizeX;
    int16_t                     ScreenBufferSizeY;
    int16_t                     WindowSizeX;
    int16_t                     WindowSizeY;
    int16_t                     WindowOriginX;
    int16_t                     WindowOriginY;
    uint32_t                    Reserved1;
    uint32_t                    Reserved2;
    uint32_t                    FontSize;
    FontFamily2                 FontFamily;
    uint32_t                    FontWeight;
    std::string                 FaceName;
    uint32_t                    CursorSize;
    uint32_t                    FullScreen;
    uint32_t                    QuickEdit;
    uint32_t                    InsertMode;
    uint32_t                    AutoPosition;
    uint32_t                    HistoryBufferSize;
    uint32_t                    NumberOfHistoryBuffers;
    uint32_t                    HistoryNoDup;
    std::array<uint32_t, 16>    ColorTable;
};

struct ConsoleFeDataBlock
{
    static const uint32_t Signature = 0xA0000004;
    uint32_t        CodePage;
};

struct DarwinDataBlock
{
    static const uint32_t Signature = 0xA0000006;
    std::string     DarwinDataAnsi;  // 260 bytes NUL-terminated
    std::string     DarwinDataUnicode;  // 520 bytes NUL-terminated
};

struct EnvVarDataBlock
{
    static const uint32_t Signature = 0xA0000001;
    std::string     TargetAnsi;  // 260 bytes NUL-terminated
    std::string     TargetUnicode;  // 520 bytes NUL-terminated
};

struct IconEnvDataBlock
{
    static const uint32_t Signature = 0xA0000007;
    std::string     TargetAnsi;  // 260 bytes NUL-terminated
    std::string     TargetUnicode;  // 520 bytes NUL-terminated
};

struct KnownFolderDataBlock
{
    static const uint32_t Signature = 0xA000000B;
    Guid            KnownFolderId;
    uint32_t        Offset;  // offset into IDList
};

struct PropertyStoreDataBlock
{
    static const uint32_t Signature = 0xA0000009;
    // oh my God TODO
};

struct ShimDataBlock
{
    static const uint32_t Signature = 0xA0000008;
    std::string     LayerName;  // variable size, depending on BlockSize
};

struct SpecialFolderDataBlock
{
    static const uint32_t Signature = 0xA0000005;
    uint32_t        SpecialFolderId;
    uint32_t        Offset;  // offset into IDList
};

struct TrackerDataBlock
{
    static const uint32_t Signature = 0xA0000003;
    uint32_t        Length;
    uint32_t        Version;
    std::string     MachineID;  // 16 bytes ANSI
    Guid            Droid1;
    Guid            Droid2;
    Guid            DroidBirth1;
    Guid            DroidBirth2;
};

struct VistaAndAboveIDListDataBlock
{
    static const uint32_t Signature = 0xA000000C;
    // another IDList TODO
};

// ExtraData structures will not be saved anywhere after parsing
struct ExtraDataPH
{
};
// end of section 2.5 }}}

typedef std::optional<LinkInfo>                         OptionalLinkInfo;
typedef std::optional<LinkTargetIdList>                 OptionalIdList;

struct All
{
    ShellLinkHeader         header;
    OptionalIdList          id_list;
    OptionalLinkInfo        info;
    StringData              string_data;
    bool has_id_list() const { return id_list.has_value(); }
    bool has_link_info() const { return info.has_value(); }
};

} // namespace LnkStruct

#endif // ifndef __LNK_STRUCT__




