/*
 * Singularity-compress: LZ77 encoder
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
#ifndef _LZ_CODER_H
#define _LZ_CODER_H

#include <Judy.h>

struct lz_buffer {
	unsigned char*   buffer;
	size_t  buffer_len;
	size_t  buffer_len_mask; 
	size_t  buffer_len_power;/* buffer_len = 2^buffer_len_base2 */
	ssize_t  offset;
	Pvoid_t jarray;
};
int setup_lz_buffer(struct lz_buffer* lz_buffer,const size_t buffer_len_power);
void cleanup_lz_buffer(struct lz_buffer* lz_buffer);

int lzbuff_insert(struct lz_buffer* lz_buff, const char c);
int lzbuff_search_longest_match(const struct lz_buffer* lz_buff,const size_t offset,const size_t data_len, ssize_t* distance, ssize_t* length);

/* offset -> WRAP(offset+buffer_len_half) : new buffer
 * WRAP(offset+buffer_len/2-1) -> offset-1: old buffer (search buffer)
 */
#define WRAP_BUFFER_INDEX(lz_buff, index)  ((index) & (lz_buff->buffer_len_mask))

/* debugging functions - to be removed in a release */
void show_lz_buff(const struct lz_buffer* lz_buff);
void show_match(const struct lz_buffer* lz_buff,ssize_t distance,ssize_t length);
#endif
