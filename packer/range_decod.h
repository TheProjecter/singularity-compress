/*
 * Singularity-compress: rangecoder decoder
 * Copyright (C) 2006-2007  Torok Edwin (edwintorok@gmail.com)
 *
 * Based on Michael Schindler's rangecoder, which is:
 * (c) Michael Schindler 1997, 1998, 1999, 2000
 * http://www.compressconsult.com/
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef NDEBUG
#define DO_CHEKCS
#endif

#include "rangecod.h"

/* Start the decoder                                         */
/* rc is the range coder to be used                          */
/* returns the char from start_encoding or EOF               */
static inline int start_decoding( rangecoder *rc )
{
	int c = inbyte(rc);
	if (c==EOF)
		return EOF;
	rc->buffer = inbyte(rc);
	rc->low = rc->buffer >> (8-EXTRA_BITS);
	rc->range = (code_value)1 << EXTRA_BITS;
	return c;
}

static inline void dec_normalize( rangecoder *rc )
{   
	while (rc->range <= Bottom_value)   {
		rc->low = (rc->low<<8) | ((rc->buffer<<EXTRA_BITS)&0xff);
		rc->buffer = inbyte(rc);
	 	rc->low |= rc->buffer >> (8-EXTRA_BITS);
		rc->range <<= 8;
	}
}


/* Calculate culmulative frequency for next symbol. Does NO update!*/
/* rc is the range coder to be used                          */
/* tot_f is the total frequency                              */
/* or: totf is (code_value)1<<shift                                      */
/* returns the culmulative frequency                         */
static inline freq decode_culfreq( rangecoder *rc, freq tot_f )
{   
	freq tmp;
	dec_normalize(rc);
	rc->help = rc->range/tot_f;
	tmp = rc->low/rc->help;
	return (tmp>=tot_f ? tot_f-1 : tmp);
}



static inline freq decode_culshift( rangecoder *rc, freq shift )
{   
	freq tmp;
	dec_normalize(rc);
	rc->help = rc->range>>shift;
    	tmp = rc->low/rc->help;
	return (tmp>>shift ? ((code_value)1<<shift)-1 : tmp);
}


/* Update decoding state                                     */
/* rc is the range coder to be used                          */
/* sy_f is the interval length (frequency of the symbol)     */
/* lt_f is the lower end (frequency sum of < symbols)        */
/* tot_f is the total interval length (total frequency sum)  */
static inline void decode_update( rangecoder *rc, freq sy_f, freq lt_f, freq tot_f)
{   
	code_value tmp;
	tmp = rc->help * lt_f;
	rc->low -= tmp;
	if (lt_f + sy_f < tot_f)
		rc->range = rc->help * sy_f;
	else
		rc->range -= tmp;
}

#define decode_update_shift(rc,f1,f2,f3) decode_update((rc),(f1),(f2),(freq)1<<(f3));

/* Decode a byte/short without modelling                     */
/* rc is the range coder to be used                          */
static inline unsigned char decode_byte(rangecoder *rc)
{   
	unsigned char tmp = decode_culshift(rc,8);
	decode_update( rc,1,tmp,(freq)1<<8);
	return tmp;
}

static inline unsigned short decode_short(rangecoder *rc)
{   
	unsigned short tmp = decode_culshift(rc,16);
	decode_update( rc,1,tmp,(freq)1<<16);
	return tmp;
}

/* Finish decoding                                           */
/* rc is the range coder to be used                          */
static inline void done_decoding( rangecoder *rc )
{   
	dec_normalize(rc);      /* normalize to use up all bytes */
}
