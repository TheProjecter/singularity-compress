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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef WIN32
#include <netinet/in.h>
#else
#include <WinSock2.h>
#endif

#include <stdarg.h>
#include "lz_coder.h"

/* ntohl uses bswap_32, as appropriate for current endianness,
 * also bswap_32 on libc/gcc uses asm instructions to swap bytes with 1 instruction */
#define CHAR4_TO_UINT32(data, i) ntohl((*(const uint32_t*)(&data[i])))

/* Use JLAP_INVALID to mark pointers to "not JudyL arrays", in this case pointers to Judy1 array 
 * JLAP_INVALID is defined in Judy.h, and is currently 0x1. Since malloc returns a pointer at least 4-byte
 * aligned, marking it with 0x1, allows us to differentiate Judy1, and JudyL pointers */

#define J1P_PUT( PJ1Array )  ( (Pvoid_t) ( (Word_t)(PJ1Array) | JLAP_INVALID) )
#define J1P_GET( PJ1Array )  ( (Pvoid_t) ( (Word_t)(PJ1Array) & ~JLAP_INVALID) )
#define IS_J1P(  PJ1Array )  ( (Word_t)(PJ1Array) & JLAP_INVALID )


/* offset -> WRAP(offset+buffer_len_half) : new buffer
 * WRAP(offset+buffer_len/2-1) -> offset-1: old buffer (search buffer)
 */

#define WRAP_BUFFER_INDEX(lz_buff, index)  ((index) & (lz_buff->buffer_len_mask))

/* error values */
#define EMEM -2



/* debug logging */
/*#define LOG_DEBUG 1*/


#if !defined(NDEBUG) && defined(LOG_DEBUG)
static void log_debug(const size_t line, const char* fmt,...)
{
	va_list ap;
	va_start(ap,fmt);
	fprintf(stderr,"DEBUG %s: %ld ",__FILE__,line);
	vfprintf(stderr,fmt,ap);
	va_end(ap);
}

#else
static inline void log_debug(const size_t line, const char* fmt,...) {}
#endif

/* ************ */
int setup_lz_buffer(struct lz_buffer* lz_buffer,const size_t buffer_len_power)
{
	lz_buffer->buffer_len_power = buffer_len_power;
	lz_buffer->buffer_len       = 1 << buffer_len_power;
	lz_buffer->buffer_len_mask  = lz_buffer->buffer_len - 1;
	lz_buffer->offset           = 0;
	lz_buffer->jarray           = (Pvoid_t) NULL;
	lz_buffer->buffer = calloc(lz_buffer->buffer_len+4,1);
	if(!lz_buffer->buffer) {
		log_debug(__LINE__,"Out of memory while trying to allocate %ld bytes",lz_buffer->buffer_len);
		return EMEM;
	}
	return 0;
}


static void judy_free_tree(Pvoid_t jarray)
{
	Word_t Index = 0;

	if(IS_J1P(jarray)) {
		int rc;
		Pvoid_t judy1_node = J1P_GET(jarray);

		log_debug(__LINE__,"Retrieved original Judy1 array pointer: %p from %p, freeing it.\n",judy1_node, jarray);

		J1FA(rc, judy1_node);
	}
	else {
		Word_t *PValue;

		log_debug(__LINE__,"Retrieving first entry in JudyL array: %p. ",jarray);

		JLF(PValue, jarray, Index);

		log_debug(__LINE__,"Retrieved first entry: index: %lx; value: %p -> %p\n", Index, PValue, *PValue);

		while(PValue != NULL) {
			log_debug(__LINE__,"At entry: index: %lx, value: %p -> %p\n", Index, PValue, *PValue);

			judy_free_tree((Pvoid_t)*PValue);
			JLN(PValue,jarray, Index);
		}
		{
			int rc;
			log_debug(__LINE__,"Freeing JudyL array: %p\n",jarray);
			JLFA(rc,jarray);
		}
	}
}

void cleanup_lz_buffer(struct lz_buffer* lz_buffer)
{
	if(lz_buffer->buffer) {
		free(lz_buffer->buffer);
		lz_buffer->buffer = NULL;
	}
	if(lz_buffer->jarray)
		judy_free_tree(lz_buffer->jarray);
}

