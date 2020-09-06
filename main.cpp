#include <fcntl.h>
#include <memory>
#include <string_view>
#include <functional>
#include <iostream>
#include <asio.hpp>

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

struct TermcapEntry
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

    TermcapEntry(const char *term)
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
    }

    int lines() const
    {
        return tgetnum("li");
    }

    int columns() const
    {
        return tgetnum("co");
    }

    void cursor_xy(int col, int line)
    {
        auto s = tgoto(cm.c_str(), col, line);
        tputs(s, 1, putchar);
    }

    void cursor_save()
    {
        tputs(sc.c_str(), 1, putchar);
    }

    void cursor_restore()
    {
        tputs(rc.c_str(), 1, putchar);
    }
};
using TermcapEntryPtr = std::shared_ptr<TermcapEntry>;

struct RawMode
{
    const int TTY_MODE = (ISIG | ICANON | ECHO | IEXTEN);
    termios m_ioval;
    int m_tty;

    RawMode(int tty) : m_tty(tty)
    {
        termios ioval;
        tcgetattr(m_tty, &ioval);
        m_ioval = ioval;
        ioval.c_lflag &= ~TTY_MODE;
        ioval.c_iflag &= ~(IXON | IXOFF);
        ioval.c_cc[VMIN] = 1;
        while (tcsetattr(m_tty, TCSANOW, &ioval) == -1)
        {
            if (errno == EINTR || errno == EAGAIN)
                continue;
            break;
        }
    }
    ~RawMode()
    {
        std::cout << "restore mode" << std::endl;
        while (tcsetattr(m_tty, TCSANOW, &m_ioval) == -1)
        {
            if (errno == EINTR || errno == EAGAIN)
                continue;
            break;
        }
    }
};

using DispatchFunc = std::function<bool(int c)>;

class Asio
{
    asio::io_context context;
    asio::posix::stream_descriptor tty;
    char byteArray[1];
    asio::signal_set signals;

    RawMode rawmode;
    TermcapEntryPtr m_entry;

public:
    Asio(int tty)
        : rawmode(tty), tty(context, tty),
          signals(context, SIGINT, SIGTERM, SIGWINCH)
    {
    }

    ~Asio()
    {
    }

    void Quit()
    {
        signals.cancel();
    }

    void ReadTty(const DispatchFunc &dispatcher)
    {
        // Terminal::term_raw();
        auto buffer = asio::buffer(byteArray);
        auto callback = std::bind(&Asio::OnReadTty, this, dispatcher,
                                  std::placeholders::_1, std::placeholders::_2);
        tty.async_read_some(buffer, callback);
    }

    void OnReadTty(const DispatchFunc &dispatcher, asio::error_code ec,
                   std::size_t n)
    {
        if (n == 0)
        {
            // LOGE << "error. read zero";
            return;
        }

        // getch
        auto c = byteArray[0];
        if (!dispatcher(c))
        {
            Quit();
            return;
        }

        // next read
        ReadTty(dispatcher);
    }

    void OnSignal(const asio::error_code &error, int signal)
    {
        // LOGI << "signal: " << signal;
        std::cout << "signal: " << signal << std::endl;
    }

    void Signal()
    {
        // Construct a signal set registered for process termination.
        auto callback = std::bind(&Asio::OnSignal, this, std::placeholders::_1,
                                  std::placeholders::_2);
        signals.async_wait(callback);
    }

    void Run()
    {
        context.run();
    }
};

class CursorMove
{
    TermcapEntryPtr m_entry;

    int m_col = 0;
    int m_line = 0;

public:
    CursorMove(const TermcapEntryPtr &entry) : m_entry(entry)
    {
        // clear
        tputs(m_entry->cl.c_str(), 1, putchar);
        m_entry->cursor_xy(m_col, m_line);
        std::cout.flush();
    }

    ~CursorMove()
    {
        // move
        auto lines = m_entry->lines();
        m_entry->cursor_xy(0, lines - 1);
        std::cout.flush();
    }

    bool Dispatch(int c)
    {
        if (c == 'q')
        {
            return false;
        }

        auto cols = m_entry->columns();
        auto lines = m_entry->lines();

        switch (c)
        {
        case 'h':
            --m_col;
            break;
        case 'j':
            ++m_line;
            break;
        case 'k':
            --m_line;
            break;
        case 'l':
            ++m_col;
            break;
        case '0':
            m_col = 0;
            break;
        case '$':
            m_col = cols - 1;
            break;
        }

        if (m_col < 0)
        {
            m_col = 0;
        }
        else if (m_col >= cols)
        {
            m_col = cols - 1;
        }
        if (m_line < 0)
        {
            m_line = 0;
        }
        else if (m_line >= lines - 1)
        {
            m_line = lines - 2;
        }

        // m_entry->cursor_save();
        m_entry->cursor_xy(0, lines - 1);
        std::cout << "key: " << (char)c;
        m_entry->cursor_xy(m_col, m_line);
        std::cout.flush();

        return true;
    }
};

int main(int argc, char **argv)
{
    // open tty
    if (!isatty(0))
    {
        return 1;
    }

    auto term = getenv("TERM");
    if (!term)
    {
        return 2;
    }

    auto entry = std::make_shared<TermcapEntry>(term);
    if (!entry)
    {
        return 3;
    }

    // main loop
    Asio asio(0);
    {
        CursorMove d(entry);
        asio.ReadTty([&d](int c) { return d.Dispatch(c); });
        asio.Signal();
        asio.Run();
    }

    return 0;
}
