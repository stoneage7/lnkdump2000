
/*****
 * Part of LnkDump2000
 * Licence: GPL, version 3 or later (see COPYING file or https://www.gnu.org/licenses/gpl-3.0.txt)
 *****/

#ifndef OUTPUT_H
#define OUTPUT_H

#include <memory>
#include <string>
#include <ostream>
#include <FL/Fl_Browser.H>
#include "encoding.h"
#include "struct.h"

#include <iostream>

namespace LnkOutput {

class BasicValue;
class IntegerValue;
class StringValue;
class EnumeratedValue;
class BitValue;
class ArrayValue;
class StructValue;
class Stream;

typedef std::unique_ptr<Stream>         StreamPtr;
typedef std::unique_ptr<BasicValue>     BasicValuePtr;

enum InfoLevel { NORMAL, DEBUG };

void        dump_yaml(std::ostream& out, const StreamPtr& stream, CodecPtr codec,
                      const std::string& name, InfoLevel level);
void        dump_fltk(Fl_Browser* widget, const StreamPtr& stream, CodecPtr codec,
                      InfoLevel level);

class OutputVisitor
{
public:
    virtual void visit(const IntegerValue* f) = 0;
    virtual void visit(const StringValue* f) = 0;
    virtual void visit(const EnumeratedValue* f) = 0;
    virtual void visit(const BitValue* f) = 0;
    virtual void visit(const ArrayValue* f) = 0;
    virtual void visit(const StructValue* f) = 0;
};


class BasicValue
{
protected:
    // name should always point to a literal string (static lifetime)
    const char*         m_name;
    InfoLevel           m_level;
    BasicValue(const char* name): m_name(name), m_level(NORMAL) { }
public:
    virtual void accept(OutputVisitor* v) const = 0;
    virtual const char* name() const { return m_name; }
    void level(InfoLevel l) { m_level = l; }
    InfoLevel level() const { return m_level; }
    virtual ~BasicValue() { }
};

class IntegerValue: public BasicValue
{
public:
    enum PreferForm { Decimal, Hex, FileSize, UnixTime };
protected:
    int64_t             m_value;  // int64_t can store any integer value in the spec
    PreferForm          m_form;
public:
    virtual void accept(OutputVisitor* v) const { v->visit(this); }
    int64_t value() const { return m_value; }
    PreferForm form() const { return m_form; }
    IntegerValue(const char* name, int64_t value, PreferForm form = Decimal):
        BasicValue(name), m_value(value), m_form(form) { }
};

class StringValue: public BasicValue
{
protected:
    std::string         m_string;
    bool                m_utf8;
public:
    virtual void accept(OutputVisitor* v) const { v->visit(this); }
    const std::string& string() const { return m_string; }
    bool is_utf8() const { return m_utf8; }
    StringValue(const char* name, const std::string& string, bool is_utf8):
        BasicValue(name), m_string(string), m_utf8(is_utf8) { }
};

class EnumeratedValue: public BasicValue
{
protected:
    EnumeratedValue(const char* name): BasicValue(name) { }
public:
    virtual const char* describe() const = 0;
    virtual int64_t value() const = 0;
};

template <class T>
class ConcreteEnumeratedValue: public EnumeratedValue
{
    using ValueType = LnkStruct::EnumeratedProperty<T>;
protected:
    ValueType           m_value;
public:
    virtual void accept(OutputVisitor* v) const { v->visit(this); }
    virtual const char* describe() const { return m_value.describe(m_value.get_value()); }
    virtual int64_t value() const { return m_value.get_value(); }
    ConcreteEnumeratedValue(const char* name, ValueType value):
        EnumeratedValue(name), m_value(value) { }
};

class BitValue: public BasicValue
{
protected:
    BitValue(const char* name): BasicValue(name) { }
public:
    virtual int num_bits() const = 0;
    virtual uint64_t value() const = 0;
    virtual bool value_of(int bit) const = 0;
    virtual bool is_valid_bit(int bit) const = 0;
    virtual const char* describe(int bit) const = 0;
};

template <class T>
class ConcreteBitValue: public BitValue
{
    using ValueType = LnkStruct::BitfieldProperty<T>;
protected:
    ValueType           m_value;
public:
    virtual void accept(OutputVisitor* v) const { v->visit(this); }
    virtual int num_bits() const { return m_value.num_bits(); }
    virtual uint64_t value() const { return m_value.value(); }
    virtual bool value_of(int bit) const { return m_value.value_of(bit); }
    virtual bool is_valid_bit(int bit) const { return m_value.is_valid_bit(bit); }
    virtual const char* describe(int bit) const { return m_value.describe(bit); }
    ConcreteBitValue(const char* name, ValueType value): BitValue(name), m_value(value) { }
};

class ArrayValue: public BasicValue
{
protected:
    ArrayValue(const char* name): BasicValue(name) { }
public:
    virtual size_t size() const = 0;
    virtual int64_t at(size_t i) const = 0;
    virtual size_t element_size() const = 0;
};

template <class T, size_t N>
class ConcreteArrayValue: public ArrayValue
{
protected:
    std::array<T, N>    m_array;
public:
    ConcreteArrayValue(const char* name, const std::array<T, N>& other):
        ArrayValue(name),  m_array(other) { }
    virtual size_t size() const { return m_array.size(); }
    virtual int64_t at(size_t i) const { return (int64_t)m_array.at(i); }
    virtual size_t element_size() const { return sizeof(T); }
};

template <class T>
class ConcreteVectorValue: public ArrayValue
{
protected:
    std::vector<T>      m_vec;
public:
    ConcreteVectorValue(const char* name, const std::vector<T>& other):
        ArrayValue(name), m_vec(other) { }
    virtual void accept(OutputVisitor* v) const { v->visit(this); }
    virtual size_t size() const { return m_vec.size(); }
    virtual int64_t at(size_t i) const { return (int64_t)m_vec.at(i); }
    virtual size_t element_size() const { return sizeof(T); }
};

class StructValue: public BasicValue
{
protected:
    StreamPtr               m_nested;
public:
    virtual void accept(OutputVisitor* v) const { v->visit(this); }
    void nest(OutputVisitor* v, InfoLevel l) const;
    StructValue(const char* name, StreamPtr nested):
        BasicValue(name), m_nested(std::move(nested)) { }
};

class Stream
{
protected:
    std::list<BasicValuePtr> m_list;
public:
    Stream() { }

