/*
 * Singularity-compress: arithmetic encoder
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

#ifndef unix
#include <fcntl.h>
#endif

#include <ctype.h>
#include <stdint.h>
#include <string.h>

#include "range_encoder.h"

#include "lz_coder.h"
#include "../common/code_tables.h"
#include "codes.h"
#include "../common/model.h"

/*
 * Symbols: 
 * 0-255: literal
 * 256-288: match length: 3-
 *
 */


struct lz_extra_data {
	uint8_t   extra_bits;
	size_t    extra_data;
	uint16_t  distance;
};


static void usage(void)
{   
	fprintf(stderr,"simple_c [inputfile [outputfile]]\n");
	exit(1);
}

static int errcnt=0;
static int check_lz(const struct lz_buffer* lz_buff,ssize_t length,ssize_t distance)
{
	ssize_t i;
	for(i=0;i<length;i++)
		if(lz_buff->buffer[ WRAP_BUFFER_INDEX(lz_buff, lz_buff->offset - distance + i) ] != 
				lz_buff->buffer[ WRAP_BUFFER_INDEX(lz_buff, lz_buff->offset + i) ] ) {
			errcnt++;
			return 0;
		}
	return 1;
}


static size_t lz_encode_buffer(struct lz_buffer* lz_buff,struct lz_extra_data* extra_datas,uint16_t* lz_out_buffer, size_t* len)
{
	ssize_t distance;
	ssize_t length;
	size_t i;
	size_t extra_datas_cnt = 0;

	
	for(i=0;i < *len; i++) {
		const char c = lz_buff->buffer[WRAP_BUFFER_INDEX(lz_buff, lz_buff->offset)];
		lzbuff_search_longest_match(lz_buff, lz_buff->offset, lz_buff->buffer_len/2, &distance, &length);

		/*show_match(lz_buff,distance,length);*/


		/*show_lz_buff(lz_buff);*/


		if(length >= code_to_length[0].start && distance >0 && distance < lz_buff->buffer_len/2 && check_lz(lz_buff,length,distance) ) {
			struct lz_extra_data* extra_data = &extra_datas[extra_datas_cnt++];


			extra_data->distance = distance;

			lz_out_buffer[i] = 0x100 + length_to_code(length, &extra_data->extra_bits, &extra_data->extra_data);
			lz_buff->offset = WRAP_BUFFER_INDEX(lz_buff, lz_buff->offset + length);
			*len -= length-1;
		}
		else {
			lzbuff_insert(lz_buff, c);
			lz_out_buffer[i] = c;
		}
	}

	for(i=0;i<lz_buff->buffer_len/2;i++)
		lz_remove(lz_buff, lz_buff->offset+lz_buff->buffer_len/2+i);
	return extra_datas_cnt;
}

