
/*****
 * Part of LnkDump2000
 * Licence: GPL, version 3 or later (see COPYING file or https://www.gnu.org/licenses/gpl-3.0.txt)
 *****/

#include "output.h"
#include <ctime>
#include <list>
#include <string>
#include <iostream>
#include <FL/Fl.H>
#include <FL/Fl_Browser.H>

namespace LnkOutput {

static std::string
iso8601_time(time_t unix_time)
{
    char buf[256] = "";
    std::tm tm = *std::gmtime(&unix_time);
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
    return std::string(buf);
}

static std::string
human_time(time_t unix_time)
{
    char buf[256] = "";
    ctime_r(&unix_time, buf);
    return std::string(buf);
}

static const char*
safe_string(const char* d)
{
    return d ? d : "Unknown";
}

static std::string
bitfield_as_string(const BitValue* f)
{
    bool first = true;
    std::string s;
    s.reserve(64);
    s.append("[ ");
    for (int i = 0; i < f->num_bits(); i++) {
        if (f->value_of(i) != 0) {
            if (!first) {
                s.append(", ");
            }
            s.append(safe_string(f->describe(i)));
            first = false;
        }
    }
    s.append(" ]");
    return s;
}

static std::string
hex(int64_t value)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "0x%lX", (uint64_t)value);
    return std::string{buf};
}

static std::string
hex(const ArrayValue* f)
{
    const char* fmt;
    if (f->size() == 0) {
        return {};
    }
    if (f->element_size() == 1) {
        fmt = "%02x";
    } else if (f->element_size() == 2) {
        fmt = "%04x";
    } else if (f->element_size() == 4) {
        fmt = "%08x";
    } else {
        return {};
    }
    char buf[f->element_size() * 2 + 1];
    std::string s;
    s.reserve(f->size() * (f->element_size() * 2 + 1) - 1);
    for (size_t i = 0; i < f->size() - 1; i++) {
        snprintf(buf, sizeof(buf), fmt, f->at(i));
        s.append(buf);
        s.append(" ");
    }
    snprintf(buf, sizeof(buf), fmt, f->at(f->size() - 1));
    s.append(buf);
    return s;
}

static std::string
as_file_size(int64_t value)
{
    if (value < 0) {
        return std::to_string(value);
    } else if (value < 1000) {
        char buf[128];
        snprintf(buf, sizeof(buf), "%ld bytes", value);
        return std::string{buf};
    } else {
        static const char* suf[] = {"kiB", "MiB", "GiB", "TiB"};
        static const double denom[] = {1024L, 1024L*1024, 1024L*1024*1024, 1024L*1024*1024*1024};
        int j = 0;
        for (int i = 0; i < int(sizeof(suf)/sizeof(suf[0])); i++) {
            j = i;
            if ((double)value / denom[j] < 1000.) {
                break;
            }
        }
        double human_size = (double)value / denom[j];
        char buf[128];
        snprintf(buf, sizeof(buf), "%.1f %s (%ld bytes)", human_size, suf[j], value);
        return std::string{buf};
    }
}

// YAML
//------------------------------------------------------------------------

class YamlDumper: public OutputVisitor
{
protected:
    std::ostream&   m_out;
    int             m_level;
    CodecPtr        m_codec;
    InfoLevel       m_info_level;

    void indent()
    {
        for (int i = 0; i < m_level; i++) {
            m_out << "  ";
        }
    }

    static std::string escape(const std::string& s)
    {
        size_t pos = 0;
        std::string r;
        r.append("\"");
        std::pair<codepoint_t, size_t> cp;
        for (pos = 0; cp = utf8_codepoint(s, pos), cp.second > 0; pos += cp.second) {
            auto c = cp.first;
            if (c == 0x5C)
            {
                // utf-8 backslash -> escape
                r.append("\\\\");
            }
            else if ((c != 0) && (c >= 0x20) && (c <= 0x7E) && (c != 0x5C) && (c != 0x22))
            {
                // 7bit alphanumeric excluding backslash and double quote
                r.push_back(cp.first);
            }
            else
            {
                char tmp[16];
                snprintf(tmp, sizeof(tmp), "\\u0%04x", c);
                r.append(tmp);
            }
        }
        r.append("\"");
        return r;
    }

public:
    YamlDumper(std::ostream &out, CodecPtr c, InfoLevel l):
        m_out(out), m_level(0), m_codec(c), m_info_level(l) { }

    void dump(const StreamPtr& stream, const std::string& name)
    {
        m_level = 0;
        m_out << "---" << std::endl;
        if (name.length() > 0) {
            m_out << "File: " << escape(name) << std::endl;
        }
        m_out << std::endl;
        stream->accept(this, m_info_level);
        m_out << "..." << std::endl;
    }

