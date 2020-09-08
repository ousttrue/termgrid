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

static int get_cols(c8::unicode::UnicodeBlock &block, char32_t unicode)
{
    switch ((c8::unicode::UnicodeBlocks)block.front)
    {
    case c8::unicode::UnicodeBlocks::BASIC_LATIN:
    case c8::unicode::UnicodeBlocks::LATIN_1_SUPPLEMENT:
    case c8::unicode::UnicodeBlocks::LATIN_EXTENDED_A:
    case c8::unicode::UnicodeBlocks::LATIN_EXTENDED_B:
    case c8::unicode::UnicodeBlocks::IPA_EXTENSIONS:
    case c8::unicode::UnicodeBlocks::SPACING_MODIFIER_LETTERS:
    case c8::unicode::UnicodeBlocks::GREEK_AND_COPTIC:
    case c8::unicode::UnicodeBlocks::CYRILLIC:
    case c8::unicode::UnicodeBlocks::CYRILLIC_SUPPLEMENT:
    case c8::unicode::UnicodeBlocks::ENCLOSED_ALPHANUMERICS:
    case c8::unicode::UnicodeBlocks::BOX_DRAWING:
    case c8::unicode::UnicodeBlocks::GEOMETRIC_SHAPES:
    case c8::unicode::UnicodeBlocks::MISCELLANEOUS_SYMBOLS:
    case c8::unicode::UnicodeBlocks::DINGBATS:
    case c8::unicode::UnicodeBlocks::COPTIC:
    case c8::unicode::UnicodeBlocks::PRIVATE_USE_AREA:
        return 1;

    case c8::unicode::UnicodeBlocks::TRANSPORT_AND_MAP_SYMBOLS:
    case c8::unicode::UnicodeBlocks::MISCELLANEOUS_SYMBOLS_AND_PICTOGRAPHS:
    case c8::unicode::UnicodeBlocks::EMOTICONS:
    case c8::unicode::UnicodeBlocks::SYMBOLS_AND_PICTOGRAPHS_EXTENDED_A:
    case c8::unicode::UnicodeBlocks::SUPPLEMENTAL_SYMBOLS_AND_PICTOGRAPHS:
        return 2;
    }

    auto w = wcwidth_cjk(unicode);
    return w;
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

    void RenderBlit(const TermcapEntryPtr &entry, char32_t plane,
                    const TermCell &from, const TermRect &dst)
    {
        plane = plane << 16;
        char32_t l = from.y << 4;
        for (int y = 0; y < dst.height; ++y, l += 16)
        {
            if (l > 0xFFFF)
            {
                break;
            }
            auto line_unicode = plane | l;
            entry->cursor_xy(dst.left, dst.top + y);
            std::cout
                /*<< "U+"*/
                << std::hex << std::setw(4) << std::setfill('0') << l << "│";

            auto block = c8::unicode::get_block(line_unicode);

            auto x = 7;
            for (int i = 0; i < 16; ++i)
            {
                char32_t unicode = line_unicode + i;
                if (replace_space(unicode))
                {
                    unicode = 0x20;
                }

                auto w = get_cols(block, unicode);
                if ((x + w) >= dst.width)
                {
                    // eol
                    break;
                }

                auto cp = c8::utf8::from_unicode(unicode);
                std::cout.write((const char *)cp.data(), cp.codeunit_count());
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
                std::cout << "│";
                x += 3;
            }

            auto it = block.name.begin();
            for (; x < dst.width && it != block.name.end(); ++x, ++it)
            {
                std::cout << *it;
            }

            entry->clear_to_eol();
        }
    }
};
using UnicodeGridPtr = std::shared_ptr<UnicodeGrid>;

class UnicodeView
{
    TermcapEntryPtr m_entry;
    UnicodeGridPtr m_grid;

    // unicode plane: 0..0x10
    int m_plane = 0;

    // term size
    int m_cols = 0;
    int m_lines = 0;

    // 0..(4095 - lines)
    int m_topline = 0;

    // cursor x: 0..16
    int m_col = 0;
    // cursor y: 0..(lines-2)
    int m_line = 0;

public:
    UnicodeView(const TermcapEntryPtr &entry)
        : m_entry(entry), m_grid(new UnicodeGrid)
    {
        m_cols = m_entry->columns();
        m_lines = m_entry->lines();
        Draw();
    }

    ~UnicodeView()
    {
        // move
        m_entry->cursor_xy(0, m_lines - 1);
        std::cout.flush();
    }

    void Draw(int c = 0)
    {
        m_grid->RenderBlit(m_entry, m_plane, {0, m_topline},
                           {0, 1, m_cols, m_lines - 2});

        {
            m_entry->cursor_xy(0, 0);
            m_entry->standout(true);
            std::cout << "    │00│01│02│03│04│05│06│07│08│09│0a│0b│0c│0d│0e│0f│"
                      << "Unicode PLANE: " << m_plane;
            m_entry->clear_to_eol();
            m_entry->standout(false);
        }

        if (c)
        {
            m_entry->standout(true);
            m_entry->cursor_xy(0, m_lines - 1);
            std::cout << "key: 0x" << std::hex << c << "(" << (char)c << ")";
            m_entry->clear_to_eol();
            m_entry->standout(false);
        }

        m_entry->cursor_xy(5 + m_col * 3, m_line + 1);
        std::cout.flush();
    }

    bool Dispatch(int c)
    {
        if (c == 'q' || c == 0x1b)
        {
            return false;
        }

        m_cols = m_entry->columns();
        m_lines = m_entry->lines();
        auto height = m_lines - 2;
        switch (c)
        {
        case 'h':
            --m_col;
            break;

        case 'l':
            ++m_col;
            break;

        case 'j':
            ++m_line;
            break;

        case 'k':
            --m_line;
            break;

        case 'J':
            ++m_topline;
            break;

        case 'K':
            --m_topline;
            break;

        case ' ':
            m_topline += height;
            break;

        case 'b':
            m_topline -= height;
            break;

        case 'g':
            m_topline = 0;
            break;

        case 'G':
            m_topline = 0xFFFF;
            break;

        case ',':
            --m_plane;
            break;

        case '.':
            ++m_plane;
            break;
        }

        if (m_line < 0)
        {
            m_topline += m_line;
            m_line = 0;
        }
        else if (m_line >= height)
        {
            m_topline += (height + 1 - m_line);
            m_line = height - 1;
        }
        m_col = std::clamp(m_col, 0, 16 - 1);
        m_topline = std::clamp(m_topline, 0, 4096 - height - 1);
        m_plane =
            std::clamp(m_plane, 0, (int)c8::unicode::UnicodePlanes::SPUA_B);

        Draw(c);

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
