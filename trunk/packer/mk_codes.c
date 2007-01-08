/*
 * Singularity-compress: LZ77 encoder: length code tables
 * Copyright (C) 2006-2007  Torok Edwin (edwintorok@gmail.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

static void make_length_code_tables(uint8_t extra_symbols,const size_t start)
{
	size_t code;
	unsigned short extra_bits = 0;
	unsigned short extra_bits_count = 0;
	size_t   i;

	printf("#include \"stdlib.h\"\n\n");
	printf("static const struct {\n");
	printf("\tsize_t start;\n");
	printf("\tuint8_t  extra_bits;\n");
	printf("} code_to_length[] = {\n");

	printf("\t");
	for(code = start; code < start + 8 && code < start + extra_symbols; code++)
		printf("{%lu, 0}, ",code); 

	extra_bits = 1;
	printf("\n\t");
	for(i = 8; i < extra_symbols; i++ ) {
		printf("{%lu, %u}, ", code, extra_bits);

		code +=  (1 << extra_bits);
		extra_bits_count++;

		if ( extra_bits_count == 4) {
			printf("\n\t");
			extra_bits++;
			extra_bits_count=0;
		}
	}
	printf("{%ld, 0xff}\n",code);

	printf("};\n");
}


int main(int argc,char* argv[])
{
	printf("#ifndef _CODES_TABLE_H\n");
	printf("#define _CODES_TABLE_H\n\n");
	make_length_code_tables(28,3);
	printf("#endif\n");
	return 0;
}
