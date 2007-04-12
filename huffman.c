#include <stdlib.h>
#include <string.h>

#include "huffman.h"


const int EMEM = -1;

struct element {
	uint16_t value;
	uint16_t code;/*huffman code*/
	uint8_t codelength;
};

struct output {
	unsigned short int code;
	uint16_t length;
	uint16_t distance;
};


int huffman_init(struct huffman* huffman, FILE* fout, const size_t bufsize)
{
	huffman->fout = fout;
	huffman->buff = malloc(bufsize*sizeof(*huffman->buff));
	if(!huffman->buff)
		return EMEM;
	huffman->buffsize = bufsize;

	huffman->heap = malloc(285*sizeof(*huffman->heap));
	if(!huffman->heap)
		return EMEM;
	memset(huffman->frequency, 0, 285*sizeof(*huffman->frequency));
	return 0;
}

static const uint16_t length_to_code[] = {
	0, 0, 0,
	257, 258, 259,260, 261, 262, 263, 264,
	265, 265,/*11, 12*/
	266, 266,/*13, 14*/
	267, 267,
	268, 268,
	269, 269, 269, 269,
	270, 270, 270, 270,
	271, 271, 271, 271,
	272, 272, 272, 272,
	273, 273, 273, 273, 273, 273, 273, 273,/* 35-42 */
	274, 274, 274, 274, 274, 274, 274, 274,
	275, 275, 275, 275, 275, 275, 275, 275,
	276, 276, 276, 276, 276, 276, 276, 276,
	277, 277, 277, 277, 277, 277, 277, 277,
	278, 278, 278, 278, 278, 278, 278, 278, 278, 278, 278, 278, 278, 278, 278, 278,/*83-98*/
	279, 279, 279, 279, 279, 279, 279, 279, 279, 279, 279, 279, 279, 279, 279, 279,
	280, 280, 280, 280, 280, 280, 280, 280, 280, 280, 280, 280, 280, 280, 280, 280,
	281, 281, 281, 281, 281, 281, 281, 281, 281, 281, 281, 281, 281, 281, 281, 281,281, 281, 281, 281, 281, 281, 281, 281, 281, 281, 281, 281, 281, 281, 281, 281,/*131 - 162 */
	282, 282, 282, 282, 282, 282, 282, 282, 282, 282, 282, 282, 282, 282, 282, 282,282, 282, 282, 282, 282, 282, 282, 282, 282, 282, 282, 282, 282, 282, 282, 282,
	283, 283, 283, 283, 283, 283, 283, 283, 283, 283, 283, 283, 283, 283, 283, 283,283, 283, 283, 283, 283, 283, 283, 283, 283, 283, 283, 283, 283, 283, 283, 283,/*195 - 226 */
	284, 284, 284, 284, 284, 284, 284, 284, 284, 284, 284, 284, 284, 284, 284, 284,284, 284, 284, 284, 284, 284, 284, 284, 284, 284, 284, 284, 284, 284, 284, /* 227 - 257*/
	285 /*258*/
};


typedef struct huffman_node {
	int c;/* -1 means virtual node */
	size_t frequency;
	struct huffman_node*  left;
	struct huffman_node*  right;
};

/* todo: use a heap implementation here, this is just some temporary s*** */

static struct huffman_node* heap_delete_min(struct huffman_node* heap[],size_t* heap_size)
{
	size_t i,j=*heap_size-1;
	const struct huffman_node* min = heap[j];


	for(i=0;i<*heap_size;i++) {
		if(heap[i]->frequency < min->frequency) {
			min = heap[i];
			j=i;
		}
	}
	if(j<*heap_size) {
		memmove(&heap[j],&heap[j+1], (*heap_size-j-1)*sizeof(*heap));
	}
	(*heap_size)--;
	return min;	
}

static void build_heap(struct huffman_node* heap,size_t heap_size)
{
}

static void heap_insert(struct huffman_node** heap,struct huffman_node* node,size_t* heap_size)
{
	/*TODO;;;;*/
	heap[*heap_size++] = node;
}


static int build_huffman_tree(struct huffman* huffman)
{
	struct huffman_node main_tree[2*285+1];
	struct huffman_node* heap[2*285+1];
	struct huffman_node* stack[285];
	struct huffman_node* root = NULL;
	size_t frequency[285];
	size_t last_node_cnt = 286;
	size_t heap_size = 0;
	size_t i;
	size_t stackp = 0;
	size_t level = 0;
	size_t code = 0;


	memset(frequency,0, 285*sizeof(frequency[0]));
	for(i=0; i<huffman->buffsize; i++) {
		huffman->frequency[huffman->buff[i].code]++;
	}

	for(i=0; i<2*285+1;i++) {
		heap[i] = calloc(1,sizeof(*heap[0]));
		heap[i]->c = i;
		if(i<285)
			heap[i]->frequency = huffman->frequency[i];
		heap[i]->left = NULL;
		heap[i]->right = NULL;
	}

	build_heap(heap,285);
	heap_size = 285;
	while(heap_size) {
		struct huffman_node* n1 = heap_delete_min(heap,&heap_size);
		struct huffman_node* n2 = heap_size ? heap_delete_min(heap,&heap_size) : NULL;
		if(n1 && n2) {
			struct huffman_node* merged = calloc(1,sizeof(*merged));
			heap[heap_size++] = merged;
			merged->frequency = n1->frequency + n2->frequency;
			merged->left = n1;
			merged->right = n2;
			merged->c = -1;
			root = merged;
		}
	}

	process(huffman,root,0);
	return 0;
}

