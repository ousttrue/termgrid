#pragma once

struct TermcapEntry
{
    struct TermcapEntryImpl *m_impl;

    // getenv("TERM")
    TermcapEntry(const char *term);
    ~TermcapEntry();
    TermcapEntry(const TermcapEntry &) = delete;
    TermcapEntry &operator=(const TermcapEntry &) = delete;

    void clear();
    int lines() const;
    int columns() const;
    void cursor_xy(int col, int line);
    void cursor_save();
    void cursor_restore();
};
