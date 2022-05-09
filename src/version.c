#ifndef NO_POSIX_SOURCE
#undef _POSIX_SOURCE
#define _POSIX_SOURCE 1
#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 2
#endif

#include <curses.h>

#include "version.h"

void do_about(void)
{
  int offset;

  offset=(COLS-41)/2;
  (void)clear();
  (void)move(0,offset+13); (void)addstr("` ',`    '  '");
  (void)move(1,offset+14); (void)addstr("`   '  ` ' '");
  (void)move(2,offset+15); (void)addstr("`' '   '`'");
  (void)move(3,offset+15); (void)addstr("' `   ' '`");
  (void)move(4,offset+2); (void)addstr("'           '` ' ` '`` `");
  (void)move(5,offset+2); (void)addstr("`.   Table Editor And Planner, or:");
  (void)move(6,offset+4); (void)addstr(",         . ,   ,  . .");
  (void)move(7,offset+14); (void)addstr("` '   `  ' '");
  (void)move(8,offset+3); (void)addstr("`::\\    /:::::::::::::::::\\   ___");
  (void)move(9,offset+4); (void)addstr("`::\\  /:::::::::::::::::::\\,'::::\\");
  (void)move(10,offset+5); (void)addstr(":::\\/:::::::::::::::::::::\\/   \\:\\");
  (void)move(11,offset+5); (void)addstr(":::::::::::::::::::::::::::\\    :::");
  (void)move(12,offset+5); (void)addstr("::::::::::::::::::::::::::::;  /:;'");
  (void)move(13,offset+5); (void)addstr("`::::::::::::::::::::::::::::_/:;'");
  (void)move(14,offset+7); (void)addstr("`::::::::::::::::::::::::::::'");
  (void)move(15,offset+8); (void)addstr("`////////////////////////'");
  (void)move(16,offset+9); (void)addstr("`:::::::::::::::::::::'");
  (void)move(17,offset+2); (void)addstr("");
  (void)move(18,offset+17); (void)addstr("Teapot");
  (void)move(20,offset+0); (void)addstr("Version " VERSION ", Copyright by Michael Haardt");
  (void)move(22,offset+8); (void)addstr("Press any key to continue");
  (void)refresh();
  (void)getch();
}