/* count number of occurances of each byte */
static void countblock(uint16_t *buffer, freq length, freq *counters)
{   
    size_t i;
    /* zero counters */
    memset(counters, 0 ,sizeof(counters[0])*(SYMBOLS+1));
    /* then count the number of occurances of each byte */
    for (i=0; i<length; i++)
       counters[buffer[i]]++;
}
#define MIN(a,b) ((a)<(b)?(a):(b))
int main( int argc, char *argv[] )
{   
	size_t blocksize;
	rangecoder rc;
	unsigned char buffer[BLOCKSIZE];
	uint16_t lz_out_buffer[BLOCKSIZE];
	struct lz_extra_data extra_datas[BLOCKSIZE];
	size_t extra_data_cnt = 0;
	size_t j;
	struct lz_buffer lz_buff;
	struct ari_model model;

	if ((argc > 3) || ((argc>1) && (argv[1][0]=='-')))
		usage();

	if ( argc <= 1 )
		fprintf( stderr, "stdin" );
	else {
		freopen( argv[1], "rb", stdin );
		fprintf( stderr, "%s", argv[1] );
	}
	if ( argc <= 2 )
		fprintf( stderr, " to stdout\n" );
	else {
		freopen( argv[2], "wb", stdout );
		fprintf( stderr, " to %s\n", argv[2] );
	}

#ifndef unix
	setmode( fileno( stdin ), O_BINARY );
	setmode( fileno( stdout ), O_BINARY );
#endif

	/* initialize the range coder, first byte 0, no header */
	start_encoding(&rc,0,0);
	setup_lz_buffer(&lz_buff, 1+BLOCKSIZE_POWER);
	model_setup(&model);

	while (1)
	{   
		freq i;
		size_t extra_datas_cnt;
		
		blocksize = MIN( BLOCKSIZE, lz_buff.buffer_len - lz_buff.offset);
		/* get the statistics */
		blocksize = fread(lz_buff.buffer+lz_buff.offset,1,(size_t)blocksize,stdin);

		/* terminate if no more data */
		if (blocksize==0) break;

		encode_freq(&rc,1,1,2); /* a stupid way to code a bit */
		/* blocksize, can be max 2^22, since we would need to restart coder anyway on <2^23*/
		extra_datas_cnt = lz_encode_buffer(&lz_buff,extra_datas,lz_out_buffer,&blocksize);

		encode_short(&rc, blocksize&0xffff);
		encode_short(&rc, blocksize>>16);
		/*countblock(lz_out_buffer,blocksize,counts);*/

		/* write the statistics. */
		/* Cant use putchar or other since we are after start of the rangecoder */
		/* as you can see the rangecoder doesn't care where probabilities come */
		/* from, it uses a flat distribution of 0..0xffff in encode_short. */

		/*fprintf(stderr,"Counters:\n");
		  for(i=0;i<SYMBOLS;i++) {
		  fprintf(stderr,"%d:%ld\n",i,counts[i]);
		  }
		  fprintf(stderr,"\n");*/

		/*for(i=0; i < SYMBOLS; i++)
			encode_short(&rc,counts[i]);*/

		/* store in counters[i] the number of all bytes < i, so sum up */
		/*counts[SYMBOLS] = blocksize;
		for (i=SYMBOLS; i; i--)
			counts[i-1] = counts[i]-counts[i-1];
		*/



		/*
		   fprintf(stderr,"Counters:\n");
		   for(i=0;i<SYMBOLS;i++) {
		   fprintf(stderr,"C%d:%ld\n",i,counts[i]);
		   }
		   fprintf(stderr,"\n");
		   */

		/* output the encoded symbols */
		for(i=0,j=0; i<blocksize; i++) {
			freq cur_freq, cum_freq, total_freq;
			const uint16_t ch = lz_out_buffer[i];
			/*fprintf(stderr,"Encoding:%d,(%c),%ld,%ld\n",ch,ch,counts[ch+1]-counts[ch],counts[ch]);*/
			model_get_freq(&model,ch,&cur_freq,&cum_freq,&total_freq);
			encode_freq(&rc,cur_freq,cum_freq,total_freq);
			model_update_freq(&model,ch);
			if(ch > 0xff) {
				/* output length,distance */
				const struct lz_extra_data* extra_data = &extra_datas[j++];
				if(extra_data->extra_bits) {
					encode_shift(&rc, (freq)1, (freq)extra_data->extra_data, extra_data->extra_bits);
				}
		    		encode_shift(&rc,(freq)1,(freq)(extra_data->distance&0xff), 8);
				encode_shift(&rc,(freq)1,(freq)(extra_data->distance>>8),8);
			}
		}
		/*fprintf(stderr,"%d\n",model.counts[SYMBOLS]);*/
	}

	/* flag absence of next block by a bit */
	encode_freq(&rc,1,0,2);

	/* close the encoder */
	done_encoding(&rc);
	model_done(&model);
	cleanup_lz_buffer(&lz_buff);

	fprintf(stderr,"Missed: %ld LZ77 encoding opportunities\n",errcnt);
	return 0;
}
