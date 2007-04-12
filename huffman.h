#include <stdio.h>
#include <stdint.h>

struct huffman {
	struct element* heap;
	struct output* buff;
	size_t frequency[285];
	size_t buffcnt;
	size_t buffsize;
	FILE* fout;
};
int huffman_init(struct huffman* huffman,FILE* fout, const size_t bufsize);
int huffman_output_char(struct huffman* huffman,const unsigned char c);
int huffman_output_length_distance(struct huffman* huffman,const uint16_t length, const uint16_t distance);
int huffman_done(struct huffman* huffman);
