#ifndef WGETC_H
#define WGETC_H

#include <curses.h>

#define KEY_FPAGE    (KEY_MAX+1)
#define KEY_BPAGE    (KEY_MAX+2)
#define KEY_QUIT     (KEY_MAX+3)
#define KEY_MENTER   (KEY_MAX+4)
#define KEY_LOAD     (KEY_MAX+5)
#define KEY_SAVEQUIT (KEY_MAX+6)

chtype wgetc(void);

#endif
