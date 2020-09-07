#include <memory>
#include <char8/char8.hpp>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <asio.hpp>
#include <assert.h>
#include <rawmode.h>
#include <termcap_entry.h>

using DispatchFunc = std::function<bool(int c)>;
using TermcapEntryPtr = std::shared_ptr<TermcapEntry>;

#include "../../_external/wcwidth-cjk/wcwidth.c"

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

struct TermCell
{
    int x;
    int y;
};

struct TermRect
{
    int left;
    int top;
    int width;
    int height;
};

static bool replace_space(char32_t unicode)
{
    if (unicode <= 0x7F)
    {
        if (unicode == 0)
        {
            return true;
        }
        if (isspace(unicode))
        {
            return true;
        }
        if (iscntrl(unicode))
        {
            return true;
        }
    }
    else if (unicode <= 0x9F)
    {
        return true;
    }
    return false;
}

class UnicodeGrid
{
    //      0 1 2 ... D E F
    // 0000
    //  :
    // 4095
public:
    UnicodeGrid()
    {
    }

    void RenderBlit(const TermcapEntryPtr &entry, const TermCell &from,
                    const TermRect &dst)
    {
        auto l = from.y << 4;
        for (int y = 0; y < dst.height; ++y, l += 16)
        {
            if (l > 0xFFFF)
            {
                break;
            }
            entry->cursor_xy(dst.left, dst.top + y);
            std::cout << "U+" << std::hex << std::setw(4) << std::setfill('0')
                      << l << "│";

            auto x = 7;
            for (int i = 0; i < 16; ++i)
            {
                char32_t unicode = l + i;
                if (replace_space(unicode))
                {
                    unicode = 0x20;
                }

                auto w = wcwidth_cjk(unicode);
                if ((x + w) >= dst.width)
                {
                    // eol
                    break;
                }

                auto cp = c8::utf8::from_unicode(unicode);
                switch (w)
                {
                case -1:
                // {
                //     std::cout << ' ';
                //     break;
                // }
                case 0:
                {
                    std::cout << "  ";
                    break;
                }
                case 1:
                {
                    std::cout << ' ';
                    break;
                }
                case 2:
                {
                    break;
                }
                default:
                    assert(false);
                    break;
                }
                std::cout.write((const char *)cp.data(), cp.codeunit_count());
                std::cout << "│";
                x += 3;
            }
            entry->clear_to_eol();
        }
    }

    void Render(const TermcapEntryPtr &entry, const TermCell &from)
    {
        auto cols = entry->columns();
        auto lines = entry->lines();
        RenderBlit(entry, from, {0, 0, cols, lines});
    }
};
using UnicodeGridPtr = std::shared_ptr<UnicodeGrid>;

class UnicodeView
{
    TermcapEntryPtr m_entry;
    UnicodeGridPtr m_grid;

    int m_line = 0;

public:
    UnicodeView(const TermcapEntryPtr &entry)
        : m_entry(entry), m_grid(new UnicodeGrid)
    {
        // clear
        m_entry->clear();
        m_grid->Render(m_entry, {0, m_line});
        m_entry->cursor_xy(0, 0);
        std::cout.flush();
    }

    ~UnicodeView()
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

        auto lines = m_entry->lines();
        switch (c)
        {
        case 'j':
            ++m_line;
            break;

        case 'k':
            --m_line;
            break;

        case ' ':
            m_line += lines;
            break;

        case 'b':
            m_line -= lines;
            break;

        case 'g':
            m_line = 0;
            break;

        case 'G':
            m_line = 0xFFFF;
            break;
        }
        m_line = std::clamp(m_line, 0, 4096 - lines);

        m_grid->Render(m_entry, {0, m_line});

        // m_entry->cursor_xy(0, lines - 1);
        // std::cout << "key: 0x" << std::hex << c << "(" << (char)c << ")"
        //           << "      ";

        // std::stringstream ss;
        // ss << "    " << m_line << ", " << m_col;
        // auto s = ss.str();
        // m_entry->cursor_xy(cols - s.size(), lines - 1);
        // std::cout << s;

        m_entry->cursor_xy(0, 0);
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
        UnicodeView d(entry);
        asio.ReadTty([&d](int c) { return d.Dispatch(c); });
        asio.Signal();
        asio.Run();
    }

    return 0;
}
