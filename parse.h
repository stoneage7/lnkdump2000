
/*****
 * Part of LnkDump2000
 * Licence: GPL, version 3 or later (see COPYING file or https://www.gnu.org/licenses/gpl-3.0.txt)
 *****/

#ifndef LNKFILE_H
#define LNKFILE_H

#include "output.h"
#include "struct.h"

namespace LnkParser {

class FileStream;

class Error: public std::runtime_error
{
public:
    explicit Error(const char *msg): std::runtime_error(msg) { }
    explicit Error(std::string msg): std::runtime_error(msg) { }
    static Error format(const char *fmt, ...);
};

class Parser final
{
private:
    void*                       p;

public:
    Parser(const std::string &file_name);
    ~Parser();
    void                        parse();
    LnkStruct::All&             data();
    const LnkOutput::StreamPtr  output();
};

};

#endif // LNKFILE_H
