#include "termcap_entry.h"
#include <fcntl.h>
#include <string>
#include <unistd.h> // isatty
#include <iostream>
#include <thread>

extern "C" int tgetent(const char *, const char *);
extern "C" char *tgetstr(const char *, const char **);
extern "C" int tgetflag(const char *);
extern "C" int tgetnum(const char *);
extern "C" int tputs(const char *, int, int (*)(int));
extern "C" char *tgoto(const char *, int, int);

static std::string_view getstr(const char *name)
{
    auto func = tgetstr(name, nullptr);
    if (!func)
    {
        return "";
    }
    return func;
}

namespace termgrid
{

struct TermcapEntryImpl
{
    std::string TERM;

    std::string ce; /* clear to the end of line */
    std::string cd; /* clear to the end of display */
    std::string cr; /* cursor right */
    std::string kr; /* cursor right */
    std::string kl; /* cursor left */
    std::string ta; /* tab */
    std::string sc; /* save cursor */
    std::string rc; /* restore cursor */
    std::string so; /* standout mode */
    std::string se; /* standout mode end */
    std::string us; /* underline mode */
    std::string ue; /* underline mode end */
    std::string md; /* bold mode */
    std::string me; /* bold mode end */
    std::string cl; /* clear screen */
    std::string cm; /* cursor move */
    std::string al; /* append line */
    std::string sr; /* scroll reverse */
    std::string ti; /* terminal init */
    std::string te; /* terminal end */
    std::string nd; /* move right one space */
    std::string eA; /* enable alternative charset */
    std::string as; /* alternative (graphic) charset start */
    std::string ae; /* alternative (graphic) charset end */
    std::string ac; /* graphics charset pairs */
    std::string op; /* set default color pair to its original value */
    std::string vi;
    std::string ve;

    TermcapEntryImpl(const char *term)
    {
        char buffer[1024];
        auto e = tgetent(buffer, term);
        if (!e)
        {
            return;
        }

        ce = getstr("ce"); /* clear to the end of line */
        cd = getstr("cd"); /* clear to the end of display */
        kr = getstr("nd"); /* cursor right */
        if (kr.empty())
        {
            kr = getstr("kr");
        }
        if (tgetflag("bs"))
        {
            kl = "\b"; /* cursor left */
        }
        else
        {
            kl = getstr("le");
            if (kl.size())
            {
                kl = getstr("kb");
            }
            if (kl.size())
            {
                kl = getstr("kl");
            }
        }
        cr = getstr("cr"); /* carriage return */
        ta = getstr("ta"); /* tab */
        sc = getstr("sc"); /* save cursor */
        rc = getstr("rc"); /* restore cursor */
        so = getstr("so"); /* standout mode */
        se = getstr("se"); /* standout mode end */
        us = getstr("us"); /* underline mode */
        ue = getstr("ue"); /* underline mode end */
        md = getstr("md"); /* bold mode */
        me = getstr("me"); /* bold mode end */
        cl = getstr("cl"); /* clear screen */
        cm = getstr("cm"); /* cursor move */
        al = getstr("al"); /* append line */
        sr = getstr("sr"); /* scroll reverse */
        ti = getstr("ti"); /* terminal init */
        te = getstr("te"); /* terminal end */
        nd = getstr("nd"); /* move right one space */
        eA = getstr("eA"); /* enable alternative charset */
        as = getstr("as"); /* alternative (graphic) charset start */
        ae = getstr("ae"); /* alternative (graphic) charset end */
        ac = getstr("ac"); /* graphics charset pairs */
        op = getstr("op"); /* set default color pair to its original value */
        vi = getstr("vi");
        ve = getstr("ve");
    }
};

TermcapEntry::TermcapEntry(const char *term)
    : m_impl(new TermcapEntryImpl(term))
{
}

TermcapEntry::~TermcapEntry()
{
    delete m_impl;
}

TermcapEntryPtr TermcapEntry::create_from_env()
{
    if (!isatty(0))
    {
        return nullptr;
    }

    auto term = getenv("TERM");
    if (!term)
    {
        return nullptr;
    }

    auto entry = std::make_shared<termgrid::TermcapEntry>(term);
    if (!entry)
    {
        return nullptr;
    }

    return entry;
}

void TermcapEntry::clear()
{
    tputs(m_impl->cl.c_str(), 1, putchar);
}

void TermcapEntry::clear_to_eol()
{
    tputs(m_impl->ce.c_str(), 1, putchar);
}

int TermcapEntry::lines() const
{
    return tgetnum("li");
}

int TermcapEntry::columns() const
{
    return tgetnum("co");
}

void TermcapEntry::cursor_xy(int col, int line)
{
    auto s = tgoto(m_impl->cm.c_str(), col, line);
    tputs(s, 1, putchar);
}

void TermcapEntry::cursor_save()
{
    tputs(m_impl->sc.c_str(), 1, putchar);
}

void TermcapEntry::cursor_restore()
{
    tputs(m_impl->rc.c_str(), 1, putchar);
}

void TermcapEntry::cursor_show(bool enable)
{
    if (enable)
    {
        tputs(m_impl->ve.c_str(), 1, putchar);
    }
    else
    {
        tputs(m_impl->vi.c_str(), 1, putchar);
    }
}

void TermcapEntry::standout(bool enable)
{
    if (enable)
    {
        tputs(m_impl->so.c_str(), 1, putchar);
    }
    else
    {
        tputs(m_impl->se.c_str(), 1, putchar);
    }
}

std::tuple<int, int> TermcapEntry::cursor_xy() const
{
    char buf[32] = {0};
    std::thread t([&buf]() {
        int i = 0;
        for (; i < sizeof(buf); ++i)
        {
            char c;
            if (!std::cin.get(c))
            {
                break;
            }
            buf[i] = c;
            if (c == 'R')
            {
                ++i;
                break;
            }
        }
    });

    std::string_view cmd = "\033[6n";
    std::cout.write(cmd.data(), cmd.size());
    std::cout.flush();

    t.join();

    auto p = buf;
    if (*(p++) != '\033')
        return {-1, -1};
    if (*(p++) != '[')
        return {-1, -1};

    int line = 0;
    for (; *p; ++p)
    {
        if (!isdigit(*p))
        {
            break;
        }
        line = line * 10 + *p - '0';
    }

    if (*(p++) != ';')
    {
        return {-1, -1};
    }

    int col = 0;
    for (; *p; ++p)
    {
        if (!isdigit(*p))
        {
            break;
        }
        col = col * 10 + *p - '0';
    }

    if (*p != 'R')
    {
        return {-1, -1};
    }

    return {col - 1, line - 1};
}

} // namespace termgrid
