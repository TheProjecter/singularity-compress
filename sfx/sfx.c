/*
 * Singularity-compress: Self extracting core
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
#include "sfx.h"

static const int SFX_EFILE_IN = 10;
static const int SFX_EFILE_OUT = 11;
static const int SFX_EFILE_SEEK = 12;
static const int SFX_ENOUT = 13;

static inline int die(const char* msg, int rc)
{
	perror(msg);
	return rc;
}

int main(int argc, char* argv[])
{
	int rc;
	FILE* fin = fopen(argv[0],"rb");
	FILE* fout;

	if(!fin) 
		return die("Unable to open self", SFX_EFILE_IN);

	if(argc<2) {
		fprintf(stderr,"Usage:%s <outputfile>\n",argv[0]);
		fclose(fin);
		return SFX_ENOUT;
	}

	if( fseek(fin, SELF_SIZE, SEEK_SET) == -1)
		return die("Unable to seek", SFX_EFILE_SEEK);

	fout = fopen(argv[1],"wb");
	if(!fout)
		return die("Unable to open out file", SFX_EFILE_OUT);

	rc = unpack(fout, fin);

	fclose(fin);
	fclose(fout);

	return rc;
}