static Pvoid_t create_judy_tree(const struct lz_buffer* lz_buff,const size_t offset, const ssize_t length,const size_t position) 
{
	Pvoid_t  PJLArray = (Pvoid_t) NULL;
	Pvoid_t* const first_node = &PJLArray;
	Pvoid_t* node = first_node;
	ssize_t i;

	for(i=0; i < length-4 ; i += 4) {
		Pvoid_t* next_node;
		const uint32_t val = CHAR4_TO_UINT32(lz_buff->buffer, WRAP_BUFFER_INDEX(lz_buff, offset+i) );

		log_debug(__LINE__,"create_judy_tree: Inserting %x into JudyL array: %p -> %p\n",val, node, *node);

		JLI(next_node, *node, val );

		log_debug(__LINE__,"create_judy_tree: Inserted into %p->%p, value: %p -> %p\n", node, *node, next_node, *next_node);

		if (next_node == PJERR) 
			return PJERR;
	        node = (Pvoid_t*) next_node;
	}
	{
		int rc;

		log_debug(__LINE__,"create_judy_tree: Inserting into judy1 array %p->%p, value:%lx\n",node,*node,position);

		J1S(rc, *node, position );

		log_debug(__LINE__,"create_judy_tree: Inserted into %p->%p\n",node,*node);

		if(rc == JERR)
			return PJERR;

		*node = J1P_PUT(*node);
		if(i==0)
			return *node;
	}
	return *first_node;
}

/* length must be multiple of 4 */
static int judy_insert_bytearray(const struct lz_buffer* lz_buff,const size_t offset, const size_t length,Pvoid_t* node,size_t position)
{
	size_t i;


	memcpy(&lz_buff->buffer[lz_buff->buffer_len],&lz_buff->buffer[0],4);

	for(i=0;i < length && !IS_J1P(*node); i += 4) {
		Pvoid_t* next;
		const uint32_t val = CHAR4_TO_UINT32(lz_buff->buffer, WRAP_BUFFER_INDEX(lz_buff, offset + i) );

		log_debug(__LINE__,"judy_insert_bytearray: Querying %x, in JudyL array %p->%p.  ",val,node,*node);

		JLG(next,*node, val);

		log_debug(__LINE__,"Query result: %p->%p\n", next, next==NULL ? NULL : *next);

		if(next == NULL) {
			log_debug(__LINE__,"judy_insert_bytearray: Inserting %x, into %p->%p.\n",val,node,*node);

			JLI(next, *node,val);

			log_debug(__LINE__,"judy_insert_bytearray: Inserted into %p->%p, value: %p->%p\n",node,*node,next,*next);

			*next = create_judy_tree(lz_buff, offset+i+4, length-i-4, position);

			log_debug(__LINE__,"judy_insert_bytearray: Stored into :%p->%p;value: %p->%p\n",node,*node,next,*next);

			if(*next == PJERR)
				return JERR;
			else
				return 0;
		}		
		node = next;
	}

	if(IS_J1P(*node)) {
		int rc;
		Pvoid_t judy1_node  = J1P_GET(*node);

		log_debug(__LINE__,"Retrieved Judy1 pointer %p from %p->%p. Setting value: %lx\n",judy1_node,node,*node, position);

		J1S(rc,judy1_node, position );

		log_debug(__LINE__,"Storing Judy1 array into %p->%p\n",node,*node);

		*node = J1P_PUT(judy1_node);
		if(rc == JERR)
			return JERR;
	}
	return 0;
}

int lzbuff_insert(struct lz_buffer* lz_buff, const char c)
{
	const int rc = judy_insert_bytearray(lz_buff, lz_buff->offset, lz_buff->buffer_len/2, &lz_buff->jarray, lz_buff->offset);
	/* TODO: also remove old entries */
	lz_buff->offset = WRAP_BUFFER_INDEX(lz_buff, lz_buff->offset + 1 );
	/*lz_buff->buffer[ lz_buff->offset ] = c;*/
	return rc;
}

