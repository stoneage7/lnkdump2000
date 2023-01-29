
/*****
 * Part of LnkDump2000
 * Licence: GPL, version 3 or later (see COPYING file or https://www.gnu.org/licenses/gpl-3.0.txt)
 *****/

#ifndef __MAIN_H__
#define __MAIN_H__

// for class MainGui
#include "output.h"
#include <string>

// to be included from *.ui files
#include <Fl/Fl_Browser.H>
#include <Fl/Fl_Double_Window.H>
#include <Fl/Fl_Output.H>
#include <Fl/Fl_Choice.H>
#include <Fl/Fl_Button.H>

class MainPriv;

class MainGui {
private:
    MainPriv*               p;

public:
    MainGui();
    ~MainGui();
    void run();
    void open_file(LnkOutput::StreamPtr o, const std::string& name);
    void open_file_cb();
    void context_menu_cb(void *v);
    void change_codepage_cb(void *v);
    void show_all_cb(void *v);
    void close_file_cb(void *v);
    void open_blank();
    void close_blank();
    void open_about();
    void close_about();
    void error_msg(const std::string& msg);
    friend int main(int, char**);
};

// global state
extern MainGui* state;

//
class DNDWindow: public Fl_Double_Window
{
protected:
    bool                    m_dnd = false;
public:
    using Fl_Double_Window::Fl_Double_Window;
    int handle(int event) override;
};

// defined in lnk.ui
class LnkWindow: public DNDWindow
{
public:
    Fl_Browser*             m_content;
    Fl_Output*              m_title;
    Fl_Choice*              m_codepages;
    Fl_Button*              m_show_all;
    LnkOutput::StreamPtr    m_output;
    LnkOutput::InfoLevel info_level() const
    {
        // for convenience
        if (m_show_all->value() <= 0) return LnkOutput::NORMAL;
        else return LnkOutput::DEBUG;
    }
    using DNDWindow::DNDWindow;
};

// defined in blank.ui
class BlankWindow: public DNDWindow
{
public:
    using DNDWindow::DNDWindow;
};

#endif // #ifndef __MAIN_H__
