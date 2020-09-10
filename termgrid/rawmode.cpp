#include "rawmode.h"
#include <errno.h>

namespace termgrid {

RawMode::RawMode(int tty) : m_tty(tty)
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

RawMode::~RawMode()
{
#if 1
    termios ioval;
    tcgetattr(m_tty, &ioval);
    ioval.c_lflag |= TTY_MODE;
    ioval.c_iflag |= (IXON | IXOFF);
    ioval.c_cc[VMIN] = 4;
    while (tcsetattr(m_tty, TCSANOW, &ioval) == -1)
#else
    while (tcsetattr(m_tty, TCSANOW, &m_ioval) == -1)
#endif
    {
        if (errno == EINTR || errno == EAGAIN)
            continue;
        break;
    }
}

}
