#ifndef MAIN_H
#define MAIN_H

#include <curses.h>
#include <nl_types.h>

#include "sheet.h"

extern nl_catd catd;
extern int batch;
extern int def_precision;
extern int quote;
extern unsigned int batchln;

int do_sheetcmd(Sheet *cursheet, chtype c, int moveonly);

#endif
