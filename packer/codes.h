/*
 * Singularity-compress: LZ77 encoder: length code tables
 * Copyright (C) 2006-2007  Torok Edwin (edwintorok@gmail.com)
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
#ifndef _CODES_H
#define _CODES_H

#include "stdlib.h"

static inline uint16_t length_to_code(ssize_t length,uint8_t* extra_bits,size_t* extra_data)
{
	size_t low = 0;
	size_t hi  = code_to_length_size-1;

	while(low < hi) {
		const size_t middle = (hi+low)/2;
		if(code_to_length[middle].start > length) {
			hi = middle-1;
		}
		else if(code_to_length[middle].start < length) {
			low = middle+1;
		}
		else {
			*extra_data = length - code_to_length[middle].start;
			*extra_bits = code_to_length[middle].extra_bits;

			return middle; 
		}
	}

	if(code_to_length[low].start > length)
		low--;

	*extra_data = length - code_to_length[low].start;
	*extra_bits = code_to_length[low].extra_bits;

	return low;
}

#endif
