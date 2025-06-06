/*	$OpenBSD: vfwprintf.c,v 1.24 2025/05/17 07:46:49 tobias Exp $ */
/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Actual wprintf innards.
 *
 * This code is large and complicated...
 */

#include <sys/types.h>
#include <sys/mman.h>

#include <errno.h>
#include <langinfo.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "local.h"
#include "fvwrite.h"

union arg {
	int			intarg;
	unsigned int		uintarg;
	long			longarg;
	unsigned long		ulongarg;
	long long		longlongarg;
	unsigned long long	ulonglongarg;
	ptrdiff_t		ptrdiffarg;
	size_t			sizearg;
	ssize_t			ssizearg;
	intmax_t		intmaxarg;
	uintmax_t		uintmaxarg;
	void			*pvoidarg;
	char			*pchararg;
	signed char		*pschararg;
	short			*pshortarg;
	int			*pintarg;
	long			*plongarg;
	long long		*plonglongarg;
	ptrdiff_t		*pptrdiffarg;
	ssize_t			*pssizearg;
	intmax_t		*pintmaxarg;
#ifdef FLOATING_POINT
	double			doublearg;
	long double		longdoublearg;
#endif
	wint_t			wintarg;
	wchar_t			*pwchararg;
};

static int __find_arguments(const wchar_t *fmt0, va_list ap, union arg **argtable,
    size_t *argtablesiz);
static int __grow_type_table(unsigned char **typetable, int *tablesize);

/*
 * Helper function for `fprintf to unbuffered unix file': creates a
 * temporary buffer.  We only work on write-only files; this avoids
 * worries about ungetc buffers and so forth.
 */
static int
__sbprintf(FILE *fp, const wchar_t *fmt, va_list ap)
{
	int ret;
	FILE fake;
	struct __sfileext fakeext;
	unsigned char buf[BUFSIZ];

	_FILEEXT_SETUP(&fake, &fakeext);
	/* copy the important variables */
	fake._flags = fp->_flags & ~__SNBF;
	fake._file = fp->_file;
	fake._cookie = fp->_cookie;
	fake._write = fp->_write;

	/* set up the buffer */
	fake._bf._base = fake._p = buf;
	fake._bf._size = fake._w = sizeof(buf);
	fake._lbfsize = 0;	/* not actually used, but Just In Case */

	/* do the work, then copy any error status */
	ret = __vfwprintf(&fake, fmt, ap);
	if (ret >= 0 && __sflush(&fake))
		ret = EOF;
	if (fake._flags & __SERR)
		fp->_flags |= __SERR;
	return (ret);
}

/*
 * Like __fputwc_unlock, but handles fake string (__SSTR) files properly.
 * File must already be locked.
 */
static wint_t
__xfputwc(wchar_t wc, FILE *fp)
{
	mbstate_t mbs;
	char buf[MB_LEN_MAX];
	struct __suio uio;
	struct __siov iov;
	size_t len;

	if ((fp->_flags & __SSTR) == 0)
		return (__fputwc_unlock(wc, fp));

	bzero(&mbs, sizeof(mbs));
	len = wcrtomb(buf, wc, &mbs);
	if (len == (size_t)-1) {
		fp->_flags |= __SERR;
		errno = EILSEQ;
		return (WEOF);
	}
	uio.uio_iov = &iov;
	uio.uio_resid = len;
	uio.uio_iovcnt = 1;
	iov.iov_base = buf;
	iov.iov_len = len;
	return (__sfvwrite(fp, &uio) != EOF ? (wint_t)wc : WEOF);
}

/*
 * Convert a multibyte character string argument for the %s format to a wide
 * string representation. ``prec'' specifies the maximum number of bytes
 * to output. If ``prec'' is greater than or equal to zero, we can't assume
 * that the multibyte character string ends in a null character.
 * 
 * Returns NULL on failure.
 * To find out what happened check errno for ENOMEM, EILSEQ and EINVAL.
 */
static wchar_t *
__mbsconv(char *mbsarg, int prec)
{
	mbstate_t mbs;
	wchar_t *convbuf, *wcp;
	const char *p;
	size_t insize, nchars, nconv;

	if (mbsarg == NULL)
		return (NULL);

	/*
	 * Supplied argument is a multibyte string; convert it to wide
	 * characters first.
	 */
	if (prec >= 0) {
		/*
		 * String is not guaranteed to be NUL-terminated. Find the
		 * number of characters to print.
		 */
		p = mbsarg;
		insize = nchars = nconv = 0;
		bzero(&mbs, sizeof(mbs));
		while (nchars != (size_t)prec) {
			nconv = mbrlen(p, MB_CUR_MAX, &mbs);
			if (nconv == (size_t)0 || nconv == (size_t)-1 ||
			    nconv == (size_t)-2)
				break;
			p += nconv;
			nchars++;
			insize += nconv;
		}
		if (nconv == (size_t)-1 || nconv == (size_t)-2)
			return (NULL);
	} else
		insize = strlen(mbsarg);

	/*
	 * Allocate buffer for the result and perform the conversion,
	 * converting at most `size' bytes of the input multibyte string to
	 * wide characters for printing.
	 */
	convbuf = calloc(insize + 1, sizeof(*convbuf));
	if (convbuf == NULL)
		return (NULL);
	wcp = convbuf;
	p = mbsarg;
	bzero(&mbs, sizeof(mbs));
	nconv = 0;
	while (insize != 0) {
		nconv = mbrtowc(wcp, p, insize, &mbs);
		if (nconv == 0 || nconv == (size_t)-1 || nconv == (size_t)-2)
			break;
		wcp++;
		p += nconv;
		insize -= nconv;
	}
	if (nconv == (size_t)-1 || nconv == (size_t)-2) {
		free(convbuf);
		return (NULL);
	}
	*wcp = '\0';

	return (convbuf);
}

