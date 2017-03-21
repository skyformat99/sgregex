
#include "regex.c"

static void _failed( const char* msg, int line ){ printf( "\nERROR: condition failed - \"%s\"\n\tline %d\n", msg, line ); exit( 1 ); }
#define RX_ASSERT( cond ) if( !(cond) ) _failed( #cond, __LINE__ ); else printf( "+" );
#define SLB( s ) s, sizeof(s)-1
int rxTest2( rxExecute* e, rxInstr* ins, rxChar* chr, const char* s )
{
	int i, ret;
	
	printf( "#" );
	rxInitExecute( e, srx_DefaultMemFunc, NULL, ins, chr );
	ret = rxExecDo( e, s, s, strlen( s ) );
	for( i = 0; i < RX_MAX_CAPTURES; ++i )
	{
		if( e->captures[ i ][0] != RX_NULL_OFFSET || e->captures[ i ][1] != RX_NULL_OFFSET )
		{
			printf( "  capture group %d: %d - %d\n", i, e->captures[ i ][0], e->captures[ i ][1] );
		}
	}
	if( ret == 0 )
	{
		/* iteration count stack should be properly reset on failure */
		if( e->iternum_count != 0 )
			printf( "\nERROR: expected iteration number value count = 0, got %d\n", (int) e->iternum_count );
		RX_ASSERT( e->iternum_count == 0 );
	}
	return ret;
}
int rxTest( rxInstr* ins, rxChar* chr, const char* s )
{
	rxExecute e;
	int ret = rxTest2( &e, ins, chr, s );
	rxFreeExecute( &e );
	return ret;
}

int rxCompTest2( const char* expr, const char* mods, rxInstr* instrs, size_t ilen, rxChar* chars )
{
	size_t i, exprlen = strlen( expr );
	char* exprNNT = (char*) malloc( exprlen + 1 );
	strcpy( exprNNT, expr );
	exprNNT[ exprlen ] = 0x8A;
	
	rxCompiler c;
	rxInitCompiler( &c, srx_DefaultMemFunc, NULL );
	rxCompile( &c, exprNNT, exprlen );
	if( c.errcode != RXSUCCESS )
	{
		printf( " ^ return code = %d, return offset = %d\n", c.errcode, c.errpos );
	}
	else
	{
		RX_LOG( rxDumpToFile( c.instrs, c.chars, stdout ) );
	}
	
	if( instrs )
	{
		RX_ASSERT( c.errcode == RXSUCCESS );
		if( c.instrs_count != ilen ) printf( "\n ^ instruction count MISMATCH: expected %d, got %d\n", (int) ilen, (int) c.instrs_count );
		RX_ASSERT( c.instrs_count == ilen );
		for( i = 0; i < ilen; ++i )
		{
			RX_ASSERT( c.instrs[ i ].op == instrs[ i ].op );
			RX_ASSERT( c.instrs[ i ].start == instrs[ i ].start );
			RX_ASSERT( c.instrs[ i ].from == instrs[ i ].from );
			RX_ASSERT( c.instrs[ i ].len == instrs[ i ].len );
		}
	}
	
	if( chars )
	{
		RX_ASSERT( c.errcode == RXSUCCESS );
		RX_ASSERT( c.chars_count == strlen( chars ) );
		for( i = 0; i < c.chars_count; ++i )
		{
			RX_ASSERT( c.chars[ i ] == chars[ i ] );
		}
	}
	
	rxFreeCompiler( &c );
	
	free( exprNNT );
	return c.errcode; /* assuming rxFreeCompiler does not nuke this */
}

int rxCompTest( const char* expr, const char* mods )
{
	return rxCompTest2( expr, mods, NULL, 0, NULL );
}


