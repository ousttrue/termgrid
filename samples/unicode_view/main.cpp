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
#include <fmt/core.h>
#include <tcb/span.hpp>
#include <termgrid.h>

using DispatchFunc = std::function<bool(int c)>;

#include "../../_external/wcwidth-cjk/wcwidth.c"

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

static int get_cols(char32_t unicode)
{
    if (unicode < 0x20 || (unicode >= 0x7F && unicode < 0xA0))
    {
        return 0;
    }

    auto block = c8::unicode::get_block(unicode);

    switch ((c8::unicode::UnicodeBlocks)block.front)
    {
    case c8::unicode::UnicodeBlocks::COMBINING_DIACRITICAL_MARKS:
        return 0;

    case c8::unicode::UnicodeBlocks::TRANSPORT_AND_MAP_SYMBOLS:
    case c8::unicode::UnicodeBlocks::MISCELLANEOUS_SYMBOLS_AND_PICTOGRAPHS:
    case c8::unicode::UnicodeBlocks::EMOTICONS:
    case c8::unicode::UnicodeBlocks::SYMBOLS_AND_PICTOGRAPHS_EXTENDED_A:
    case c8::unicode::UnicodeBlocks::SUPPLEMENTAL_SYMBOLS_AND_PICTOGRAPHS:
        return 2;

    case c8::unicode::UnicodeBlocks::CJK_UNIFIED_IDEOGRAPHS:
    case c8::unicode::UnicodeBlocks::CJK_UNIFIED_IDEOGRAPHS_EXTENSION_A:
    case c8::unicode::UnicodeBlocks::CJK_UNIFIED_IDEOGRAPHS_EXTENSION_B:
    case c8::unicode::UnicodeBlocks::CJK_UNIFIED_IDEOGRAPHS_EXTENSION_C:
    case c8::unicode::UnicodeBlocks::CJK_UNIFIED_IDEOGRAPHS_EXTENSION_D:
    case c8::unicode::UnicodeBlocks::CJK_UNIFIED_IDEOGRAPHS_EXTENSION_E:
    case c8::unicode::UnicodeBlocks::CJK_UNIFIED_IDEOGRAPHS_EXTENSION_F:
    case c8::unicode::UnicodeBlocks::CJK_UNIFIED_IDEOGRAPHS_EXTENSION_G:
    case c8::unicode::UnicodeBlocks::HANGUL_JAMO:
    case c8::unicode::UnicodeBlocks::HANGUL_JAMO_EXTENDED_A:
    case c8::unicode::UnicodeBlocks::HANGUL_JAMO_EXTENDED_B:
    case c8::unicode::UnicodeBlocks::HANGUL_COMPATIBILITY_JAMO:
    case c8::unicode::UnicodeBlocks::HANGUL_SYLLABLES:
    case c8::unicode::UnicodeBlocks::CJK_RADICALS_SUPPLEMENT:
    case c8::unicode::UnicodeBlocks::KANGXI_RADICALS:
    case c8::unicode::UnicodeBlocks::ENCLOSED_CJK_LETTERS_AND_MONTHS:
    case c8::unicode::UnicodeBlocks::CJK_COMPATIBILITY:
    case c8::unicode::UnicodeBlocks::CJK_COMPATIBILITY_FORMS:
    case c8::unicode::UnicodeBlocks::CJK_COMPATIBILITY_IDEOGRAPHS:
    case c8::unicode::UnicodeBlocks::CJK_COMPATIBILITY_IDEOGRAPHS_SUPPLEMENT:
    case c8::unicode::UnicodeBlocks::HALFWIDTH_AND_FULLWIDTH_FORMS:
    case c8::unicode::UnicodeBlocks::YI_RADICALS:
    case c8::unicode::UnicodeBlocks::YI_SYLLABLES:
    case c8::unicode::UnicodeBlocks::CJK_SYMBOLS_AND_PUNCTUATION:
    case c8::unicode::UnicodeBlocks::HIRAGANA:
    case c8::unicode::UnicodeBlocks::KATAKANA:
    case c8::unicode::UnicodeBlocks::KATAKANA_PHONETIC_EXTENSIONS:
    case c8::unicode::UnicodeBlocks::BOPOMOFO:
    case c8::unicode::UnicodeBlocks::BOPOMOFO_EXTENDED:
    case c8::unicode::UnicodeBlocks::CJK_STROKES:
    case c8::unicode::UnicodeBlocks::IDEOGRAPHIC_DESCRIPTION_CHARACTERS:
    case c8::unicode::UnicodeBlocks::KANBUN:

        // CJK
        return wcwidth_cjk(unicode);
    }

    return 1;
}