#ifdef FLOATING_POINT
#include <float.h>
#include <locale.h>
#include <math.h>
#include "floatio.h"
#include "gdtoa.h"

#define	DEFPREC		6

static int exponent(wchar_t *, int, int);
#endif /* FLOATING_POINT */

/*
 * The size of the buffer we use as scratch space for integer
 * conversions, among other things.  Technically, we would need the
 * most space for base 10 conversions with thousands' grouping
 * characters between each pair of digits.  100 bytes is a
 * conservative overestimate even for a 128-bit uintmax_t.
 */
#define BUF	100

#define STATIC_ARG_TBL_SIZE 8	/* Size of static argument table. */


/*
 * Macros for converting digits to letters and vice versa
 */
#define	to_digit(c)	((c) - '0')
#define is_digit(c)	((unsigned)to_digit(c) <= 9)
#define	to_char(n)	((wchar_t)((n) + '0'))

/*
 * Flags used during conversion.
 */
#define	ALT		0x0001		/* alternate form */
#define	LADJUST		0x0004		/* left adjustment */
#define	LONGDBL		0x0008		/* long double */
#define	LONGINT		0x0010		/* long integer */
#define	LLONGINT	0x0020		/* long long integer */
#define	SHORTINT	0x0040		/* short integer */
#define	ZEROPAD		0x0080		/* zero (as opposed to blank) pad */
#define FPT		0x0100		/* Floating point number */
#define PTRINT		0x0200		/* (unsigned) ptrdiff_t */
#define SIZEINT		0x0400		/* (signed) size_t */
#define CHARINT		0x0800		/* 8 bit integer */
#define MAXINT		0x1000		/* largest integer size (intmax_t) */

int
__vfwprintf(FILE * __restrict fp, const wchar_t * __restrict fmt0, __va_list ap)
{
	wchar_t *fmt;		/* format string */
	wchar_t ch;		/* character from fmt */
	int n, n2, n3;		/* handy integers (short term usage) */
	wchar_t *cp;		/* handy char pointer (short term usage) */
	int flags;		/* flags as above */
	int ret;		/* return value accumulator */
	int width;		/* width from format (%8d), or 0 */
	int prec;		/* precision from format; <0 for N/A */
	wchar_t sign;		/* sign prefix (' ', '+', '-', or \0) */
#ifdef FLOATING_POINT
	/*
	 * We can decompose the printed representation of floating
	 * point numbers into several parts, some of which may be empty:
	 *
	 * [+|-| ] [0x|0X] MMM . NNN [e|E|p|P] [+|-] ZZ
	 *    A       B     ---C---      D       E   F
	 *
	 * A:	'sign' holds this value if present; '\0' otherwise
	 * B:	ox[1] holds the 'x' or 'X'; '\0' if not hexadecimal
	 * C:	cp points to the string MMMNNN.  Leading and trailing
	 *	zeros are not in the string and must be added.
	 * D:	expchar holds this character; '\0' if no exponent, e.g. %f
	 * F:	at least two digits for decimal, at least one digit for hex
	 */
	char *decimal_point = NULL;
	int signflag;		/* true if float is negative */
	union {			/* floating point arguments %[aAeEfFgG] */
		double dbl;
		long double ldbl;
	} fparg;
	int expt;		/* integer value of exponent */
	char expchar;		/* exponent character: [eEpP\0] */
	char *dtoaend;		/* pointer to end of converted digits */
	int expsize;		/* character count for expstr */
	int lead;		/* sig figs before decimal or group sep */
	int ndig;		/* actual number of digits returned by dtoa */
	wchar_t expstr[MAXEXPDIG+2];	/* buffer for exponent string: e+ZZZ */
	char *dtoaresult = NULL;
#endif

	uintmax_t _umax;	/* integer arguments %[diouxX] */
	enum { OCT, DEC, HEX } base;	/* base for %[diouxX] conversion */
	int dprec;		/* a copy of prec if %[diouxX], 0 otherwise */
	int realsz;		/* field size expanded by dprec */
	int size;		/* size of converted field or string */
	const char *xdigs;	/* digits for %[xX] conversion */
	wchar_t buf[BUF];	/* buffer with space for digits of uintmax_t */
	wchar_t ox[2];		/* space for 0x; ox[1] is either x, X, or \0 */
	union arg *argtable;	/* args, built due to positional arg */
	union arg statargtable[STATIC_ARG_TBL_SIZE];
	size_t argtablesiz;
	int nextarg;		/* 1-based argument index */
	va_list orgap;		/* original argument pointer */
	wchar_t *convbuf;	/* buffer for multibyte to wide conversion */

	/*
	 * Choose PADSIZE to trade efficiency vs. size.  If larger printf
	 * fields occur frequently, increase PADSIZE and make the initialisers
	 * below longer.
	 */
#define	PADSIZE	16		/* pad chunk size */
	static wchar_t blanks[PADSIZE] =
	 {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '};
	static wchar_t zeroes[PADSIZE] =
	 {'0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0'};

	static const char xdigs_lower[16] = "0123456789abcdef";
	static const char xdigs_upper[16] = "0123456789ABCDEF";

	/*
	 * BEWARE, these `goto error' on error, PRINT uses 'n3',
	 * PAD uses `n' and 'n3', and PRINTANDPAD uses 'n', 'n2', and 'n3'.
	 */
#define	PRINT(ptr, len)	do {	\
	for (n3 = 0; n3 < (len); n3++) {	\
		if ((__xfputwc((ptr)[n3], fp)) == WEOF)	\
			goto error; \
	} \
} while (0)
#define	PAD(howmany, with) do { \
	if ((n = (howmany)) > 0) { \
		while (n > PADSIZE) { \
			PRINT(with, PADSIZE); \
			n -= PADSIZE; \
		} \
		PRINT(with, n); \
	} \
} while (0)
#define	PRINTANDPAD(p, ep, len, with) do {	\
	n2 = (ep) - (p);       			\
	if (n2 > (len))				\
		n2 = (len);			\
	if (n2 > 0)				\
		PRINT((p), n2);			\
	PAD((len) - (n2 > 0 ? n2 : 0), (with));	\
} while(0)

	/*
	 * To extend shorts properly, we need both signed and unsigned
	 * argument extraction methods.
	 */
