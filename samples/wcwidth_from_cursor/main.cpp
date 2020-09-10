#include <termcap_entry.h>
#include <rawmode.h>
#include <iostream>
#include <char8/char8.hpp>

static int get_cols(const termgrid::TermcapEntryPtr &e, char32_t unicode)
{
    e->cursor_xy(0, 0);
    auto utf8 = c8::utf8::from_unicode(unicode);
    std::cout.write((const char *)utf8.data(), utf8.codeunit_count());
    auto [x, y] = e->cursor_xy();
    // std::cout << " => " << x << std::endl;
    return x;
}

struct UnicodeColumns
{
    uint8_t plane = 0; // BMP
    uint8_t columns[65536];

    UnicodeColumns(const termgrid::TermcapEntryPtr &e, int plane = 0)
    {
        this->plane = plane;
        auto unicode = plane << 16;
        for (int i = 0; i < 65536; ++i, ++unicode)
        {
            this->columns[i] = get_cols(e, unicode);
        }
    }
};

int main(int argc, char **argv)
{
    auto entry = termgrid::TermcapEntry::create_from_env();
    if (!entry)
    {
        return 1;
    }

    termgrid::RawMode raw;

    UnicodeColumns cols(entry);

    return 0;
}