void process(struct huffman* huffman,struct huffman_node* root,int code,int level) 
{
	if (root) {
		if(root->c != -1) {
			huffman->heap[root->c].value = root->c;
			huffman->heap[root->c].codelength = level + 1;
			huffman->heap[root->c].code = code;
		}
		if(root->left)
			process(huffman, root->left,(code<<1)|0, level+1);
		if(root->right)
			process(huffman, root->right,(code<<1)|1, level+1);
	}

	/*TODO build heap*/
	return 0;
}

static void output_bit(struct huffman* huffman,const uint8_t bit)
{
	printf("%d");
}

static void huffman_encode(struct huffman* huffman, const struct output* out)
{
	size_t i;
	for(i=0;i<huffman->buffsize;i++) {
		struct element* e = &huffman->heap[huffman->buff[i].code];
		int j;
		for(j=0;j < e->codelength;j++)
			output_bit(huffman,(e->code>>j)&1);
	}
}


static int output_huffman_tree(struct huffman* huffman)
{
#define MAX_BITS 16
	uint8_t bl_count[16];
	uint8_t next_code[MAX_BITS];
	uint16_t i, n;
	uint8_t code, bits;
/* assign codes */
	memset(bl_count, 0, 16);
	for(i=0; i<285; i++)
		bl_count[ huffman->heap[i].codelength ]++;
	code = 0;
	bl_count[0] = 0;
	for(bits = 1; bits <= MAX_BITS; bits++) {
		code = (code + bl_count[bits-1]) << 1;
		next_code[bits] = code;
	}
	for(n = 0; n < 285; n++) {
		size_t len = huffman->heap[i].codelength;
		if(len) {
			huffman->heap[i].code = next_code[len];
			next_code[len]++;
		}
	}
	output_bit(huffman, 1);
	output_bit(huffman, 0);
	/*TODO:now encode the code lengths using another huffman code*/
	/*TODO: now output the other huffman code tree */
	/*TODO: output code length tree using previous huffman code*/
	/* now encode the buffer, and output */
	for(i=0; i < huffman->buffsize; i++)
		huffman_encode(huffman, &huffman->buff[i]);
	return 0;
}

static int reset_tree(struct huffman* huffman)
{
	/* TODO */
	return 0;
}

static int output_data(struct huffman* huffman)
{
	/* TODO */
	return 0;
}


static int huffman_output_tree(struct huffman* huffman) 
{
	int rc;
	return 0;
	if(( rc = build_huffman_tree(huffman) ))
		return rc;
	if(( rc = output_huffman_tree(huffman) ))
		return rc;
	if(( rc = output_data(huffman) ))
		return rc;
	if(( rc = reset_tree(huffman) ))
		return rc;
	return 0;
}

static inline int huffman_check_blockend(struct huffman* huffman)
{
	if(huffman->buffcnt == huffman->buffsize) {
		int rc;
		if(( rc = huffman_output_tree(huffman) ))
			return rc;
		huffman->buffcnt = 0;
	}
	return 0;
}

int huffman_output_char(struct huffman* huffman, const unsigned char c)
{
	struct output* out = &huffman->buff[ huffman->buffcnt++ ];
	out->code = c;
	out->length = 0;
	huffman->frequency[c]++;

	return huffman_check_blockend(huffman);
}

int huffman_output_length_distance(struct huffman* huffman, const uint16_t length, const uint16_t distance)
{
	const unsigned short code = length_to_code[length];
	struct output* out = &huffman->buff[ huffman->buffcnt++ ];
	out->length = length;
	out->distance = distance;
	out->code = code;
	huffman->frequency[code]++;

	return huffman_check_blockend(huffman);
}
/*
int main()
{
	const char text[]="THREE SWISS WITCHES WATCH THREE SWISS SWATCH WATCHES";
	size_t i;
	struct huffman huffman;

	huffman_init(&huffman,NULL,sizeof(text));
	for(i=0;i < sizeof(text);i++)
		huffman_output_char(&huffman, text[i]);
	build_huffman_tree(&huffman);
	huffman_encode(&huffman,NULL);
}
*/