#define	SARG() \
	((intmax_t)(flags&MAXINT ? GETARG(intmax_t) : \
	    flags&LLONGINT ? GETARG(long long) : \
	    flags&LONGINT ? GETARG(long) : \
	    flags&PTRINT ? GETARG(ptrdiff_t) : \
	    flags&SIZEINT ? GETARG(ssize_t) : \
	    flags&SHORTINT ? (short)GETARG(int) : \
	    flags&CHARINT ? (signed char)GETARG(int) : \
	    GETARG(int)))
#define	UARG() \
	((uintmax_t)(flags&MAXINT ? GETARG(uintmax_t) : \
	    flags&LLONGINT ? GETARG(unsigned long long) : \
	    flags&LONGINT ? GETARG(unsigned long) : \
	    flags&PTRINT ? (uintptr_t)GETARG(ptrdiff_t) : /* XXX */ \
	    flags&SIZEINT ? GETARG(size_t) : \
	    flags&SHORTINT ? (unsigned short)GETARG(int) : \
	    flags&CHARINT ? (unsigned char)GETARG(int) : \
	    GETARG(unsigned int)))

	/*
	 * Append a digit to a value and check for overflow.
	 */
#define APPEND_DIGIT(val, dig) do { \
	if ((val) > INT_MAX / 10) \
		goto overflow; \
	(val) *= 10; \
	if ((val) > INT_MAX - to_digit((dig))) \
		goto overflow; \
	(val) += to_digit((dig)); \
} while (0)

	 /*
	  * Get * arguments, including the form *nn$.  Preserve the nextarg
	  * that the argument can be gotten once the type is determined.
	  */
#define GETASTER(val) \
	n2 = 0; \
	cp = fmt; \
	while (is_digit(*cp)) { \
		APPEND_DIGIT(n2, *cp); \
		cp++; \
	} \
	if (*cp == '$') { \
		int hold = nextarg; \
		if (argtable == NULL) { \
			argtable = statargtable; \
			if (__find_arguments(fmt0, orgap, &argtable, \
			    &argtablesiz) == -1) { \
				ret = -1; \
				goto error; \
			} \
		} \
		nextarg = n2; \
		val = GETARG(int); \
		nextarg = hold; \
		fmt = ++cp; \
	} else { \
		val = GETARG(int); \
	}

