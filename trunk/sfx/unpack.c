/*
 * Singularity-compress: stub unpacker core
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
#include "unpack.h"

int unpack(FILE* out, FILE* in)
{
	const size_t BUF_SIZE = 8192;
	char buff[BUF_SIZE];
	size_t read;
	do{
		read = fread(buff, 1, BUF_SIZE, in);
		if(read)
			fwrite(buff, 1, read, out);
	} while(read);
	return 0;
}

