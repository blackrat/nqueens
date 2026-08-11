/* Copyright (c) 1992-2026 Paul McKibbin */

#if defined(__unix__) || defined(__APPLE__)
#define _POSIX_C_SOURCE 200809L
#endif

#include "keys.h"

#include <stdlib.h>

bool key_is_quit(int key)
{
    return key == 0x1b || key == 'q' || key == 'Q';
}

#if defined(__unix__) || defined(__APPLE__)

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

static struct termios saved_termios;
static int saved_flags = -1;
static bool raw_mode = false;

void keys_end(void)
{
    if (!raw_mode) {
        return;
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &saved_termios);
    if (saved_flags != -1) {
        fcntl(STDIN_FILENO, F_SETFL, saved_flags);
    }
    raw_mode = false;
}

void keys_begin(void)
{
    if (raw_mode || isatty(STDIN_FILENO) != 1) {
        return;
    }
    if (tcgetattr(STDIN_FILENO, &saved_termios) != 0) {
        return;
    }

    struct termios raw = saved_termios;
    raw.c_lflag &= (tcflag_t) ~(ICANON | ECHO); /* unbuffered, and do not echo */
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) != 0) {
        return;
    }

    saved_flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (saved_flags != -1) {
        fcntl(STDIN_FILENO, F_SETFL, saved_flags | O_NONBLOCK);
    }

    raw_mode = true;
    atexit(keys_end); /* never leave the user's terminal in raw mode */
}

int keys_poll(void)
{
    if (!raw_mode) {
        return KEY_NONE;
    }
    unsigned char key = 0;
    return read(STDIN_FILENO, &key, 1) == 1 ? (int)key : KEY_NONE;
}

#elif defined(_WIN32)

#include <conio.h>

void keys_begin(void)
{
}

void keys_end(void)
{
}

int keys_poll(void)
{
    return _kbhit() ? _getch() : KEY_NONE;
}

#else

void keys_begin(void)
{
}

void keys_end(void)
{
}

int keys_poll(void)
{
    return KEY_NONE;
}

#endif