    static StreamPtr make() { return std::make_unique<Stream>(); }

    void put(const char* name, int64_t value, IntegerValue::PreferForm form = IntegerValue::Decimal)
    {
        auto tmp = std::make_unique<IntegerValue>(name, value, form);
        m_list.emplace_back(std::move(tmp));
    }

    void put(const char* name, const std::string& s, bool is_utf8)
    {
        auto tmp = std::make_unique<StringValue>(name, s, is_utf8);
        m_list.emplace_back(std::move(tmp));
    }

    template <class T>
    void put(const char* name, const LnkStruct::EnumeratedProperty<T>& value)
    {
        auto tmp = std::make_unique<ConcreteEnumeratedValue<T> >(name, value);
        m_list.emplace_back(std::move(tmp));
    }

    template <class T>
    void put(const char* name, const LnkStruct::BitfieldProperty<T>& value)
    {
        auto tmp = std::make_unique<ConcreteBitValue<T> >(name, value);
        m_list.emplace_back(std::move(tmp));
    }

    void put(const char* name, StreamPtr nested)
    {
        auto tmp = std::make_unique<StructValue>(name, std::move(nested));
        m_list.emplace_back(std::move(tmp));
    }

    void put(const char* name, LnkStruct::MSTimeProperty time)
    {
        auto tmp = std::make_unique<IntegerValue>(name, time.unix_time(), IntegerValue::UnixTime);
        m_list.emplace_back(std::move(tmp));
    }

    void put(const char* name, LnkStruct::FATTime time)
    {
        auto tmp = std::make_unique<IntegerValue>(name, time.unix_time(), IntegerValue::UnixTime);
        m_list.emplace_back(std::move(tmp));
    }

    void put(const char* name, LnkStruct::Guid guid)
    {
        put(name, guid.string(), true);
    }

    template <class T, size_t N>
    void put(const char* name, const std::array<T, N>& array)
    {
        auto tmp = std::make_unique<ConcreteArrayValue<T, N> >(name, array);
        m_list.emplace_back(std::move(tmp));
    }

    template <class T>
    void put(const char* name, const std::vector<T>& vec)
    {
        auto tmp = std::make_unique<ConcreteVectorValue<T> >(name, vec);
        m_list.emplace_back(std::move(tmp));
    }

    template <class... Args>
    void put_debug(const char* name, Args...x)
    {
        put(name, x...);
        auto& tmp = m_list.back();
        tmp->level(DEBUG);
    }

    template <class... Args>
    void put_debug(const char* name, StreamPtr nested)
    {
        put(name, std::move(nested));
        auto& tmp = m_list.back();
        tmp->level(DEBUG);
    }

    void accept(OutputVisitor *v, InfoLevel l) const
    {
        for (const BasicValuePtr& field : m_list) {
            if ((l == NORMAL && field->level() == NORMAL) ||
                (l == DEBUG))
            {
                field->accept(v);
            }
        }
    }

    int size() const
    {
        return m_list.size();
    }
};

inline void
StructValue::nest(LnkOutput::OutputVisitor* v, InfoLevel l) const
{
    m_nested->accept(v, l);
}

};  // namespace LnkOutput

#endif  // OUTPUT_H