/*
* Get the argument indexed by nextarg.   If the argument table is
* built, use it to get the argument.  If its not, get the next
* argument (and arguments must be gotten sequentially).
*/
#define GETARG(type) \
	((argtable != NULL) ? *((type*)(&argtable[nextarg++])) : \
		(nextarg++, va_arg(ap, type)))

	_SET_ORIENTATION(fp, 1);
	/* sorry, fwprintf(read_only_file, "") returns EOF, not 0 */
	if (cantwrite(fp))
		return (EOF);

	/* optimise fwprintf(stderr) (and other unbuffered Unix files) */
	if ((fp->_flags & (__SNBF|__SWR|__SRW)) == (__SNBF|__SWR) &&
	    fp->_file >= 0)
		return (__sbprintf(fp, fmt0, ap));

	fmt = (wchar_t *)fmt0;
	argtable = NULL;
	nextarg = 1;
	va_copy(orgap, ap);
	ret = 0;
	convbuf = NULL;

	/*
	 * Scan the format for conversions (`%' character).
	 */
	for (;;) {
		for (cp = fmt; (ch = *fmt) != '\0' && ch != '%'; fmt++)
			continue;
		if (fmt != cp) {
			ptrdiff_t m = fmt - cp;
			if (m < 0 || m > INT_MAX - ret)
				goto overflow;
			PRINT(cp, m);
			ret += m;
		}
		if (ch == '\0')
			goto done;
		fmt++;		/* skip over '%' */

		flags = 0;
		dprec = 0;
		width = 0;
		prec = -1;
		sign = '\0';
		ox[1] = '\0';

rflag:		ch = *fmt++;
reswitch:	switch (ch) {
		case ' ':
			/*
			 * ``If the space and + flags both appear, the space
			 * flag will be ignored.''
			 *	-- ANSI X3J11
			 */
			if (!sign)
				sign = ' ';
			goto rflag;
		case '#':
			flags |= ALT;
			goto rflag;
		case '\'':
			/* grouping not implemented */
			goto rflag;
		case '*':
			/*
			 * ``A negative field width argument is taken as a
			 * - flag followed by a positive field width.''
			 *	-- ANSI X3J11
			 * They don't exclude field widths read from args.
			 */
			GETASTER(width);
			if (width >= 0)
				goto rflag;
			if (width == INT_MIN)
				goto overflow;
			width = -width;
			/* FALLTHROUGH */
		case '-':
			flags |= LADJUST;
			goto rflag;
		case '+':
			sign = '+';
			goto rflag;
		case '.':
			if ((ch = *fmt++) == '*') {
				GETASTER(n);
				prec = n < 0 ? -1 : n;
				goto rflag;
			}
			n = 0;
			while (is_digit(ch)) {
				APPEND_DIGIT(n, ch);
				ch = *fmt++;
			}
			if (ch == '$') {
				nextarg = n;
				if (argtable == NULL) {
					argtable = statargtable;
					if (__find_arguments(fmt0, orgap,
					    &argtable, &argtablesiz) == -1) {
						ret = -1;
						goto error;
					}
				}
				goto rflag;
			}
			prec = n;
			goto reswitch;
		case '0':
			/*
			 * ``Note that 0 is taken as a flag, not as the
			 * beginning of a field width.''
			 *	-- ANSI X3J11
			 */
			flags |= ZEROPAD;
			goto rflag;
		case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			n = 0;
			do {
				APPEND_DIGIT(n, ch);
				ch = *fmt++;
			} while (is_digit(ch));
			if (ch == '$') {
				nextarg = n;
				if (argtable == NULL) {
					argtable = statargtable;
					if (__find_arguments(fmt0, orgap,
					    &argtable, &argtablesiz) == -1) {
						ret = -1;
						goto error;
					}
				}
				goto rflag;
			}
			width = n;
			goto reswitch;
#ifdef FLOATING_POINT
		case 'L':
			flags |= LONGDBL;
			goto rflag;
#endif
		case 'h':
			if (*fmt == 'h') {
				fmt++;
				flags |= CHARINT;
			} else {
				flags |= SHORTINT;
			}
			goto rflag;
		case 'j':
			flags |= MAXINT;
			goto rflag;
		case 'l':
			if (*fmt == 'l') {
				fmt++;
				flags |= LLONGINT;
			} else {
				flags |= LONGINT;
			}
			goto rflag;
		case 'q':
			flags |= LLONGINT;
			goto rflag;
		case 't':
			flags |= PTRINT;
			goto rflag;
		case 'z':
			flags |= SIZEINT;
			goto rflag;
		case 'C':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'c':
			if (flags & LONGINT)
				*(cp = buf) = (wchar_t)GETARG(wint_t);
			else
				*(cp = buf) = (wchar_t)btowc(GETARG(int));
			size = 1;
			sign = '\0';
			break;
		case 'D':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'd':
		case 'i':
			_umax = SARG();
			if ((intmax_t)_umax < 0) {
				_umax = -_umax;
				sign = '-';
			}
			base = DEC;
			goto number;
#ifdef FLOATING_POINT
		case 'a':
		case 'A':
			if (ch == 'a') {
				ox[1] = 'x';
				xdigs = xdigs_lower;
				expchar = 'p';
			} else {
				ox[1] = 'X';
				xdigs = xdigs_upper;
				expchar = 'P';
			}
			if (prec >= 0)
				prec++;
			if (dtoaresult)
				__freedtoa(dtoaresult);
			if (flags & LONGDBL) {
				fparg.ldbl = GETARG(long double);
				dtoaresult =
				    __hldtoa(fparg.ldbl, xdigs, prec,
				    &expt, &signflag, &dtoaend);
				if (dtoaresult == NULL) {
					errno = ENOMEM;
					goto error;
				}
			} else {
				fparg.dbl = GETARG(double);
				dtoaresult =
				    __hdtoa(fparg.dbl, xdigs, prec,
				    &expt, &signflag, &dtoaend);
				if (dtoaresult == NULL) {
					errno = ENOMEM;
					goto error;
				}
			}
			if (prec < 0)
				prec = dtoaend - dtoaresult;
			if (expt == INT_MAX)
				ox[1] = '\0';
			free(convbuf);
			cp = convbuf = __mbsconv(dtoaresult, -1);
			if (cp == NULL)
				goto error;
			ndig = dtoaend - dtoaresult;
			goto fp_common;
		case 'e':
		case 'E':
			expchar = ch;
			if (prec < 0)	/* account for digit before decpt */
				prec = DEFPREC + 1;
			else
				prec++;
			goto fp_begin;
		case 'f':
		case 'F':
			expchar = '\0';
			goto fp_begin;
		case 'g':
		case 'G':
			expchar = ch - ('g' - 'e');
 			if (prec == 0)
 				prec = 1;
fp_begin:
			if (prec < 0)
				prec = DEFPREC;
			if (dtoaresult)
				__freedtoa(dtoaresult);
			if (flags & LONGDBL) {
				fparg.ldbl = GETARG(long double);
				dtoaresult =
				    __ldtoa(&fparg.ldbl, expchar ? 2 : 3, prec,
				    &expt, &signflag, &dtoaend);
				if (dtoaresult == NULL) {
					errno = ENOMEM;
					goto error;
				}
			} else {
				fparg.dbl = GETARG(double);
				dtoaresult =
				    __dtoa(fparg.dbl, expchar ? 2 : 3, prec,
				    &expt, &signflag, &dtoaend);
				if (dtoaresult == NULL) {
					errno = ENOMEM;
					goto error;
				}
				if (expt == 9999)
					expt = INT_MAX;
 			}
			free(convbuf);
			cp = convbuf = __mbsconv(dtoaresult, -1);
			if (cp == NULL)
				goto error;
			ndig = dtoaend - dtoaresult;
fp_common:
			if (signflag)
				sign = '-';
			if (expt == INT_MAX) {	/* inf or nan */
				if (*cp == 'N')
					cp = (ch >= 'a') ? L"nan" : L"NAN";
				else
					cp = (ch >= 'a') ? L"inf" : L"INF";
 				size = 3;
				flags &= ~ZEROPAD;
 				break;
 			}
			flags |= FPT;
 			if (ch == 'g' || ch == 'G') {
				if (expt > -4 && expt <= prec) {
					/* Make %[gG] smell like %[fF] */
					expchar = '\0';
					if (flags & ALT)
						prec -= expt;
					else
						prec = ndig - expt;
					if (prec < 0)
						prec = 0;
				} else {
					/*
					 * Make %[gG] smell like %[eE], but
					 * trim trailing zeroes if no # flag.
					 */
					if (!(flags & ALT))
						prec = ndig;
				}
 			}
			if (expchar) {
				expsize = exponent(expstr, expt - 1, expchar);
				size = expsize + prec;
				if (prec > 1 || flags & ALT)
 					++size;
			} else {
				/* space for digits before decimal point */
				if (expt > 0)
					size = expt;
				else	/* "0" */
					size = 1;
				/* space for decimal pt and following digits */
				if (prec || flags & ALT)
					size += prec + 1;
				lead = expt;
			}
			break;
#endif /* FLOATING_POINT */
		case 'n': {
			static const char n_msg[] = ": *wprintf used %n, aborting";
			char buf[1024], *p;

			/* <10> is LOG_CRIT */
			strlcpy(buf, "<10>", sizeof buf);

			/* Make sure progname does not fill the whole buffer */
			strlcat(buf, __progname, sizeof(buf) - sizeof n_msg);
			strlcat(buf, n_msg, sizeof buf);
			/*
			 * vfprintf sends fmt0 via syslog, but this is not
			 * good behaviour for wide strings.
			 */
			if ((p = strchr(buf, '\n')))
				*p = '\0';
			sendsyslog(buf, strlen(buf), LOG_CONS);
			abort();
			break;
			}
		case 'O':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'o':
			_umax = UARG();
			base = OCT;
			goto nosign;
		case 'p':
			/*
			 * ``The argument shall be a pointer to void.  The
			 * value of the pointer is converted to a sequence
			 * of printable characters, in an implementation-
			 * defined manner.''
			 *	-- ANSI X3J11
			 */
			_umax = (u_long)GETARG(void *);
			base = HEX;
			xdigs = xdigs_lower;
			ox[1] = 'x';
			goto nosign;
		case 'S':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 's':
			if (flags & LONGINT) {
				if ((cp = GETARG(wchar_t *)) == NULL) {
					struct syslog_data sdata = SYSLOG_DATA_INIT;
					int save_errno = errno;

					syslog_r(LOG_CRIT | LOG_CONS, &sdata,
					    "vfwprintf %%ls NULL in \"%ls\"", fmt0);
					errno = save_errno;

					cp = L"(null)";
				}
			} else {
				char *mbsarg;
				if ((mbsarg = GETARG(char *)) == NULL) {
					struct syslog_data sdata = SYSLOG_DATA_INIT;
					int save_errno = errno;

					syslog_r(LOG_CRIT | LOG_CONS, &sdata,
					    "vfwprintf %%s NULL in \"%ls\"", fmt0);
					errno = save_errno;

					mbsarg = "(null)";
				}
				free(convbuf);
				convbuf = __mbsconv(mbsarg, prec);
				if (convbuf == NULL) {
					fp->_flags |= __SERR;
					goto error;
				} else
					cp = convbuf;
			}
			if (prec >= 0) {
				/*
				 * can't use wcslen; can only look for the
				 * NUL in the first `prec' characters, and
				 * wcslen() will go further.
				 */
				wchar_t *p = wmemchr(cp, 0, prec);

				size = p ? (p - cp) : prec;
			} else {
				size_t len;

				if ((len = wcslen(cp)) > INT_MAX)
					goto overflow;
				size = (int)len;
			}
			sign = '\0';
			break;
		case 'U':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'u':
			_umax = UARG();
			base = DEC;
			goto nosign;
		case 'X':
			xdigs = xdigs_upper;
			goto hex;
		case 'x':
			xdigs = xdigs_lower;
hex:			_umax = UARG();
			base = HEX;
			/* leading 0x/X only if non-zero */
			if (flags & ALT && _umax != 0)
				ox[1] = ch;

			/* unsigned conversions */
nosign:			sign = '\0';
			/*
			 * ``... diouXx conversions ... if a precision is
			 * specified, the 0 flag will be ignored.''
			 *	-- ANSI X3J11
			 */
number:			if ((dprec = prec) >= 0)
				flags &= ~ZEROPAD;

			/*
			 * ``The result of converting a zero value with an
			 * explicit precision of zero is no characters.''
			 *	-- ANSI X3J11
			 */
			cp = buf + BUF;
			if (_umax != 0 || prec != 0) {
				/*
				 * Unsigned mod is hard, and unsigned mod
				 * by a constant is easier than that by
				 * a variable; hence this switch.
				 */
				switch (base) {
				case OCT:
					do {
						*--cp = to_char(_umax & 7);
						_umax >>= 3;
					} while (_umax);
					/* handle octal leading 0 */
					if (flags & ALT && *cp != '0')
						*--cp = '0';
					break;

				case DEC:
					/* many numbers are 1 digit */
					while (_umax >= 10) {
						*--cp = to_char(_umax % 10);
						_umax /= 10;
					}
					*--cp = to_char(_umax);
					break;

				case HEX:
					do {
						*--cp = xdigs[_umax & 15];
						_umax >>= 4;
					} while (_umax);
					break;

				default:
					cp = L"bug in vfwprintf: bad base";
					size = wcslen(cp);
					goto skipsize;
				}
			}
			size = buf + BUF - cp;
			if (size > BUF)	/* should never happen */
				abort();
		skipsize:
			break;
		default:	/* "%?" prints ?, unless ? is NUL */
			if (ch == '\0')
				goto done;
			/* pretend it was %c with argument ch */
			cp = buf;
			*cp = ch;
			size = 1;
			sign = '\0';
			break;
		}

		/*
		 * All reasonable formats wind up here.  At this point, `cp'
		 * points to a string which (if not flags&LADJUST) should be
		 * padded out to `width' places.  If flags&ZEROPAD, it should
		 * first be prefixed by any sign or other prefix; otherwise,
		 * it should be blank padded before the prefix is emitted.
		 * After any left-hand padding and prefixing, emit zeroes
		 * required by a decimal %[diouxX] precision, then print the
		 * string proper, then emit zeroes required by any leftover
		 * floating precision; finally, if LADJUST, pad with blanks.
		 *
		 * Compute actual size, so we know how much to pad.
		 * size excludes decimal prec; realsz includes it.
		 */
		realsz = dprec > size ? dprec : size;
		if (sign)
			realsz++;
		if (ox[1])
			realsz+= 2;

		/* right-adjusting blank padding */
		if ((flags & (LADJUST|ZEROPAD)) == 0)
			PAD(width - realsz, blanks);

		/* prefix */
		if (sign)
			PRINT(&sign, 1);
		if (ox[1]) {	/* ox[1] is either x, X, or \0 */
			ox[0] = '0';
			PRINT(ox, 2);
		}

		/* right-adjusting zero padding */
		if ((flags & (LADJUST|ZEROPAD)) == ZEROPAD)
			PAD(width - realsz, zeroes);

		/* leading zeroes from decimal precision */
		PAD(dprec - size, zeroes);

		/* the string or number proper */
#ifdef FLOATING_POINT
		if ((flags & FPT) == 0) {
			PRINT(cp, size);
		} else {	/* glue together f_p fragments */
			if (decimal_point == NULL)
				decimal_point = nl_langinfo(RADIXCHAR);
			if (!expchar) {	/* %[fF] or sufficiently short %[gG] */
				if (expt <= 0) {
					PRINT(zeroes, 1);
					if (prec || flags & ALT)
						PRINT(decimal_point, 1);
					PAD(-expt, zeroes);
					/* already handled initial 0's */
					prec += expt;
 				} else {
					PRINTANDPAD(cp, convbuf + ndig,
					    lead, zeroes);
					cp += lead;
					if (prec || flags & ALT)
						PRINT(decimal_point, 1);
				}
				PRINTANDPAD(cp, convbuf + ndig, prec, zeroes);
			} else {	/* %[eE] or sufficiently long %[gG] */
				if (prec > 1 || flags & ALT) {
					buf[0] = *cp++;
					buf[1] = *decimal_point;
					PRINT(buf, 2);
					PRINT(cp, ndig-1);
					PAD(prec - ndig, zeroes);
				} else { /* XeYYY */
					PRINT(cp, 1);
				}
				PRINT(expstr, expsize);
			}
		}
#else
		PRINT(cp, size);
#endif
		/* left-adjusting padding (always blank) */
		if (flags & LADJUST)
			PAD(width - realsz, blanks);

		/* finally, adjust ret */
		if (width < realsz)
			width = realsz;
		if (width > INT_MAX - ret)
			goto overflow;
		ret += width;
	}