class UnicodeGrid
{
    //      0 1 2 ... D E F
    // 0000
    //  :
    // 4095

    int m_plane = -1;
    std::array<termgrid::TermLine, 4096> m_lines;

public:
    UnicodeGrid()
    {
        SetPlane(0);
    }

    void SetPlane(int unicode_plane)
    {
        if (m_plane == unicode_plane)
        {
            return;
        }
        m_plane = unicode_plane;
        for (int j = 0; j < 4096; ++j)
        {
            auto unicode_base = (unicode_plane << 16) | (j << 4);
            auto block = c8::unicode::get_block(unicode_base);
            auto &l = m_lines[j];
            l.clear();
            l.push(fmt::format((const char *)u8"{:04X}│", unicode_base));
            for (int i = 0; i < 16; ++i)
            {
                auto unicode = unicode_base + i;
                auto cols = get_cols(unicode);
                auto cp = c8::utf8::from_unicode(unicode);
                // auto cols = get_cols(unicode);
                // padding
                {
                    switch (cols)
                    {
                    case 0:
                    {
                        auto span = l.push(u8"  │");
                        for (auto &c : span)
                            c.cols = 1;
                        break;
                    }
                    case 1:
                    {
                        l.push(cp.view())[0].cols = 1;
                        auto span = l.push(u8" │");
                        for (auto &c : span)
                            c.cols = 1;
                        break;
                    }
                    case 2:
                    {
                        l.push(cp.view())[0].cols = 2;
                        auto span = l.push(u8"│");
                        for (auto &c : span)
                            c.cols = 1;
                        break;
                    }
                    default:
                        assert(false);
                        break;
                    }
                }
            }
            l.push(block.name);
        }
    }

    tcb::span<termgrid::TermCodepoint> GetLine(const termgrid::TermPoint &p)
    {
        // TODO: p.x
        return m_lines[p.y].codes;
    }

    using GetLineFunc = std::function<tcb::span<termgrid::TermCodepoint>(
        const termgrid::TermPoint &)>;

    void RenderBlit(const termgrid::TermcapEntryPtr &entry,
                    const GetLineFunc &getLine, const termgrid::TermPoint &src,
                    const termgrid::TermSize &size,
                    const termgrid::TermPoint &dst)
    {
        for (int y = 0; y < size.height; ++y)
        {
            entry->cursor_xy(dst.x, dst.y + y);
            auto line = getLine({src.x, src.y + y});
            auto p = line.begin();
            for (int x = 0; x < size.width && p != line.end(); ++p)
            {
                if (x + p->cols > size.width)
                {
                    // over eol
                    break;
                }
                std::cout.write((const char *)p->cp.data(),
                                p->cp.codeunit_count());
                x += p->cols;
            }
            entry->clear_to_eol();
        }
    }
};
using UnicodeGridPtr = std::shared_ptr<UnicodeGrid>;

class UnicodeView
{
    termgrid::TermcapEntryPtr m_entry;
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
    UnicodeView(const termgrid::TermcapEntryPtr &entry)
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
        m_grid->SetPlane(m_plane);

        m_entry->cursor_show(false);
        m_grid->RenderBlit(m_entry,
                           [g = m_grid](const termgrid::TermPoint &p) {
                               return g->GetLine(p);
                           },
                           {0, m_topline}, {m_cols, m_lines - 2}, {0, 1});

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
        m_entry->cursor_show(true);
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
        m_topline = std::clamp(m_topline, 0, 4096 - height);
        m_plane =
            std::clamp(m_plane, 0, (int)c8::unicode::UnicodePlanes::SPUA_B);

        Draw(c);

        return true;
    }
};

int main(int argc, char **argv)
{
    auto entry = termgrid::TermcapEntry::create_from_env();
    if (!entry)
    {
        return 1;
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
