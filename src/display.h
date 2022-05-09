#ifndef DISPLAY_H
#define DISPLAY_H

#include <sys/types.h>
#include <curses.h>

#include "sheet.h"

typedef struct
{
  chtype c;
  char *str;
} MenuChoice;

void redraw(void);
void redraw_sheet(Sheet *sheet);
int line_edit(Sheet *sheet, char *buf, size_t size, const char *prompt, size_t *x, size_t *offx);
int line_menu(const char *prompt, const MenuChoice *choice, int curx);
void line_msg(const char *prompt, const char *msg);
int keypressed(void);

#endif
