#include <stdio.h>
#include <fcntl.h>

#include "simple_d.h"
static void usage(void)
{
	fprintf(stderr,"simple_d [inputfile [outputfile]]\n");
	exit(1);
}
int main( int argc, char *argv[] )
{
	if ((argc > 3) || ((argc>1) && (argv[1][0]=='-')))
		usage();

	if ( argc<=1 )
		fprintf( stderr, "stdin" );
	else {  
		freopen( argv[1], "rb", stdin );
		fprintf( stderr, "%s", argv[1] );
	}
	if ( argc<=2 )
		fprintf( stderr, " to stdout\n" );
	else {  
		freopen( argv[2], "wb", stdout );
		fprintf( stderr, " to %s\n", argv[2] );
	}

#ifndef unix
	setmode( fileno( stdin ), O_BINARY );
	setmode( fileno( stdout ), O_BINARY );
#endif
	return unpack(stdout,stdin);
}