done:
error:
	va_end(orgap);
	if (__sferror(fp))
		ret = -1;
	goto finish;

overflow:
	errno = EOVERFLOW;
	ret = -1;

finish:
	free(convbuf);
#ifdef FLOATING_POINT
	if (dtoaresult)
		__freedtoa(dtoaresult);
#endif
	if (argtable != NULL && argtable != statargtable) {
		munmap(argtable, argtablesiz);
		argtable = NULL;
	}
	return (ret);
}

int
vfwprintf(FILE * __restrict fp, const wchar_t * __restrict fmt0, __va_list ap)
{
	int r;

	FLOCKFILE(fp);
	r = __vfwprintf(fp, fmt0, ap);
	FUNLOCKFILE(fp);

	return (r);
}
DEF_STRONG(vfwprintf);

/*
 * Type ids for argument type table.
 */
#define T_UNUSED	0
#define T_SHORT		1
#define T_U_SHORT	2
#define TP_SHORT	3
#define T_INT		4
#define T_U_INT		5
#define TP_INT		6
#define T_LONG		7
#define T_U_LONG	8
#define TP_LONG		9
#define T_LLONG		10
#define T_U_LLONG	11
#define TP_LLONG	12
#define T_DOUBLE	13
#define T_LONG_DOUBLE	14
#define TP_CHAR		15
#define TP_VOID		16
#define T_PTRINT	17
#define TP_PTRINT	18
#define T_SIZEINT	19
#define T_SSIZEINT	20
#define TP_SSIZEINT	21
#define T_MAXINT	22
#define T_MAXUINT	23
#define TP_MAXINT	24
#define T_CHAR		25
#define T_U_CHAR	26
#define T_WINT		27
#define TP_WCHAR	28