int main()
{
	int i;
	
	puts( "##### REGEX ENGINE tests #####" );
	
	puts( "=== basic matching (string) ===" );
	{
		rxInstr instrs[] = /* aa */
		{
			{ RX_OP_MATCH_STRING, 0, 0, 2 },
			{ RX_OP_MATCH_DONE, 0, 0, 0 }
		};
		rxChar chars[] = "aa";
		RX_ASSERT( rxTest( instrs, chars, "aa" ) == 1 );
		RX_ASSERT( rxTest( instrs, chars, "aaa" ) == 1 );
		RX_ASSERT( rxTest( instrs, chars, "aab" ) == 1 );
		RX_ASSERT( rxTest( instrs, chars, "ab" ) == 0 );
		RX_ASSERT( rxTest( instrs, chars, "a" ) == 0 );
	}
	puts( "" );
	
	puts( "=== more matching (char ranges) ===" );
	{
		rxInstr instrs[] = /* aa */
		{
			{ RX_OP_MATCH_CHARSET, 0, 0, 2 },
			{ RX_OP_MATCH_CHARSET_INV, 0, 2, 2 },
			{ RX_OP_MATCH_DONE, 0, 0, 0 }
		};
		rxChar chars[] = "azAZ";
		RX_ASSERT( rxTest( instrs, chars, "a0" ) == 1 );
		RX_ASSERT( rxTest( instrs, chars, "z#" ) == 1 );
		RX_ASSERT( rxTest( instrs, chars, "f!" ) == 1 );
		RX_ASSERT( rxTest( instrs, chars, "ff" ) == 1 );
		RX_ASSERT( rxTest( instrs, chars, "Az" ) == 0 );
		RX_ASSERT( rxTest( instrs, chars, "Za" ) == 0 );
		RX_ASSERT( rxTest( instrs, chars, "aZ" ) == 0 );
		RX_ASSERT( rxTest( instrs, chars, "zA" ) == 0 );
	}
	puts( "" );
	
	puts( "=== basic branching ===" );
	{
		static const uint32_t types[] = { RX_OP_REPEAT_GREEDY, RX_OP_REPEAT_LAZY };
		for( i = 0; i < 2; ++i )
		{
			rxInstr instrs[] = /* a*b */
			{
				{ RX_OP_JUMP, 2, 0, 0 },
				{ RX_OP_MATCH_STRING, 0, 0, 1 },
				{ types[ i ], 1, 0, RX_MAX_REPEATS },
				{ RX_OP_MATCH_STRING, 0, 1, 1 },
				{ RX_OP_MATCH_DONE, 0, 0, 0 }
			};
			rxChar chars[] = "ab";
			RX_ASSERT( rxTest( instrs, chars, "ab" ) == 1 );
			RX_ASSERT( rxTest( instrs, chars, "b" ) == 1 );
			RX_ASSERT( rxTest( instrs, chars, "aaaab" ) == 1 );
			RX_ASSERT( rxTest( instrs, chars, "aaaa" ) == 0 );
		}
	}
	puts( "" );
	
	puts( "=== specific number of iterations ===" );
	{
		static const uint32_t types[] = { RX_OP_REPEAT_GREEDY, RX_OP_REPEAT_LAZY };
		for( i = 0; i < 2; ++i )
		{
			rxInstr instrs[] = /* a{3}b */
			{
				{ RX_OP_JUMP, 2, 0, 0 },
				{ RX_OP_MATCH_STRING, 0, 0, 1 },
				{ types[ i ], 1, 3, 3 },
				{ RX_OP_MATCH_STRING, 0, 1, 1 },
				{ RX_OP_MATCH_DONE, 0, 0, 0 }
			};
			rxChar chars[] = "ab";
			RX_ASSERT( rxTest( instrs, chars, "aaab" ) == 1 );
			RX_ASSERT( rxTest( instrs, chars, "aaabb" ) == 1 );
			RX_ASSERT( rxTest( instrs, chars, "aaa" ) == 0 );
			RX_ASSERT( rxTest( instrs, chars, "aab" ) == 0 );
			RX_ASSERT( rxTest( instrs, chars, "aaaaa" ) == 0 );
			RX_ASSERT( rxTest( instrs, chars, "aaaaab" ) == 0 );
		}
	}
	puts( "" );
	
	puts( "=== 'or' (|) / backtrack jump ===" );
	{
		rxInstr instrs[] = /* a|b */
		{
			{ RX_OP_BACKTRK_JUMP, 3, 0, 0 },
			{ RX_OP_MATCH_STRING, 0, 0, 1 },
			{ RX_OP_JUMP, 4, 0, 0 },
			{ RX_OP_MATCH_STRING, 0, 1, 1 },
			{ RX_OP_MATCH_DONE, 0, 0, 0 }
		};
		rxChar chars[] = "ab";
		RX_ASSERT( rxTest( instrs, chars, "a" ) == 1 );
		RX_ASSERT( rxTest( instrs, chars, "b" ) == 1 );
		RX_ASSERT( rxTest( instrs, chars, "c" ) == 0 );
	}
	puts( "" );
	
	puts( "=== nested iterations ===" );
	{
		static const uint32_t types[] = { RX_OP_REPEAT_GREEDY, RX_OP_REPEAT_LAZY };
		for( i = 0; i < 2; ++i )
		{
			rxInstr instrs[] = /* (a{1,2}b){1,3} */
			{
				{ RX_OP_JUMP, 5, 0, 0 },
				{ RX_OP_JUMP, 3, 0, 0 },
				{ RX_OP_MATCH_STRING, 0, 0, 1 },
				{ types[ i ], 2, 1, 2 },
				{ RX_OP_MATCH_STRING, 0, 1, 1 },
				{ types[ i ], 1, 1, 3 },
				{ RX_OP_MATCH_DONE, 0, 0, 0 }
			};
			rxChar chars[] = "ab";
			RX_ASSERT( rxTest( instrs, chars, "aab" ) == 1 );
			RX_ASSERT( rxTest( instrs, chars, "aabab" ) == 1 );
			RX_ASSERT( rxTest( instrs, chars, "ababaab" ) == 1 );
			RX_ASSERT( rxTest( instrs, chars, "" ) == 0 );
			RX_ASSERT( rxTest( instrs, chars, "aaabb" ) == 0 );
			RX_ASSERT( rxTest( instrs, chars, "aaa" ) == 0 );
		}
	}
	puts( "" );
	
	puts( "=== basic capturing ===" );
	{
		rxInstr instrs[] = /* (ab)(cd) */
		{
			{ RX_OP_CAPTURE_START, 0, 0, 0 },
			{ RX_OP_MATCH_STRING, 0, 0, 2 },
			{ RX_OP_CAPTURE_END, 0, 0, 0 },
			{ RX_OP_CAPTURE_START, 0, 1, 0 },
			{ RX_OP_MATCH_STRING, 0, 2, 2 },
			{ RX_OP_CAPTURE_END, 0, 1, 0 },
			{ RX_OP_MATCH_DONE, 0, 0, 0 }
		};
		rxChar chars[] = "abcd";
		{
			rxExecute e;
			RX_ASSERT( rxTest2( &e, instrs, chars, "abcd" ) == 1 );
			RX_ASSERT( e.captures[0][0] == 0 );
			RX_ASSERT( e.captures[0][1] == 2 );
			RX_ASSERT( e.captures[1][0] == 2 );
			RX_ASSERT( e.captures[1][1] == 4 );
			RX_ASSERT( e.captures[2][0] == RX_NULL_OFFSET );
			RX_ASSERT( e.captures[2][1] == RX_NULL_OFFSET );
			rxFreeExecute( &e );
			
			/* test if changes are reverted */
			RX_ASSERT( rxTest2( &e, instrs, chars, "abc" ) == 0 );
			RX_ASSERT( e.captures[0][0] == RX_NULL_OFFSET );
			RX_ASSERT( e.captures[0][1] == RX_NULL_OFFSET );
			RX_ASSERT( e.captures[1][0] == RX_NULL_OFFSET );
			RX_ASSERT( e.captures[1][1] == RX_NULL_OFFSET );
			RX_ASSERT( e.captures[2][0] == RX_NULL_OFFSET );
			RX_ASSERT( e.captures[2][1] == RX_NULL_OFFSET );
			rxFreeExecute( &e );
		}
	}
	puts( "" );
	
	puts( "=== optional capturing 1 ===" );
	{
		rxInstr instrs[] = /* (ab)|(cd) */
		{
			{ RX_OP_BACKTRK_JUMP, 5, 0, 0 },
			{ RX_OP_CAPTURE_START, 0, 0, 0 },
			{ RX_OP_MATCH_STRING, 0, 0, 2 },
			{ RX_OP_CAPTURE_END, 0, 0, 0 },
			{ RX_OP_JUMP, 8, 0, 0 },
			{ RX_OP_CAPTURE_START, 0, 1, 0 },
			{ RX_OP_MATCH_STRING, 0, 2, 2 },
			{ RX_OP_CAPTURE_END, 0, 1, 0 },
			{ RX_OP_MATCH_DONE, 0, 0, 0 }
		};
		rxChar chars[] = "abcd";
		{
			rxExecute e;
			RX_ASSERT( rxTest2( &e, instrs, chars, "ab" ) == 1 );
			RX_ASSERT( e.captures[0][0] == 0 );
			RX_ASSERT( e.captures[0][1] == 2 );
			RX_ASSERT( e.captures[1][0] == RX_NULL_OFFSET );
			RX_ASSERT( e.captures[1][1] == RX_NULL_OFFSET );
			RX_ASSERT( e.captures[2][0] == RX_NULL_OFFSET );
			RX_ASSERT( e.captures[2][1] == RX_NULL_OFFSET );
			rxFreeExecute( &e );
			
			RX_ASSERT( rxTest2( &e, instrs, chars, "cd" ) == 1 );
			RX_ASSERT( e.captures[0][0] == RX_NULL_OFFSET );
			RX_ASSERT( e.captures[0][1] == RX_NULL_OFFSET );
			RX_ASSERT( e.captures[1][0] == 0 );
			RX_ASSERT( e.captures[1][1] == 2 );
			RX_ASSERT( e.captures[2][0] == RX_NULL_OFFSET );
			RX_ASSERT( e.captures[2][1] == RX_NULL_OFFSET );
			rxFreeExecute( &e );
			
			RX_ASSERT( rxTest2( &e, instrs, chars, "ac" ) == 0 );
			RX_ASSERT( e.captures[0][0] == RX_NULL_OFFSET );
			RX_ASSERT( e.captures[0][1] == RX_NULL_OFFSET );
			RX_ASSERT( e.captures[1][0] == RX_NULL_OFFSET );
			RX_ASSERT( e.captures[1][1] == RX_NULL_OFFSET );
			RX_ASSERT( e.captures[2][0] == RX_NULL_OFFSET );
			RX_ASSERT( e.captures[2][1] == RX_NULL_OFFSET );
			rxFreeExecute( &e );
		}
	}
	puts( "" );
	
	puts( "=== optional capturing 2 ===" );
	{
		rxInstr instrs[] = /* (ab|cd) */
		{
			{ RX_OP_CAPTURE_START, 0, 0, 0 },
			{ RX_OP_BACKTRK_JUMP, 4, 0, 0 },
			{ RX_OP_MATCH_STRING, 0, 0, 2 },
			{ RX_OP_JUMP, 5, 0, 0 },
			{ RX_OP_MATCH_STRING, 0, 2, 2 },
			{ RX_OP_CAPTURE_END, 0, 0, 0 },
			{ RX_OP_MATCH_DONE, 0, 0, 0 }
		};
		rxChar chars[] = "abcd";
		{
			rxExecute e;
			RX_ASSERT( rxTest2( &e, instrs, chars, "ab" ) == 1 );
			RX_ASSERT( e.captures[0][0] == 0 );
			RX_ASSERT( e.captures[0][1] == 2 );
			RX_ASSERT( e.captures[1][0] == RX_NULL_OFFSET );
			RX_ASSERT( e.captures[1][1] == RX_NULL_OFFSET );
			rxFreeExecute( &e );
			
			RX_ASSERT( rxTest2( &e, instrs, chars, "cd" ) == 1 );
			RX_ASSERT( e.captures[0][0] == 0 );
			RX_ASSERT( e.captures[0][1] == 2 );
			RX_ASSERT( e.captures[1][0] == RX_NULL_OFFSET );
			RX_ASSERT( e.captures[1][1] == RX_NULL_OFFSET );
			rxFreeExecute( &e );
			
			RX_ASSERT( rxTest2( &e, instrs, chars, "ac" ) == 0 );
			RX_ASSERT( e.captures[0][0] == RX_NULL_OFFSET );
			RX_ASSERT( e.captures[0][1] == RX_NULL_OFFSET );
			RX_ASSERT( e.captures[1][0] == RX_NULL_OFFSET );
			RX_ASSERT( e.captures[1][1] == RX_NULL_OFFSET );
			rxFreeExecute( &e );
		}
	}
	puts( "" );
	
	puts( "=== all tests done! ===" );
	
	puts( "##### REGEX COMPILER tests #####" );
	
	puts( "=== plain string ===" );
	RX_ASSERT( rxCompTest( "a", "" ) == RXSUCCESS );
	RX_ASSERT( rxCompTest( "ab", "" ) == RXSUCCESS );
	{
		rxInstr instrs[] =
		{
			{ RX_OP_CAPTURE_START, 0, 0, 0 },
			{ RX_OP_MATCH_STRING, 0, 0, 1 },
			{ RX_OP_CAPTURE_END, 0, 0, 0 },
			{ RX_OP_MATCH_DONE, 0, 0, 0 },
		};
		RX_ASSERT( rxCompTest2( "a", "", instrs, 4, "a" ) == RXSUCCESS );
	}
	{
		rxInstr instrs[] =
		{
			{ RX_OP_CAPTURE_START, 0, 0, 0 },
			{ RX_OP_MATCH_STRING, 0, 0, 2 },
			{ RX_OP_CAPTURE_END, 0, 0, 0 },
			{ RX_OP_MATCH_DONE, 0, 0, 0 },
		};
		RX_ASSERT( rxCompTest2( "ab", "", instrs, 4, "ab" ) == RXSUCCESS );
	}
	{
		rxInstr instrs[] =
		{
			{ RX_OP_CAPTURE_START, 0, 0, 0 },
			{ RX_OP_MATCH_STRING, 0, 0, 3 },
			{ RX_OP_CAPTURE_END, 0, 0, 0 },
			{ RX_OP_MATCH_DONE, 0, 0, 0 },
		};
		RX_ASSERT( rxCompTest2( "abc", "", instrs, 4, "abc" ) == RXSUCCESS );
	}
	puts( "" );
	
	puts( "=== character classes ===" );
	RX_ASSERT( rxCompTest( "[a]", "" ) == RXSUCCESS );
	RX_ASSERT( rxCompTest( "[ab]", "" ) == RXSUCCESS );
	{
		rxInstr instrs[] =
		{
			{ RX_OP_CAPTURE_START, 0, 0, 0 },
			{ RX_OP_MATCH_CHARSET, 0, 0, 4 },
			{ RX_OP_CAPTURE_END, 0, 0, 0 },
			{ RX_OP_MATCH_DONE, 0, 0, 0 },
		};
		RX_ASSERT( rxCompTest2( "[ab]", "", instrs, 4, "aabb" ) == RXSUCCESS );
	}
	RX_ASSERT( rxCompTest( "[^a]", "" ) == RXSUCCESS );
	{
		rxInstr instrs[] =
		{
			{ RX_OP_CAPTURE_START, 0, 0, 0 },
			{ RX_OP_MATCH_CHARSET_INV, 0, 0, 4 },
			{ RX_OP_CAPTURE_END, 0, 0, 0 },
			{ RX_OP_MATCH_DONE, 0, 0, 0 },
		};
		RX_ASSERT( rxCompTest2( "[^ab]", "", instrs, 4, "aabb" ) == RXSUCCESS );
	}
	{
		rxInstr instrs[] =
		{
			{ RX_OP_CAPTURE_START, 0, 0, 0 },
			{ RX_OP_MATCH_CHARSET, 0, 0, 2 },
			{ RX_OP_CAPTURE_END, 0, 0, 0 },
			{ RX_OP_MATCH_DONE, 0, 0, 0 },
		};
		RX_ASSERT( rxCompTest2( "[a-b]", "", instrs, 4, "ab" ) == RXSUCCESS );
	}
	RX_ASSERT( rxCompTest( "[a-bcd-e]", "" ) == RXSUCCESS );
	RX_ASSERT( rxCompTest( "[]]", "" ) == RXSUCCESS );
	RX_ASSERT( rxCompTest( "[^]]", "" ) == RXSUCCESS );
	RX_ASSERT( rxCompTest( "[", "" ) == RXEPART );
	RX_ASSERT( rxCompTest( "[a", "" ) == RXEPART );
	RX_ASSERT( rxCompTest( "[b-a]", "" ) == RXERANGE );
	{
		rxInstr instrs[] =
		{
			{ RX_OP_CAPTURE_START, 0, 0, 0 },
			{ RX_OP_MATCH_STRING, 0, 0, 2 },
			{ RX_OP_MATCH_CHARSET, 0, 2, 2 },
			{ RX_OP_CAPTURE_END, 0, 0, 0 },
			{ RX_OP_MATCH_DONE, 0, 0, 0 },
		};
		RX_ASSERT( rxCompTest2( "aa[b]", "", instrs, 5, "aabb" ) == RXSUCCESS );
	}
	puts( "" );
	
	puts( "=== all tests done! ===" );
	
	return 0;
}