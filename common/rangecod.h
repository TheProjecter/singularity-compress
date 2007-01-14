/*
   rangecod.h     headerfile for range encoding

   (c) Michael Schindler
   1997, 1998, 1999, 2000
   http://www.compressconsult.com/
   michael@compressconsult.com

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; version 2
   of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
 
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef _RANGECOD_H
#define _RANGECOD_H

typedef uint32_t code_value;       /* Type of an rangecode value, must accomodate 32 bits */
/* it is highly recommended that the total frequency count is less  */
/* than 1 << 19 to minimize rounding effects.                       */
/* the total frequency count MUST be less than 1<<23                */

typedef uint32_t freq; 


typedef struct {
    uint32_t low,        /* low end of interval */
	     range,         /* length of interval */
	     help;          /* bytes_to_follow resp. intermediate value */
    unsigned char buffer;/* buffer for input/output */
    /* the following is used only when encoding */
    uint32_t bytecount;     /* counter for outputed bytes  */
    /* insert fields you need for input/output below this line! */
} rangecoder;

#define CODE_BITS 32
#define Top_value ((code_value)1 << (CODE_BITS-1))

#define outbyte(cod,x) putchar(x)
#define inbyte(cod)    getchar()

#define SHIFT_BITS (CODE_BITS - 9)
#define EXTRA_BITS ((CODE_BITS-2) % 8 + 1)
#define Bottom_value (Top_value >> 8)

#endif
