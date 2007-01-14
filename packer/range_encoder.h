/*
 * Singularity-compress: rangecoder encoder
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

#ifndef _RANGE_ENCODER_H
#define _RANGE_ENCODER_H
#include "../common/rangecod.h"

/* rc is the range coder to be used                            */
/* c is written as first byte in the datastream                */
/* one could do without c, but then you have an additional if  */
/* per outputbyte.                                             */
static void start_encoding( rangecoder *rc, char c, int initlength )
{   
	rc->low = 0;                /* Full code range */
	rc->range = Top_value;
	rc->buffer = c;
	rc->help = 0;               /* No bytes to follow */
	rc->bytecount = initlength;
}

static inline void enc_normalize( rangecoder *rc )
{  
	while(rc->range <= Bottom_value) {
		/* do we need renormalisation?  */
	     	if (rc->low < (code_value)0xff << SHIFT_BITS) {
			/* no carry possible --> output */
			outbyte(rc, rc->buffer);
			for(; rc->help; rc->help--)
				outbyte(rc, 0xff);
			rc->buffer = (unsigned char)(rc->low >> SHIFT_BITS);
		} else if (rc->low & Top_value) {
			/* carry now, no future carry */
			outbyte(rc,rc->buffer+1);
			for(; rc->help; rc->help--)
				outbyte(rc,0);
			rc->buffer = (unsigned char)(rc->low >> SHIFT_BITS);
		} else  {
			/* passes on a potential carry */
#ifndef DO_CHECKS
			rc->help++;
#else
			if (rc->bytestofollow++ == 0xffffffffL) {
				fprintf(stderr,"Too many bytes outstanding - File too large\n");
				exit(1);
			}
#endif
		}

		rc->range <<= 8;
		rc->low = (rc->low<<8) & (Top_value-1);
		rc->bytecount++;
	}
}


/* Encode a symbol using frequencies                         */
/* rc is the range coder to be used                          */
/* sy_f is the interval length (frequency of the symbol)     */
/* lt_f is the lower end (frequency sum of < symbols)        */
/* tot_f is the total interval length (total frequency sum)  */
/* or (faster): tot_f = (code_value)1<<shift                             */
static void encode_freq( rangecoder *rc, freq sy_f, freq lt_f, freq tot_f )
{	
	code_value r, tmp;
	enc_normalize( rc );
	r = rc->range / tot_f;
	tmp = r * lt_f;
	rc->low += tmp;
	if (lt_f+sy_f < tot_f)
		rc->range = r * sy_f;
	else
		rc->range -= tmp;
#ifdef DO_CHECKS
	if(!rc->range)
	    fprintf(stderr,"ooops, zero range\n");
#endif
}

static void encode_shift( rangecoder *rc, freq sy_f, freq lt_f, freq shift )
{
	code_value r, tmp;
	enc_normalize( rc );
	r = rc->range >> shift;
	tmp = r * lt_f;
	rc->low += tmp;
	if ((lt_f+sy_f) >> shift)
		rc->range -= tmp;
	else  
		rc->range = r * sy_f;
#ifdef DO_CHECKS	
	if(!rc->range)
		fprintf(stderr,"Oops, zero range\n");
#endif
}


static uint32_t done_encoding( rangecoder *rc )
{   
	size_t tmp;
	enc_normalize(rc);     /* now we have a normalized state */
	rc->bytecount += 5;
	if ((rc->low & (Bottom_value-1)) < ((rc->bytecount&0xffffffL)>>1))
		tmp = rc->low >> SHIFT_BITS;
	else
		tmp = (rc->low >> SHIFT_BITS) + 1;
	if (tmp > 0xff) /* we have a carry */
	{   
		outbyte(rc,rc->buffer+1);
		for(; rc->help; rc->help--)
			outbyte(rc,0);
	} else  /* no carry */
	{   
		outbyte(rc,rc->buffer);
		for(; rc->help; rc->help--)
			outbyte(rc, 0xff);
	}
	outbyte(rc, tmp & 0xff);
    	outbyte(rc, (rc->bytecount>>16) & 0xff);
    	outbyte(rc, (rc->bytecount>>8) & 0xff);
    	outbyte(rc, rc->bytecount & 0xff);
	return rc->bytecount;
}

#define encode_short(ac,s) encode_shift(ac,(freq)1,(freq)(s),(freq)16)

#endif
