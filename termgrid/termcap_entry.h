#pragma once
#include <memory>

namespace termgrid {
struct TermcapEntry
{
    struct TermcapEntryImpl *m_impl;

    // getenv("TERM")
    TermcapEntry(const char *term);
    ~TermcapEntry();
    TermcapEntry(const TermcapEntry &) = delete;
    TermcapEntry &operator=(const TermcapEntry &) = delete;

    static std::shared_ptr<TermcapEntry> create_from_env();

    void clear();
    void clear_to_eol();
    int lines() const;
    int columns() const;
    void cursor_xy(int col, int line);
    void cursor_save();
    void cursor_restore();
    void cursor_show(bool enable);
    void standout(bool enable);

    std::tuple<int, int> cursor_xy()const;
};
using TermcapEntryPtr = std::shared_ptr<termgrid::TermcapEntry>;

}