/*
 * Find all arguments when a positional parameter is encountered.  Returns a
 * table, indexed by argument number, of pointers to each arguments.  The
 * initial argument table should be an array of STATIC_ARG_TBL_SIZE entries.
 * It will be replaced with a mmap-ed one if it overflows (malloc cannot be
 * used since we are attempting to make snprintf thread safe, and alloca is
 * problematic since we have nested functions..)
 */
static int
__find_arguments(const wchar_t *fmt0, va_list ap, union arg **argtable,
    size_t *argtablesiz)
{
	wchar_t *fmt;		/* format string */
	int ch;			/* character from fmt */
	int n, n2;		/* handy integer (short term usage) */
	wchar_t *cp;		/* handy char pointer (short term usage) */
	int flags;		/* flags as above */
	unsigned char *typetable; /* table of types */
	unsigned char stattypetable[STATIC_ARG_TBL_SIZE];
	int tablesize;		/* current size of type table */
	int tablemax;		/* largest used index in table */
	int nextarg;		/* 1-based argument index */
	int ret = 0;		/* return value */

	/*
	 * Add an argument type to the table, expanding if necessary.
	 */
#define ADDTYPE(type) \
	((nextarg >= tablesize) ? \
		__grow_type_table(&typetable, &tablesize) : 0, \
	(nextarg > tablemax) ? tablemax = nextarg : 0, \
	typetable[nextarg++] = type)

#define	ADDSARG() \
        ((flags&MAXINT) ? ADDTYPE(T_MAXINT) : \
	    ((flags&PTRINT) ? ADDTYPE(T_PTRINT) : \
	    ((flags&SIZEINT) ? ADDTYPE(T_SSIZEINT) : \
	    ((flags&LLONGINT) ? ADDTYPE(T_LLONG) : \
	    ((flags&LONGINT) ? ADDTYPE(T_LONG) : \
	    ((flags&SHORTINT) ? ADDTYPE(T_SHORT) : \
	    ((flags&CHARINT) ? ADDTYPE(T_CHAR) : ADDTYPE(T_INT))))))))

#define	ADDUARG() \
        ((flags&MAXINT) ? ADDTYPE(T_MAXUINT) : \
	    ((flags&PTRINT) ? ADDTYPE(T_PTRINT) : \
	    ((flags&SIZEINT) ? ADDTYPE(T_SIZEINT) : \
	    ((flags&LLONGINT) ? ADDTYPE(T_U_LLONG) : \
	    ((flags&LONGINT) ? ADDTYPE(T_U_LONG) : \
	    ((flags&SHORTINT) ? ADDTYPE(T_U_SHORT) : \
	    ((flags&CHARINT) ? ADDTYPE(T_U_CHAR) : ADDTYPE(T_U_INT))))))))

	/*
	 * Add * arguments to the type array.
	 */
#define ADDASTER() \
	n2 = 0; \
	cp = fmt; \
	while (is_digit(*cp)) { \
		APPEND_DIGIT(n2, *cp); \
		cp++; \
	} \
	if (*cp == '$') { \
		int hold = nextarg; \
		nextarg = n2; \
		ADDTYPE(T_INT); \
		nextarg = hold; \
		fmt = ++cp; \
	} else { \
		ADDTYPE(T_INT); \
	}
	fmt = (wchar_t *)fmt0;
	typetable = stattypetable;
	tablesize = STATIC_ARG_TBL_SIZE;
	tablemax = 0;
	nextarg = 1;
	memset(typetable, T_UNUSED, STATIC_ARG_TBL_SIZE);

	/*
	 * Scan the format for conversions (`%' character).
	 */
	for (;;) {
		for (cp = fmt; (ch = *fmt) != '\0' && ch != '%'; fmt++)
			continue;
		if (ch == '\0')
			goto done;
		fmt++;		/* skip over '%' */

		flags = 0;

rflag:		ch = *fmt++;
reswitch:	switch (ch) {
		case ' ':
		case '#':
		case '\'':
			goto rflag;
		case '*':
			ADDASTER();
			goto rflag;
		case '-':
		case '+':
			goto rflag;
		case '.':
			if ((ch = *fmt++) == '*') {
				ADDASTER();
				goto rflag;
			}
			while (is_digit(ch)) {
				ch = *fmt++;
			}
			goto reswitch;
		case '0':
			goto rflag;
		case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			n = 0;
			do {
				APPEND_DIGIT(n ,ch);
				ch = *fmt++;
			} while (is_digit(ch));
			if (ch == '$') {
				nextarg = n;
				goto rflag;
			}
			goto reswitch;
#ifdef FLOATING_POINT
		case 'L':
			flags |= LONGDBL;
			goto rflag;
#endif
		case 'h':
			if (*fmt == 'h') {
				fmt++;
				flags |= CHARINT;
			} else {
				flags |= SHORTINT;
			}
			goto rflag;
		case 'l':
			if (*fmt == 'l') {
				fmt++;
				flags |= LLONGINT;
			} else {
				flags |= LONGINT;
			}
			goto rflag;
		case 'q':
			flags |= LLONGINT;
			goto rflag;
		case 't':
			flags |= PTRINT;
			goto rflag;
		case 'z':
			flags |= SIZEINT;
			goto rflag;
		case 'C':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'c':
			if (flags & LONGINT)
				ADDTYPE(T_WINT);
			else
				ADDTYPE(T_INT);
			break;
		case 'D':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'd':
		case 'i':
			ADDSARG();
			break;
#ifdef FLOATING_POINT
		case 'a':
		case 'A':
		case 'e':
		case 'E':
		case 'f':
		case 'F':
		case 'g':
		case 'G':
			if (flags & LONGDBL)
				ADDTYPE(T_LONG_DOUBLE);
			else
				ADDTYPE(T_DOUBLE);
			break;
#endif /* FLOATING_POINT */
		case 'n':
			if (flags & LLONGINT)
				ADDTYPE(TP_LLONG);
			else if (flags & LONGINT)
				ADDTYPE(TP_LONG);
			else if (flags & SHORTINT)
				ADDTYPE(TP_SHORT);
			else if (flags & PTRINT)
				ADDTYPE(TP_PTRINT);
			else if (flags & SIZEINT)
				ADDTYPE(TP_SSIZEINT);
			else if (flags & MAXINT)
				ADDTYPE(TP_MAXINT);
			else
				ADDTYPE(TP_INT);
			continue;	/* no output */
		case 'O':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'o':
			ADDUARG();
			break;
		case 'p':
			ADDTYPE(TP_VOID);
			break;
		case 'S':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 's':
			if (flags & LONGINT)
				ADDTYPE(TP_CHAR);
			else
				ADDTYPE(TP_WCHAR);
			break;
		case 'U':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'u':
		case 'X':
		case 'x':
			ADDUARG();
			break;
		default:	/* "%?" prints ?, unless ? is NUL */
			if (ch == '\0')
				goto done;
			break;
		}
	}
done:
	/*
	 * Build the argument table.
	 */
	if (tablemax >= STATIC_ARG_TBL_SIZE) {
		*argtablesiz = sizeof(union arg) * (tablemax + 1);
		*argtable = mmap(NULL, *argtablesiz,
		    PROT_WRITE|PROT_READ, MAP_ANON|MAP_PRIVATE, -1, 0);
		if (*argtable == MAP_FAILED)
			return (-1);
	}

#if 0
	/* XXX is this required? */
	(*argtable)[0].intarg = 0;
#endif
	for (n = 1; n <= tablemax; n++) {
		switch (typetable[n]) {
		case T_UNUSED:
		case T_CHAR:
		case T_U_CHAR:
		case T_SHORT:
		case T_U_SHORT:
		case T_INT:
			(*argtable)[n].intarg = va_arg(ap, int);
			break;
		case TP_SHORT:
			(*argtable)[n].pshortarg = va_arg(ap, short *);
			break;
		case T_U_INT:
			(*argtable)[n].uintarg = va_arg(ap, unsigned int);
			break;
		case TP_INT:
			(*argtable)[n].pintarg = va_arg(ap, int *);
			break;
		case T_LONG:
			(*argtable)[n].longarg = va_arg(ap, long);
			break;
		case T_U_LONG:
			(*argtable)[n].ulongarg = va_arg(ap, unsigned long);
			break;
		case TP_LONG:
			(*argtable)[n].plongarg = va_arg(ap, long *);
			break;
		case T_LLONG:
			(*argtable)[n].longlongarg = va_arg(ap, long long);
			break;
		case T_U_LLONG:
			(*argtable)[n].ulonglongarg = va_arg(ap, unsigned long long);
			break;
		case TP_LLONG:
			(*argtable)[n].plonglongarg = va_arg(ap, long long *);
			break;
#ifdef FLOATING_POINT
		case T_DOUBLE:
			(*argtable)[n].doublearg = va_arg(ap, double);
			break;
		case T_LONG_DOUBLE:
			(*argtable)[n].longdoublearg = va_arg(ap, long double);
			break;
#endif
		case TP_CHAR:
			(*argtable)[n].pchararg = va_arg(ap, char *);
			break;
		case TP_VOID:
			(*argtable)[n].pvoidarg = va_arg(ap, void *);
			break;
		case T_PTRINT:
			(*argtable)[n].ptrdiffarg = va_arg(ap, ptrdiff_t);
			break;
		case TP_PTRINT:
			(*argtable)[n].pptrdiffarg = va_arg(ap, ptrdiff_t *);
			break;
		case T_SIZEINT:
			(*argtable)[n].sizearg = va_arg(ap, size_t);
			break;
		case T_SSIZEINT:
			(*argtable)[n].ssizearg = va_arg(ap, ssize_t);
			break;
		case TP_SSIZEINT:
			(*argtable)[n].pssizearg = va_arg(ap, ssize_t *);
			break;
		case TP_MAXINT:
			(*argtable)[n].intmaxarg = va_arg(ap, intmax_t);
			break;
		case T_WINT:
			(*argtable)[n].wintarg = va_arg(ap, wint_t);
			break;
		case TP_WCHAR:
			(*argtable)[n].pwchararg = va_arg(ap, wchar_t *);
			break;
		}
	}
	goto finish;

overflow:
	errno = EOVERFLOW;
	ret = -1;

finish:
	if (typetable != NULL && typetable != stattypetable) {
		munmap(typetable, *argtablesiz);
		typetable = NULL;
	}
	return (ret);
}

