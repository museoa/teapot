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

#include "utf8.h"

#include <string.h>

#ifdef ENABLE_UTF8

size_t mbslen(const char *s)
{
	const unsigned char *str = (unsigned char *)s;
	size_t chars = 0;

	while (*str) {
		chars++;
		if (is_mbchar(*str++)) while (is_mbcont(*str)) str++;
	}

	return chars;
}

char *mbspos(const char *s, int ch)
{
	const unsigned char *str = (unsigned char *)s;
	int chars = 0;

	if (ch < 0) {
		while (chars > ch) {
			chars--;
			while (is_mbcont(*--str));
		}
	} else {
		while (chars < ch && *str) {
			chars++;
			if (is_mbchar(*str++)) while (is_mbcont(*str)) str++;
		}
	}

	return (char *)(void *)str;
}

#else

size_t mbslen(const char *str)
{
	return strlen(str);
}

char *mbspos(const char *str, int ch)
{
	return (char *)(void *)(str+ch);
}

#endif
