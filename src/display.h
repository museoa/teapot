#ifndef DISPLAY_H
#define DISPLAY_H

#include "main.h"
#include "sheet.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
  Key c;
  char *str;
} MenuChoice;

void display_main(Sheet *cursheet);
void display_init(Sheet *cursheet, int always_redraw);
void display_end(void);
void redraw_cell(Sheet *sheet, int x, int y, int z);
void redraw_sheet(Sheet *sheet);
const char *line_file(const char *file, const char *pattern, const char *title, int create);
int line_edit(Sheet *sheet, char *buf, size_t size, const char *prompt, size_t *x, size_t *offx);
int line_ok(const char *prompt, int curx);
void line_msg(const char *prompt, const char *msg);
int keypressed(void);
void show_text(const char *text);

Key show_menu(Sheet *cursheet);
int line_menu(const char *prompt, const MenuChoice *choice, int curx);
void find_helpfile(char *buf, int size, const char *argv0);

#ifdef __cplusplus
}
#endif


#endif
