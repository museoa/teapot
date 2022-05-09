#ifndef DEFAULT_H
#define DEFAULT_H

#include <stdlib.h>

/* default width of a column */
#define DEF_COLUMNWIDTH       12

/* default precision of a printed value */
#define DEF_PRECISION          2

/* default is no scientific notation for numbers */
#define DEF_SCIENTIFIC         0

/* character attribute for cell and row numbers */
#define DEF_NUMBER        A_BOLD

/* character attribute for cell cursor */
#define DEF_CELLCURSOR A_REVERSE

/* character attribute for selected menu choice */
#define DEF_MENU       A_REVERSE

/* maximum number of sort keys */
#define MAX_SORTKEYS           8

/* maximum number of eval() nesting */
#define MAX_EVALNEST          32

/* define if testing with EletricFence */
#ifdef THE_ELECTRIC_FENCE
#undef malloc
#undef free
#undef realloc
#undef calloc
#endif

#endif