static int get_longest_match(const struct lz_buffer * lz_buff, const uint32_t prev,const uint32_t next,size_t pos,ssize_t* distance, ssize_t* length)
{
	size_t i;
	const size_t buffer_half_len = lz_buff->buffer_len/2;
	const ssize_t data_search_idx = lz_buff->offset - buffer_half_len/2 - 1;
	ssize_t data_buff_idx   = prev; 

	size_t match_len = pos;

	for(i = pos; i < buffer_half_len; i++) {
		if(lz_buff->buffer[WRAP_BUFFER_INDEX(lz_buff, data_search_idx + i)] != lz_buff->buffer[WRAP_BUFFER_INDEX(lz_buff, data_buff_idx + i)]) {
			match_len = i - 1;
			break;
		}
	}

	if(i == buffer_half_len) {
		/* entire buffer matches */
		*length = buffer_half_len;
		*distance = WRAP_BUFFER_INDEX(lz_buff, lz_buff->offset - prev);
		return 0;
	}

	data_buff_idx = next;

	for(i = pos; i < buffer_half_len; i++) {
		if(lz_buff->buffer[WRAP_BUFFER_INDEX(lz_buff, data_search_idx + i)] != lz_buff->buffer[WRAP_BUFFER_INDEX(lz_buff, data_buff_idx + i)]) {
			if(i - 1 < match_len) {
				/* data_buff_idx = prev has a longer match */
				*distance = WRAP_BUFFER_INDEX(lz_buff, lz_buff->offset - prev);
			}
			else {
				match_len = i-1;
				*distance = WRAP_BUFFER_INDEX(lz_buff, lz_buff->offset - next);
			}
			*length = match_len;
			return 0;
		}
	}

	/* entire buffer matches */
	*length = buffer_half_len;
	*distance = WRAP_BUFFER_INDEX(lz_buff, lz_buff->offset - prev);
	return 0;
}


/* return number of bytes that match,
 * we always assume big endian, since 
 * 0x30313233, comes from the string "0123" */
static inline uint8_t compare_bytes(const uint32_t a,const uint32_t b)
{
	const uint32_t c = a ^ b;/*will be 0 where there is a match*/
	if(c&0xff000000) /* if there are any 1's there, we got a mismatch in the very first byte */
		return 0;
	if(c&0x00ff0000)
		return 1;
	if(c&0x0000ff00)
		return 2;
	if(c&0x000000ff)
		return 3;
	return 4;
}


static inline const Pvoid_t* get_best_match(const uint32_t orig_val, const Word_t prev_val, const Word_t next_val, const Pvoid_t* prev, const Pvoid_t* next,uint8_t* match)
{
	/* determine how many bytes we have matched */
	const uint8_t match_next = compare_bytes(next_val, orig_val);
	const uint8_t match_prev = compare_bytes(prev_val, orig_val);
	if( match_prev < match_next) {
		*match = match_next;
		return next;
	}

	*match = match_prev;
	return prev;
}


/* gets index that is closest */

#define J1_CLOSEST(rc, J1Array, search_index,  prev_index, next_index) \
{\
	(next_index) = (search_index);\
	J1F((rc), (J1Array), (next_index));\
	if(!(rc)) {\
		(prev_index) = (search_index);\
		J1P((rc), (J1Array), (prev_index));\
		if((rc)) {\
			(next_index) = (prev_index);\
			J1N((rc), (J1Array), (next_index));\
		}\
	}\
	else {\
		if((next_index) != (search_index)) {\
			(prev_index) = (next_index);\
			J1P((rc), (J1Array), (prev_index));\
		}\
	}\
}

#define J1_NEIGHBOUR(rc, J1Array, search_index, neighbour_index) \
{\
	(neighbour_index) = (search_index);\
	J1F((rc), (J1Array), (neighbour_index));\
	if(!(rc)) {\
		(neighbour_index) = (search_index);\
		J1P((rc), (J1Array), (neighbour_index));\
	}\
}

