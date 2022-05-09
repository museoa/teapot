/*
    UTF-8 wrapper for teapot
    Copyright (C) 2010 Joerg Walter

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef UTF8_H
#define UTF8_H

#include <sys/types.h>
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

size_t mbslen(const char *str);
char *mbspos(const char *str, int ch);

#ifdef ENABLE_UTF8

#define UTF8SZ 4
static inline int is_mbcont(unsigned char c)
{
	return (c >= 0x80 && c < 0xc0);
}

static inline int is_mbchar(unsigned char c)
{
	return (c >= 0x80);
}

#else

#define UTF8SZ 1
static inline int is_mbcont(unsigned char c)
{
	return 0;
}

static inline int is_mbchar(unsigned char c)
{
	return 0;
}

#endif

#ifdef __cplusplus
}
#endif

#endif