    virtual void visit(const IntegerValue* f)
    {
        indent();
        std::string s;
        switch (f->form()) {
            case IntegerValue::UnixTime:
                s = iso8601_time(f->value());
                break;
            default:
                s = std::to_string(f->value());
        }
        m_out << f->name() << ": " << s << std::endl;
    }

    virtual void visit(const StringValue* f)
    {
        indent();
        m_out << f->name() << ": ";
        if (f->is_utf8()) {
            m_out << escape(f->string());
        } else {
            m_out << (m_codec ? escape(m_codec->string(f->string())) : escape(f->string()));
        }
        m_out << std::endl;
    }

    virtual void visit(const EnumeratedValue* f)
    {
        indent();
        m_out << f->name() << ": " << safe_string(f->describe()) << std::endl;
        indent();
        m_out << f->name() << "_Numeric: " << f->value() << std::endl;
    }

    virtual void visit(const BitValue* f)
    {
        indent();
        m_out << f->name() << ": " << bitfield_as_string(f) << std::endl;
        indent();
        m_out << f->name() << "_Numeric: " << f->value() << std::endl;
    }

    virtual void visit(const ArrayValue* f)
    {
        indent();
        m_out << f->name() << ": " << hex(f) << std::endl;
    }

    virtual void visit(const StructValue* f)
    {
        indent();
        m_out << f->name() << ":" << std::endl;
        m_level++;
        f->nest(this, m_info_level);
        m_level--;
    }
};

void dump_yaml(std::ostream& out, const StreamPtr& stream, CodecPtr codec,
               const std::string& name, InfoLevel level)
{
    YamlDumper d(out, codec, level);
    d.dump(stream, name);
}

// FLTK
//------------------------------------------------------------------------

class FltkDumper: public OutputVisitor
{
protected:
    Fl_Browser *            m_widget;
    std::list<std::string>  m_path;
    CodecPtr                m_codec;
    InfoLevel               m_info_level;

public:
    FltkDumper(Fl_Browser* w, CodecPtr c, InfoLevel l):
        m_widget(w), m_codec(c), m_info_level(l) { };

    void dump(const StreamPtr& stream)
    {
        m_path.clear();
        stream->accept(this, m_info_level);
    }

    virtual void visit(const IntegerValue* f)
    {
        std::string s;
        s.reserve(64);
        s.append(f->name());
        s.append("\t");
        switch (f->form()) {
            case IntegerValue::Decimal:
                s.append(std::to_string(f->value()));
                break;
            case IntegerValue::Hex:
                s.append(hex(f->value()));
                break;
            case IntegerValue::FileSize:
                s.append(as_file_size(f->value()));
                break;
            case IntegerValue::UnixTime:
                s.append(human_time(f->value()));
                break;
        }
        m_widget->add(s.c_str());
    }

    virtual void visit(const StringValue* f)
    {
        std::string s;
        s.reserve(64);
        s.append(f->name());
        s.append("\t");
        if (f->is_utf8()) {
            s.append(f->string());
        } else {
            s.append(m_codec ? m_codec->string(f->string()) : f->string());
        }
        m_widget->add(s.c_str());
    }

    virtual void visit(const EnumeratedValue* f)
    {
        std::string s;
        s.reserve(64);
        s.append(f->name());
        s.append("\t");
        s.append(hex(f->value()));
        s.append(" (");
        s.append(safe_string(f->describe()));
        s.append(")");
        m_widget->add(s.c_str());
    }

    virtual void visit(const BitValue* f)
    {
        std::string s;
        s.reserve(64);
        s.append(f->name());
        s.append("\t");
        s.append(hex(f->value()));
        s.append(" ");
        s.append(bitfield_as_string(f));
        m_widget->add(s.c_str());
    }

    virtual void visit(const ArrayValue* f)
    {
        std::string s{f->name()};
        s.append("\t");
        s.append(hex(f));
        m_widget->add(s.c_str());
    }

    virtual void visit(const StructValue* f)
    {
        std::string s;
        s.reserve(64);
        m_widget->add("");
        m_path.emplace_back(std::string(f->name()));
        s.append("/");
        for (const auto& e : m_path) {
            s.append(e);
            s.append("/");
        }
        m_widget->add(s.c_str());
        f->nest(this, m_info_level);
        m_path.pop_back();
    }
};

void
dump_fltk(Fl_Browser* widget, const StreamPtr& stream, CodecPtr codec, InfoLevel level)
{
    FltkDumper d(widget, codec, level);
    d.dump(stream);
}

}  // namespace LnkOutput
