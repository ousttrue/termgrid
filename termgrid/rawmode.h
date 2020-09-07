#pragma once
#include <termios.h>

struct RawMode
{
    const int TTY_MODE = (ISIG | ICANON | ECHO | IEXTEN);
    termios m_ioval;
    int m_tty;

    RawMode(int tty);
    ~RawMode();
};
