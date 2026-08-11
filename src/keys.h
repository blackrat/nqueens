/* Copyright (c) 1992-2026 Paul McKibbin */

/* Non-blocking keyboard polling, so a running search can be asked "where are
   you?" without stopping it.

   This is what the original used <conio.h> kbhit()/getch() for. There is no
   standard C way to do it, so this is termios on POSIX and _kbhit() on Windows,
   and a no-op everywhere else. When stdin is not a terminal — a pipe, a test
   harness, CI — polling always reports "no key", so nothing has to care. */
#pragma once

#include <stdbool.h>

#define KEY_NONE (-1)

/* Put the terminal in raw non-blocking mode. Safe to call when stdin is not a
   terminal; keys_end() is registered to run at exit either way. */
void keys_begin(void);
void keys_end(void);

/* The next key pressed, or KEY_NONE. */
int keys_poll(void);

/* True for the keys that mean "stop": ESC, q, Q. */
bool key_is_quit(int key);
