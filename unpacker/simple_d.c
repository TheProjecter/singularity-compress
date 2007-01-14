/*
 * Singularity-compress: arithmetic decoder, and lz77 decoder
 *
 * Based on Michael Schindler's rangecoder, which is:
 * (c) Michael Schindler 1999
 * http://www.compressconsult.com/
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "../common/code_tables.h"

#include "range_decod.h"

#define EXTRA_SYMBOLS 28
#define SYMBOLS (256+EXTRA_SYMBOLS)



static void readcounts(rangecoder *rc, freq *counters)
{  
	size_t i;
	for (i=0; i<SYMBOLS; i++)
		counters[i] = decode_short(rc);
}

struct buffer {
	unsigned char* data;
	size_t         len;
	ssize_t        len_mask;
	size_t         len_power;
	size_t         offset;
};

#define MIN(a,b) ((a)<(b) ? (a) : (b))

static ssize_t copy_back_bytes(struct buffer* buff,ssize_t dest,ssize_t backbytes,size_t backsize)
{
	size_t from = (dest - backbytes) & buff->len_mask;

	size_t dest_maxcopy = MIN(buff->len - dest, backsize);
	size_t src_maxcopy =  MIN(buff->len - from, backsize);

	if(src_maxcopy < dest_maxcopy) {
		memcpy(&buff->data[dest], &buff->data[from], src_maxcopy);

		dest += src_maxcopy;
		backsize -= src_maxcopy;
		from  = (from + src_maxcopy)&buff->len_mask; 
		dest_maxcopy -= src_maxcopy;

		if(dest_maxcopy) {
			memcpy(&buff->data[dest], &buff->data[from], dest_maxcopy);

			from += dest_maxcopy;
			backsize -= dest_maxcopy;
			dest  = (dest + dest_maxcopy)&buff->len_mask;
		        if(!dest)
				fwrite(buff->data,1,buff->len,stdout);

			memcpy(&buff->data[dest], &buff->data[from], backsize);
			dest += backsize;
		}
	}
	else {
		memcpy(&buff->data[dest], &buff->data[from], dest_maxcopy);

		from += dest_maxcopy;
		backsize -= dest_maxcopy;
		dest = (dest + dest_maxcopy)&buff->len_mask;
		src_maxcopy -= dest_maxcopy;

		if(!dest)
			fwrite(buff->data,1,buff->len,stdout);
		if(src_maxcopy) {
			memcpy(&buff->data[dest], &buff->data[from], src_maxcopy);

			dest += src_maxcopy;
			backsize -= src_maxcopy;
			from = (from + src_maxcopy) &buff->len_mask;

			memcpy(&buff->data[dest],&buff->data[from], backsize);
			dest += backsize;
		}
	}
	return dest;
}

static freq get_symbol(const freq* counts, const freq cf)
{
	size_t sym_lo = 0;
	size_t sym_hi = SYMBOLS;

	do{
		size_t middle = (sym_lo+sym_hi+1)/2;
		/* we need +1 there, because we want last symbol that has freq <= cf */
		if(counts[middle] > cf) {
			sym_hi = middle - 1;
		}
		else if(counts[middle] < cf) {
			sym_lo = middle + 1;
		}
		else {
			sym_lo = middle;
		}

	} while(sym_lo < sym_hi);
	if(counts[sym_lo] > cf)
		sym_lo--;
	return sym_lo;
}



int unpack(FILE* out,FILE* in) 
{
	freq cf;
	rangecoder rc;
	struct buffer buffer;

	buffer.offset = 0;
	buffer.len_power = 6;
	buffer.len = 1<<buffer.len_power;
	buffer.len_mask = buffer.len - 1;
	buffer.data = malloc(buffer.len);
	rc.in = in;

	if(!buffer.data)
		abort();

	if (start_decoding(&rc) != 0) {   
		fprintf(stderr,"could not suceessfully open input data\n");
		exit(1);
	}

	while ( (cf = decode_culfreq(&rc,2)) ) {   
		freq counts[SYMBOLS+1], i, blocksize;

		decode_update(&rc,1,1,2);
		readcounts(&rc,counts);

		/*
		fprintf(stderr,"Retrieved counters:\n");
		for(i=0;i<SYMBOLS;i++) {
			fprintf(stderr,"%d:%ld\n",i,counts[i]);
		}
		fprintf(stderr,"\n");
		*/

		/* figure out blocksize by summing counts; also use counta as in encoder */
		blocksize = 0;
		for (i=0; i<SYMBOLS; i++) {  
			freq tmp = counts[i];
			counts[i] = blocksize;
			blocksize += tmp;
		}
		counts[SYMBOLS] = blocksize;

		/*
		fprintf(stderr,"Retrieved counters:\n");
		for(i=0;i<SYMBOLS;i++) {
			fprintf(stderr,"C%d:%ld\n",i,counts[i]);
		}
		fprintf(stderr,"\n");
		*/

		for (i=0; i<blocksize; i++) {   
			freq symbol;

			cf = decode_culfreq(&rc,blocksize);

			symbol =  get_symbol(counts, cf);

			/*fprintf(stderr,"Decoding:%d(%c),%ld,%ld\n",symbol,symbol,counts[symbol+1]-counts[symbol],counts[symbol]);*/

			decode_update(&rc, counts[symbol+1]-counts[symbol],counts[symbol],blocksize);
			if(symbol > 0xff) {
				const uint8_t extra_bits = code_to_length[symbol-0x100].extra_bits;
				const size_t  extra_data = decode_culshift(&rc, extra_bits);

				const size_t  length = code_to_length[symbol-0x100].start + extra_data;
				size_t distance;
				size_t distance_hi;

				decode_update_shift(&rc, 1, extra_data, extra_bits);

				distance = decode_culshift(&rc,8);
				decode_update_shift(&rc, 1, distance, 8);

				distance_hi = decode_culshift(&rc,8);
				decode_update_shift(&rc, 1, distance_hi, 8);

				distance |= distance_hi<<8;

				/* fprintf(stderr,"Retrieved length,distance:%ld,%ld\n",length,distance);*/
				buffer.offset = copy_back_bytes(&buffer,buffer.offset,distance,length);
			}
			else {
				buffer.data[buffer.offset++] = symbol;
				if(buffer.offset >= buffer.len) {
					buffer.offset = 0;
					fwrite(buffer.data,1,buffer.len,out);
				}
			}
		}
	}
	fwrite(buffer.data,1,buffer.offset,out);
	done_decoding(&rc);

	fclose(out);
	return 0;
}
