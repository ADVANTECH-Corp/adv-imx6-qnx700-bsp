/*
 * $QNXLicenseC:
 * Copyright 2016 QNX Software Systems.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You
 * may not reproduce, modify or distribute this software except in
 * compliance with the License. You may obtain a copy of the License
 * at: http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTIES OF ANY KIND, either express or implied.
 *
 * This file may contain contributions from others, either as
 * contributors under the License or as licensors under other terms.
 * Please review this entire file for other proprietary rights or license
 * notices, as well as the QNX Development Suite License Guide at
 * http://licensing.qnx.com/license-guide/ for other information.
 * $
 */

#include <inttypes.h>
#include <sys/types.h>
#include <string.h>
#include "ipl.h"

/* utility string func */
size_t strlen(const char *s)
{	/* find length of s[] */
	const char *sc;

	for (sc = s; *sc != '\0'; ++sc)
		;
	return (sc - s);
}

char *strsep(char ** stringp, const char *delim)
{
	char *s;
	const char *spanp;
	int c, sc;
	char *tok;

	if ((s = *stringp) == NULL)
		return (NULL);
	for (tok = s;;) {
		c = *s++;
		spanp = delim;
		do {
			if ((sc = *spanp++) == c) {
				if (c == 0)
					s = NULL;
				else
					s[-1] = 0;
				*stringp = s;
				return (tok);
			}
		} while (sc != 0);
	}
	/* NOTREACHED */
}

int strncmp(const char *s1, const char *s2, size_t n)
{	/* compare unsigned char s1[max n], s2[max n] */
	for (; 0 < n; ++s1, ++s2, --n)
		if (*s1 != *s2)
			return (*(unsigned char *)s1
				< *(unsigned char *)s2 ? -1 : +1);
		else if (*s1 == '\0')
			return (0);
	return (0);
}

char *strcpy(char *s1, const char *s2)
{	/* copy char s2[] to s1[] */
	char *s;

	for (s = s1; (*s++ = *s2++) != '\0'; )
		;
	return (s1);
}

char *strncat(char *s1, const char *s2, size_t n)
{	/* copy char s2[max n] to end of s1[] */
	char *s;

	for (s = s1; *s != '\0'; ++s)
		;	/* find end of s1[] */
	for (; 0 < n && *s2 != '\0'; --n)
		*s++ = *s2++;	/* copy at most n chars from s2[] */
	*s = '\0';
	return (s1);
}

void *memcpy(void *dst, const void *src, size_t nbytes) {
	{
		char			*d = dst;
		const char		*s = src;

		while(nbytes--) {
			*d++ = *s++;
		}
	}
	return dst;
}

//simple hex string conversion, no error and overflow checking
unsigned long strhextoul(const char *cp)
{
	unsigned long result = 0;
	int value;

	//skip 0 and x
	while(*cp == '0' || *cp == 'x') {
		cp++;
	}

	while (*cp != '\0'){
		value = *cp;
		/* Adjust character to from a char to an integer */
		if(value >= '0' && value <= '9') {
			value -= '0';
		} else if(value >= 'a' && value <= 'f') {
			value -= 'a' - 10;
		} else if(value >= 'A' && value <= 'F') {
			value -= 'A' - 10;
		} else {
			/* Unknown, done */
			ser_putstr("error hex convert!\n");
			break;
		}
		result = result*16 + value;
		cp++;
	}

	return result;
}

#if 0
void *memset(void *p, int c, size_t size)
{
	uint8_t *pp = (uint8_t *)p;

	while (size-- > 0)
		*pp++ = (uint8_t)c;

	return p;
}
#endif


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/ipl/lib/string.c $ $Rev: 808052 $")
#endif
