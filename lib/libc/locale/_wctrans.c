/*	$OpenBSD: _wctrans.c,v 1.3 2024/08/18 02:22:29 guenther Exp $ */
/*	$NetBSD: _wctrans.c,v 1.6 2005/02/10 19:19:57 tnozaki Exp $	*/

/*-
 * Copyright (c)2003 Citrus Project,
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*-
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Paul Borman at Krystal Technologies.
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

#include <wctype.h>
#include <stdlib.h>
#include <string.h>
#include "runetype.h"
#include "_wctrans_local.h"

/*
 * _wctrans_init:
 */

void
_wctrans_init(_RuneLocale *rl)
{
	rl->rl_wctrans[_WCTRANS_INDEX_LOWER].te_name = "tolower";
	rl->rl_wctrans[_WCTRANS_INDEX_LOWER].te_cached = rl->rl_maplower;
	rl->rl_wctrans[_WCTRANS_INDEX_LOWER].te_extmap = &rl->rl_maplower_ext;
	rl->rl_wctrans[_WCTRANS_INDEX_UPPER].te_name = "toupper";
	rl->rl_wctrans[_WCTRANS_INDEX_UPPER].te_cached = rl->rl_mapupper;
	rl->rl_wctrans[_WCTRANS_INDEX_UPPER].te_extmap = &rl->rl_mapupper_ext;
}

/*
 * _wctrans_ext:
 *	translate a character (extended part)
 */
wint_t
_towctrans_ext(wint_t c, struct _WCTransEntry *te)
{
	rune_t c0;
	uint32_t x;
	_RuneRange *rr;
	_RuneEntry *base, *re;

	if (c == WEOF)
		return (c);

	c0 = (rune_t)c; /* XXX assumes wchar_t = int */
	rr = te->te_extmap;
	base = rr->rr_rune_ranges;
	for (x = rr->rr_nranges; x != 0; x >>= 1) {
		re = base + (x >> 1);
		if (re->re_min <= c0 && re->re_max >= c0)
			return (re->re_map + c0 - re->re_min);
		else if (c0 >= re->re_max) {
			base = re + 1;
			x--;
		}
	}

	return (c);
}
