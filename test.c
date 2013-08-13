
#include <assert.h>
#include <inttypes.h>

#include "regex.c"


int memusage = 0;

void* testmemfunc( void* ud, void* ptr, size_t sz )
{
	if( ptr )
	{
		size_t* pp = (size_t*) ptr;
		pp--;
		memusage -= *pp;
		ptr = pp;
	}
	ptr = srx_DefaultMemFunc( NULL, ptr, sz ? sz + sizeof(size_t) : 0 );
	if( ptr )
	{
		size_t* pp = (size_t*) ptr;
		*pp = sz;
		memusage += sz;
		return pp+1;
	}
	return NULL;
}

typedef unsigned char byte;
#define GUARD 0xA5
#define PADDING 64

void mem_validate( byte* pp, byte bits, int numbytes )
{
	byte* ppe = pp + numbytes;
	while( pp < ppe )
	{
		assert( *pp == bits );
		pp++;
	}
}

void* testmemfunc2( void* ud, void* ptr, size_t sz )
{
	if( ptr )
	{
		size_t* sp, size;
		byte* pp = (byte*) ptr;
		pp -= PADDING;
		sp = (size_t*) pp;
		size = *(sp-1);
		mem_validate( pp, GUARD, PADDING );
		mem_validate( pp + size - PADDING, GUARD, PADDING );
		ptr = pp;
	}
	ptr = testmemfunc( NULL, ptr, sz ? sz + PADDING * 2 : 0 );
	if( ptr )
	{
		byte* pp = (byte*) ptr;
		memset( pp, GUARD, PADDING );
		memset( pp + sz + PADDING, GUARD, PADDING );
		return pp + PADDING;
	}
	return NULL;
}


#define TEST_DUMP 1
#define TEST_MONKEY 2
#define STREQ( a, b ) (strcmp(a,b)==0)

#define COMPTEST( pat ) \
	col = printf( "compile test: '%s'", pat ); \
	if( col > 25 ) col = 0; else col = 25 - col; \
	R = srx_CreateExt( pat, "", err, testmemfunc2, NULL ); \
	printf( "%*s output code: %d, position %d\n", col, "", err[0], err[1] ); \
	if( R && ( flags & TEST_DUMP ) ){ srx_DumpToStdout( R ); puts(""); } \
	srx_Destroy( R ); \
	assert( memusage == 0 );

#define MATCHTEST( str, pat ) \
	col = printf( "match test: '%s' like '%s'", str, pat ); \
	if( col > 35 ) col = 0; else col = 35 - col; \
	R = srx_CreateExt( pat, "", err, testmemfunc2, NULL ); \
	printf( "%*s output code: %d, position %d", col, "", err[0], err[1] ); \
	if( R ){ printf( ", match: %s\n", srx_Match( R, str ) ? "TRUE" : "FALSE" ); } else puts(""); \
	if( R && ( flags & TEST_DUMP ) ){ srx_DumpToStdout( R ); puts(""); } \
	srx_Destroy( R ); \
	assert( memusage == 0 );


int main( int argc, char* argv[] )
{
	int flags = 0, i;
	int err[2], col;
	srx_Context* R;
	
	for( i = 1; i < argc; ++i )
	{
		if( STREQ( argv[i], "dump" ) )
			flags |= TEST_DUMP;
		else if( STREQ( argv[i], "monkey" ) )
			flags |= TEST_MONKEY;
	}
	
	COMPTEST( "" );
	COMPTEST( "a" );
	COMPTEST( "[a-z]" );
	COMPTEST( "[^a-z]" );
	COMPTEST( "[^a-z]*" );
	COMPTEST( "*" );
	COMPTEST( "(a)" );
	COMPTEST( "(.)(.)" );
	COMPTEST( "a{1,2}" );
	COMPTEST( "5*?" );
	COMPTEST( ".*?" );
	COMPTEST( "^.*X.*$" );
	COMPTEST( ".*?*" );
	COMPTEST( "[-z]" );
	COMPTEST( "[a-]" );
	COMPTEST( "[^]z]" );
	COMPTEST( "|b" );
	COMPTEST( "a|" );
	COMPTEST( "a|b" );
	COMPTEST( "a(b|c)d" );
	COMPTEST( "( [a-z]{2,8}){1,2}" );
	COMPTEST( " |[a-z]{2,8}" );
	COMPTEST( "<([a-z]+)>.*?<\\1>" );
	
	MATCHTEST( "a cat", " c" );
	MATCHTEST( " in the 2013-01-02...", "[0-9]{4}-[0-9]{2}-[0-9]{2}" );
	MATCHTEST( "a cat", "(f|c)at" );
	MATCHTEST( "a cat", "(f|r)at" );
	MATCHTEST( "a cat", "a cat" );
	MATCHTEST( "a cat", "cat$" );
	MATCHTEST( "a cat", "^cat" );
	MATCHTEST( "a cat", "^a" );
	MATCHTEST( "captured", "((x(y))|(p))" );
	MATCHTEST( "a cat", ".*" );
	MATCHTEST( "a cat", ".*?" );
	MATCHTEST( "<tag> b <tag>", "<([a-z]{1,5})>.*?<\\1>" );
//	MATCHTEST( "<tag> b <tag>", "[a-z]+" );
//	MATCHTEST( "<tag> b <tag>", "([a-z]+)" );
//	MATCHTEST( "<tag> b <tag>", "<([a-z]+)>" );
//	MATCHTEST( "<tag> b <tag>", "<([a-z]+)>.*?<\\1>" );
	
	return 0;
}