/*
 * Increase the size of the type table.
 */
static int
__grow_type_table(unsigned char **typetable, int *tablesize)
{
	unsigned char *oldtable = *typetable;
	int newsize = *tablesize * 2;

	if (newsize < getpagesize())
		newsize = getpagesize();

	if (*tablesize == STATIC_ARG_TBL_SIZE) {
		*typetable = mmap(NULL, newsize, PROT_WRITE|PROT_READ,
		    MAP_ANON|MAP_PRIVATE, -1, 0);
		if (*typetable == MAP_FAILED)
			return (-1);
		bcopy(oldtable, *typetable, *tablesize);
	} else {
		unsigned char *new = mmap(NULL, newsize, PROT_WRITE|PROT_READ,
		    MAP_ANON|MAP_PRIVATE, -1, 0);
		if (new == MAP_FAILED)
			return (-1);
		memmove(new, *typetable, *tablesize);
		munmap(*typetable, *tablesize);
		*typetable = new;
	}
	memset(*typetable + *tablesize, T_UNUSED, (newsize - *tablesize));

	*tablesize = newsize;
	return (0);
}

 
#ifdef FLOATING_POINT
static int
exponent(wchar_t *p0, int exp, int fmtch)
{
	wchar_t *p, *t;
	wchar_t expbuf[MAXEXPDIG];

	p = p0;
	*p++ = fmtch;
	if (exp < 0) {
		exp = -exp;
		*p++ = '-';
	} else
		*p++ = '+';
	t = expbuf + MAXEXPDIG;
	if (exp > 9) {
		do {
			*--t = to_char(exp % 10);
		} while ((exp /= 10) > 9);
		*--t = to_char(exp);
		for (; t < expbuf + MAXEXPDIG; *p++ = *t++)
			/* nothing */;
	} else {
		/*
		 * Exponents for decimal floating point conversions
		 * (%[eEgG]) must be at least two characters long,
		 * whereas exponents for hexadecimal conversions can
		 * be only one character long.
		 */
		if (fmtch == 'e' || fmtch == 'E')
			*p++ = '0';
		*p++ = to_char(exp);
	}
	return (p - p0);
}
#endif /* FLOATING_POINT */
