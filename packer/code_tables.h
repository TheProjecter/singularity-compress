#ifndef _CODES_TABLE_H
#define _CODES_TABLE_H

#include "stdlib.h"

static const struct {
	ssize_t start;
	uint8_t  extra_bits;
} code_to_length[] = {
	{3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0}, {8, 0}, {9, 0}, {10, 0}, 
	{11, 1}, {13, 1}, {15, 1}, {17, 1}, 
	{19, 2}, {23, 2}, {27, 2}, {31, 2}, 
	{35, 3}, {43, 3}, {51, 3}, {59, 3}, 
	{67, 4}, {83, 4}, {99, 4}, {115, 4}, 
	{131, 5}, {163, 5}, {195, 5}, {227, 5}, 
	{259, 0xff}
};

static const size_t code_to_length_size = sizeof(code_to_length)/sizeof(code_to_length[0]);
#endif
