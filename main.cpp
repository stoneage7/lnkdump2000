
/*****
 * Part of LnkDump2000
 * Licence: GPL, version 3 or later (see COPYING file or https://www.gnu.org/licenses/gpl-3.0.txt)
 *****/

// Headers that are generated from FLUID sources
#include "about.h"
#include "blank.h"
#include "lnk.h"
#include "config.h"

// main project
#include "parse.h"
#include "main.h"
#include "output.h"
#include "struct.h"
#include "themes.h"
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Text_Buffer.H>
#include <Fl/Fl_PNG_Image.H>

// std
#include <filesystem>
#include <getopt.h>
#include <list>
#include <string>
#include <iostream>

// detect console stdin
#include <unistd.h>

// globals {{{
MainGui*            state;
CodecFactory        codecs;

static const int    ERROR_USAGE = 2;
static const int    ERROR_PARSE = 1;
static const int    MAX_GUI_ERROR_MSGS = 5;

static const char *about_blurb =
    "lnkump2000 " VERSION "\n"
    "This program is free software: you can redistribute it and/or modify\n"
    "it under the terms of the GNU General Public License as published by\n"
    "the Free Software Foundation, either version 3 of the License, or\n"
    "(at your option) any later version.\n"
    "See file COPYING or https://www.gnu.org/licenses/gpl-3.0.txt\n";

static const char *usage_text =
    "Command line options:\n"
    "   -h, --help          show this message and exit\n"
    "   -a, --all           show more fields\n"
    "   -y, --yaml          show output in YAML on the console\n"
    "   -g, --gui           show output on GUI\n"
    "   -c, --codepage X    if the file contains non-Unicode strings,\n"
    "                       convert them using this codepage\n"
    "Return value is always 0 if GUI is showing,\n"
    "otherwise 0 for success, 1 for parse error, 2 for command line error.\n";

static const char *shill_text =
    "You can send me some crypto if you're a cool hacker:\n"
    "XMR: 82tcaucC9ZHMSdT86omiTpVN2oQRghkHcRmRWhpLP1xDY2XMdDFRH77Jiuwh1Mdq6Y2M5mfBvwWGGCNyNhMWziPESWt7zuu\n"
    "BTC: 15wkwFMSYp7VGEoJ4U6WNNkgwjw8i39fFH\n\n";

int open_files(const std::list<std::string>& names);
// }}}

// command line {{{
static struct {
    LnkOutput::InfoLevel    default_info_level = LnkOutput::NORMAL;
    bool                    yaml = false;
    bool                    gui = false;
    std::string             codepage;
    std::list<std::string>  files;
} command_line;

static void
usage()
{
    std::cerr << about_blurb << usage_text;
}

static bool
cmdline(int argc, char **argv)
{
    static struct option long_options[] = {
        {"help",            no_argument, 0,             'h'},
        {"all",             no_argument, 0,             'a'},
        {"yaml",            no_argument, 0,             'y'},
        {"gui",             no_argument, 0,             'g'},
        {"codepage",        required_argument, 0,       'c'},
        {NULL,              0, 0, 0}
    };
    while (true) {
        int c = getopt_long(argc, argv, "haygc:", long_options, nullptr);
        if (c == -1) {
            break;
        }
        switch (c) {
            case 'h':
                usage();
                exit(0);
            case 'a':
                command_line.default_info_level = LnkOutput::DEBUG;
                break;
            case 'y':
                command_line.yaml = true;
                break;
            case 'g':
                command_line.gui = true;
                break;
            case 'c':
                errno = 0;
                command_line.codepage = std::string(optarg);
                if (errno != 0) {
                    return false;
                }
                break;
            default:
                return false;
        }
    }
    while (optind < argc) {
        auto canon = std::filesystem::weakly_canonical(argv[optind++]).string();
        command_line.files.emplace_back(canon);
    }
    return true;
}
// }}}

// GUI {{{
std::optional<std::string>
urldecode(const std::string_view& v)
{
    size_t pos = 0;
    size_t prev = 0;
    std::string r;
    while ((pos = v.find("%", prev)) != std::string::npos) {
        if (pos + 1 < v.size()) {
            r.append(v.substr(prev, pos - prev));
            char hex[3];
            char *p = nullptr;
            hex[0] = v[pos];
            hex[1] = v[pos+1];
            hex[2] = '\0';
            long l = strtoul(hex, &p, 16);
            if (p) {
                r.push_back((char)l);
            } else {
                return std::nullopt;
            }
            prev = pos + 1;
        } else {
            return std::nullopt;
        }
    }
    r.append(v.substr(prev, v.size() - prev));
    return r;
}