static int get_closest(const struct lz_buffer* lz_buff,const Pvoid_t* node,ssize_t* distance)
{
	/* now walk the array of JudyL arrays, till we reach judy1 pointers, and then select the
	 * position stored there that is closest to current offset*/
	while(node && !IS_J1P(*node)) {
		Pvoid_t *next;
		Word_t val = 0;
		JLF(next,*node,val);
		node = next;
	}

	if(node == NULL) {
		fprintf(stderr,"Warning: encountered unexpected empty JudyL array");
		*distance = 0;
		return -2;
	}
	else {
		int rc;
		Pvoid_t judy1_node = J1P_GET(*node);
		Word_t val ;

		J1_NEIGHBOUR(rc, judy1_node, lz_buff->offset, val);

		if(!rc) {
			fprintf(stderr,"Emtpy judy1 array?\n");
			*distance = 0;
			return -1;
		}

		*distance = WRAP_BUFFER_INDEX(lz_buff, lz_buff->offset - val);
		return 0;
	}
}


int lzbuff_search_longest_match(const struct lz_buffer* lz_buff,const size_t offset,const size_t data_len, ssize_t* distance, ssize_t* length)
{
	size_t i;
	/* start searching from root array */
	const Pvoid_t* node = &lz_buff->jarray;

	memcpy(&lz_buff->buffer[lz_buff->buffer_len],&lz_buff->buffer[0],4);
	/* when we reach end-of-buffer during a search, we would need to wrap the search,
	 * but we can only wrap on multiples of 4, so to prevent accessing uninitialized memory,
	 * we copy first 4 bytes, to end of buffer */

	for(i=0;i < data_len && !IS_J1P(*node); i+=4) {
		const Pvoid_t* next;
		const Pvoid_t* prev;

		const uint32_t orig_val = CHAR4_TO_UINT32(lz_buff->buffer, WRAP_BUFFER_INDEX(lz_buff, offset + i) );
		/* indexes in the array are groups of 4 characters converted into uint32 */
		Word_t prev_val;
		Word_t next_val = orig_val;

		/* search the first index >= than the value we search for,
		 * or if it doesn't exist, then the index lower than it
		 * next is the value contained in the JudyL array at index val
		 * It is really a pointer to another JudyL array (if IS_J1P(*next) == 0) ;
		 * Or it is a pointer to a Judy1 array if IS_J1P(*next) == 1
		 */

		JLF( next, *node, next_val);
		if(!next) {
			prev_val = orig_val;
			JLP(prev, *node, prev_val);
			if(prev) {
				/* previous found, search for next neighbour now */
				next_val = prev_val;
				JLN(next, *node, next_val);
				if(!next) {
					/* array has only 1 element */
					next = prev;
					next_val = prev_val;
				}
			}
			else {
				fprintf(stderr,"Empty JudyL array?\n");
				/* no value found in JudyL array, buffer is empty */
				*length = -1;
				*distance = 0;
				return -1;
			}
		}
		else {
			if(next_val != orig_val) {
				prev_val = next_val;
				JLP(prev, *node, prev_val);
				if(!prev) {
					/* array has only 1 element */
					prev = next;
					prev_val = next_val;
				}
			}
		}

		if(next_val != orig_val) {
			uint8_t match;
			/* we encountered a mismatch */
			node = get_best_match(orig_val, prev_val, next_val, prev, next ,&match);
			*length = match+i;
			return get_closest(lz_buff, node, distance);
		}	
		
		node = next;
	}

	if(IS_J1P(*node)) {
		int rc;
		Pvoid_t judy1_node = J1P_GET(*node);
		const uint32_t orig_val = CHAR4_TO_UINT32(lz_buff->buffer, WRAP_BUFFER_INDEX(lz_buff, offset + i) );
		Word_t val = orig_val;
		Word_t val_prev, val_next;

		log_debug(__LINE__,"Retrieved original Judy1 array pointer: %p from %p->%p\n",judy1_node, node,*node);

		J1_CLOSEST(rc, judy1_node, val, val_prev, val_next);

		if(val_next != orig_val) {
			return get_longest_match(lz_buff,val_prev, val_next,i, distance, length);
		}
		else {
			*distance = -data_len;
			*length	 = data_len;
			return 0;
		}
	}
	else {
		return -1;
		/* assertion failure */
	}	
}


