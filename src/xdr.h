#ifndef XDR_H
#define XDR_H

/* Thanks to curses. */
#if 0
#undef TRUE
#undef FALSE
#endif

#include <sys/types.h>
#include <rpc/types.h>
#include <rpc/xdr.h>

#include "sheet.h"

bool_t xdr_cell(XDR *xdrs, Cell *cell);
bool_t xdr_column(XDR *xdrs, int *x, int *z, int *width);
bool_t xdr_magic(XDR *xdrs);

#endif