static std::list<std::string>
parse_drag_drop(const char* s)
{
    // drag and drop is a string of file:/// URIs, url-encoded and separated by newlines
    std::list<std::string> ret;
    std::istringstream is(s);
    for (std::string line; std::getline(is, line); ) {
        if (line.starts_with("file://")) {
            auto filename = urldecode(line.substr(7, line.size()));
            if (filename.has_value()) {
                ret.push_back(std::move(filename.value()));
            }
        }
    }
    return ret;
}

class MainPriv final
{
private:
    Fl_Text_Buffer *            m_aboutText;
    Fl_Double_Window *          m_aboutWindow;
    Fl_Double_Window *          m_blankWindow;
    Fl_PNG_Image *              m_icon;

public:
    void
    gui_open_file(LnkOutput::StreamPtr o, const std::string &name)
    {
        LnkWindow *w = make_window();
        static int widths[] = {200, 100, 0};
        for (auto const &e : codec_defs) {
            w->m_codepages->add(e.first, nullptr, nullptr, nullptr, 0);
        }
        CodecPtr c = codecs.get(command_line.codepage);
        if (c != nullptr) {
            w->m_codepages->value(c->index());
        }
        w->m_content->column_widths(widths);
        if (command_line.default_info_level == LnkOutput::DEBUG) {
            w->m_show_all->value(1);
        }
        LnkOutput::dump_fltk(w->m_content, o, c, w->info_level());
        w->m_title->value(name.c_str());
        w->copy_label(name.c_str());
        w->show();
        w->m_output = std::move(o);
        w->icon(m_icon);
        m_blankWindow->hide();
    }

    friend class MainGui;
};

MainGui::MainGui()
{
    p = new MainPriv();
    Fl_Window::default_xclass("lnkdump2000");
    Themes::use_aero_theme();
    char* appdir = std::getenv("APPDIR");
    if (appdir) {
        std::filesystem::path ip(appdir);
        ip += "/lnkdump2k.png";
        p->m_icon = new Fl_PNG_Image(ip.c_str());
        if (!p->m_icon->fail()) {
            Fl_Window::default_icon(p->m_icon);
        }
    }
    p->m_aboutText = new Fl_Text_Buffer();
    p->m_aboutText->text(about_blurb);
    p->m_aboutText->append(shill_text);
    p->m_aboutText->append(usage_text);
    p->m_aboutWindow = about_window();
    ui_about_text->buffer(p->m_aboutText);
    p->m_blankWindow = blank_window();
}

MainGui::~MainGui()
{
    if (p->m_aboutWindow)   delete p->m_aboutWindow;
    if (p->m_blankWindow)   delete p->m_blankWindow;
    if (p->m_aboutText)     delete p->m_aboutText;
    if (p->m_icon)          delete p->m_icon;
    delete p;
}


void MainGui::open_file(LnkOutput::StreamPtr o, const std::string& name)
{
    p->gui_open_file(std::move(o), name);
}


void
MainGui::open_file_cb()
{
    char *name = fl_file_chooser(nullptr, "*.lnk", nullptr, 0);
    if (name) {
        open_files({name});
    }
}

void
MainGui::context_menu_cb(void *v)
{
    static const Fl_Menu_Item m[] = {
        { "&Copy Value", FL_CTRL+'c', nullptr, nullptr, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0 }
    };
    if (Fl::event_button() == 3) {
        const Fl_Menu_Item* picked = m->popup(Fl::event_x(), Fl::event_y());
        if (picked == &m[0]) {
            LnkWindow *mainwin = (LnkWindow *)v;
            int selected = mainwin->m_content->value();
            const char *value = mainwin->m_content->text(selected);
            if (value) {
                std::string_view s(value);
                size_t tab = s.find("\t");
                if (tab != std::string::npos) {
                    std::string subs(s.substr(tab+1, s.npos));
                    Fl::copy(subs.c_str(), subs.length(), 1);
                }
            }
        }
    }
}

void
MainGui::change_codepage_cb(void *v)
{
    LnkWindow *w = (LnkWindow*)v;
    int idx = w->m_codepages->value();
    CodecPtr c = idx >= 0 ? codecs.get(idx) : nullptr;
    w->m_content->clear();
    LnkOutput::dump_fltk(w->m_content, w->m_output, c, w->info_level());
}


