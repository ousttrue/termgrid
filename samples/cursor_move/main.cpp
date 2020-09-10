#include <memory>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <asio.hpp>
#include <rawmode.h>
#include <termcap_entry.h>

using DispatchFunc = std::function<bool(int c)>;

class Asio
{
    asio::io_context context;
    asio::posix::stream_descriptor tty;
    char byteArray[1];
    asio::signal_set signals;

    termgrid::RawMode rawmode;
    termgrid::TermcapEntryPtr m_entry;

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
    termgrid::TermcapEntryPtr m_entry;

    int m_col = 0;
    int m_line = 0;

public:
    CursorMove(const termgrid::TermcapEntryPtr &entry) : m_entry(entry)
    {
        // clear
        m_entry->clear();

        auto cols = m_entry->columns();
        auto lines = m_entry->lines();

        std::stringstream ss;
        ss << "lines x cols = " << lines << " x " << cols;
        auto s = ss.str();
        auto width = s.size();
        if (cols >= width)
        {
            auto mergin = (cols - width) / 2;
            m_entry->cursor_xy(mergin, lines / 2);
            std::cout << s;
        }
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

        m_col = std::clamp(m_col, 0, cols - 1);
        m_line = std::clamp(m_line, 0, lines - 2);

        m_entry->cursor_xy(0, lines - 1);
        std::cout << "key: 0x" << std::hex << c << "(" << (char)c << ")"
                  << "      ";

        std::stringstream ss;
        ss << "    " << m_line << ", " << m_col;
        auto s = ss.str();
        m_entry->cursor_xy(cols - s.size(), lines - 1);
        std::cout << s;

        m_entry->cursor_xy(m_col, m_line);
        std::cout.flush();

        return true;
    }
};

int main(int argc, char **argv)
{
    auto entry = termgrid::TermcapEntry::create_from_env();
    if(!entry)
    {
        return 1;
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