static void judy_show_tree(Pvoid_t jarray,int level)
{	
	char* spaces = malloc(level+1);
	Word_t Index = 0;

	memset(spaces,0x20,level);
	spaces[level] = 0;

	if(IS_J1P(jarray)) {
		int rc;
		Pvoid_t judy1_node = J1P_GET(jarray);
		J1F(rc,judy1_node,Index);
		while(rc) {
			fprintf(stderr,":%s%lx\n",spaces,Index);
			J1N(rc,judy1_node,Index);
		}
	}
	else {
		Word_t * PValue;
		log_debug(__LINE__,"querying first:%p\n",jarray);
		JLF(PValue, jarray, Index);
		while(PValue != NULL) {		
			fprintf(stderr,"%s%lx\n",spaces,Index);
			judy_show_tree((Pvoid_t)*PValue, level+1);
			JLN(PValue, jarray, Index);
		}
	}
	free(spaces);
}

void show_lz_buff(const struct lz_buffer* lz_buff)
{
	size_t i;
	for(i=0;i < lz_buff->buffer_len;i++) {
		if(i == lz_buff->offset)
			fprintf(stderr,"|");
		fprintf(stderr,"%02x ",lz_buff->buffer[i]);
	}
	fprintf(stderr,"\n");
}

void show_match(const struct lz_buffer* lz_buff,ssize_t distance,ssize_t length)
{
	size_t start = WRAP_BUFFER_INDEX(lz_buff,   lz_buff->offset - distance);
	
	ssize_t match = -1;
	size_t i;

	for(i=0;i < lz_buff->buffer_len;i++) {
		if(i == start) {
			fprintf(stderr,"<");
			match = 0;
		}
		if(match >= 0)
			match++;
		fprintf(stderr,"%02x ",lz_buff->buffer[i]);
		if(match == length)
			printf(">");
	}
	fprintf(stderr,"\n");
}

/*
int main(int argc,char* argv[])
{
	const unsigned char test[] = "0123456789012345";
	const size_t len = sizeof(test)-1;
	size_t i;

	struct lz_buffer    lz_buff;
	FILE* out = fopen("/tmp/testout","w");

	setup_lz_buffer(&lz_buff,5);


	 this will be replaced by a read() 
	for(i=0;i < len;i++) {
		lz_buff.buffer[lz_buff.offset + i] = test[i];
	}


	for(i=0; i < len;) {
		ssize_t distance;
		ssize_t  length;

		show_lz_buff(&lz_buff);

		lzbuff_search_longest_match(&lz_buff, &lz_buff.buffer[lz_buff.offset], lz_buff.buffer_len/2, &distance, &length);

		show_match(&lz_buff,distance,length);

		lzbuff_insert(&lz_buff, lz_buff.buffer[lz_buff.offset + i]);

		show_lz_buff(&lz_buff);
		judy_show_tree(lz_buff.jarray,0);
		printf("match:%ld,%ld\n",distance,length);
		if(length < 3) {
			length = 1;
		}
		else {
		}
		i += length;
	}
	cleanup_lz_buffer(&lz_buff);

	return 0;
}
*/
/*
int main(int argc,char* argv[])
{
	Pvoid_t jarray = (Pvoid_t) NULL;
	const size_t len = 16*2;
	const size_t maxIdx = 8;
	unsigned char* test = malloc(len+1);

	const unsigned char test2[] = "0123456789012346";
	const size_t len2 = sizeof(test)-1;
	size_t i;
	ssize_t distance=0;
	size_t length=0;

	srand(2);
	for(i=0;i<len/2;i++) {
		test[i] = test2[i%len2];
	}
	test[len]=0;
	for(i=0;i<len/2;i++) {
		judy_insert_bytearray(test+i,maxIdx,&jarray,i);
	}
	judy_insert_bytearray(test,maxIdx,&jarray,0);

	judy_show_tree(jarray,0);
	struct lz_buffer buff;
	lzbuff_search_longest_match(&buff,test2+10,len2-10,&distance,&length);
	printf("%ld:%ld\n",distance,length);
	judy_free_tree(jarray);
	jarray = NULL;
	return 0;
}
*/