void
MainGui::show_all_cb(void *v)
{
    LnkWindow *w = (LnkWindow*)v;
    int idx = w->m_codepages->value();
    CodecPtr c = idx >= 0 ? codecs.get(idx) : nullptr;
    w->m_content->clear();
    LnkOutput::dump_fltk(w->m_content, w->m_output, c, w->info_level());
}

void
MainGui::close_file_cb(void *w)
{
    delete (Fl_Double_Window*)w;
}

void
MainGui::open_blank()
{
    p->m_blankWindow->show();
    p->m_blankWindow->set_modal();
}

void
MainGui::close_blank()
{
    p->m_blankWindow->hide();
}

void
MainGui::open_about()
{
    p->m_aboutWindow->show();
    p->m_aboutWindow->set_modal();
}

void
MainGui::close_about()
{
    p->m_aboutWindow->hide();
}

void
MainGui::error_msg(const std::string& msg)
{
    fl_alert("%s", msg.c_str());
}
// }}}

// dnd window / lnkwindow / blankwindow {{{
int
DNDWindow::handle(int event)
{
    int result = 0;
    switch (event) {
        case FL_ENTER:
            // during DND the window will also receive FL_ENTER event. default implementation
            // forwards this event to child widgets, which would interrupt the DND.
            if (m_dnd) {
                result = 1;
            } else {
                result = Fl_Double_Window::handle(event);
            }
            break;
        case FL_LEAVE:
        case FL_DND_LEAVE:
            if (m_dnd) {
                m_dnd = false;
                result = 1;
            } else {
                result = Fl_Double_Window::handle(event);
            }
            break;
        case FL_DND_ENTER:
            m_dnd = true;
            result = 1;
            break;
        case FL_DND_DRAG:
            m_dnd = true;
            result = 1;
            break;
        case FL_DND_RELEASE:
            m_dnd = false;
            result = 1;
            break;
        case FL_PASTE:
        {
            auto files = parse_drag_drop(Fl::event_text());
            open_files(files);
            break;
        }
        default:
            result = Fl_Double_Window::handle(event);
    }
    return result;
}
// }}}

// main logic {{{
int
open_files(const std::list<std::string>& names)
{
    std::list<std::string> error_names;
    std::list<std::string> error_msgs;
    for (auto& n : names) {
        try {
            LnkParser::Parser parser(n);
            parser.parse();
            LnkOutput::StreamPtr o = parser.output();
            if (command_line.yaml) {
                CodecPtr c = codecs.get(command_line.codepage);
                dump_yaml(std::cout, o, c, n, command_line.default_info_level);
            }
            if (command_line.gui && state) {
                state->open_file(std::move(o), n);
            }
        }
        catch (LnkParser::Error &e) {
            // if we're doing console output, then put the error on console
            if (command_line.yaml) {
                std::cerr << n << ": " << e.what() << std::endl;
            }
            // at the same time, if we're showing the GUI, log the message
            // and keep opening files
            if (command_line.gui) {
                error_names.emplace_back(std::move(n));
                error_msgs.emplace_back(std::string(e.what()));
                continue;
            } else {
                // if we're not showing the GUI then bail
                return ERROR_PARSE;
            }
        }
    }
    // show error summary on gui
    if (command_line.gui && (error_names.size() > 0 || error_msgs.size() > 0)) {
        std::stringstream s;
        if (error_names.size() > 1) {
            s << error_names.size() << " files failed to parse:" << std::endl;
        }
        auto e = error_names.begin();
        auto m = error_msgs.begin();
        size_t i = 1;
        for (; e != error_names.end() && m != error_msgs.end(); i++, e++) {
            s << *e << ": " << *m << std::endl;
            if (MAX_GUI_ERROR_MSGS > 0 && i >= MAX_GUI_ERROR_MSGS) {
                break;
            }
        }
        state->error_msg(s.str() + ((error_names.size() > i) ? "..." : ""));
        return ERROR_PARSE;
    }
    return 0;
}

int
main(int argc, char** argv)
{
    if (!cmdline(argc, argv)) {
        usage();
        return ERROR_USAGE;
    }
    if (!command_line.gui && !command_line.yaml) {
        if (isatty(0)) {
            command_line.yaml = true;
        } else {
            command_line.gui = true;
        }
    }
    state = command_line.gui ? new MainGui() : nullptr;
    int ret = 0;
    if (command_line.files.empty()) {
        if (state != nullptr) {
            state->open_blank();
        } else {
            usage();
        }
    } else {
        ret = open_files(command_line.files);
    }
    if (state != nullptr) {
        Fl::run();
        return 0;
    } else {
        return ret;
    }
}
// }}}
