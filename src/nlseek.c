#ifdef Limit2FIO32
#warning Building without File I/O with 64-bit offsets.
#endif

#ifndef _WIN32

#ifndef Limit2FIO32
#define _FILE_OFFSET_BITS 64  
#endif

#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <errno.h>

#ifndef _WIN32
#include <sys/types.h>

typedef off_t foff_t;

#ifndef Limit2FIO32

#if (!defined(__STDC_VERSION__) || __STDC_VERSION__ < 201112L)
typedef char IwhiQU[(sizeof(foff_t) == 8) ? 1 : -1]; // foff_t not truely 64-bits whilst Limit2FIO32 not enabled
#else
static_assert(sizeof(foff_t) == 8, "foff_t is not truely 64-bit whilst Limit2FIO32 not enabled");
#endif

#endif

#else // Windows jerryrig

#ifndef Limit2FIO32
#define fseeko _fseeki64
#define ftello _ftelli64
typedef int64_t foff_t;
#else
#define fseeko fseek
#define ftello ftell
typedef long foff_t;
#endif

#endif

#define VERSION "v1"
#define AUTHOR "M. Chang"
#define COPYRIGHT ("Copyright (c) 2026- " AUTHOR "; GNU GPLv3+ Software")

#ifdef _WIN32
#include <io.h>
#define F_OK 0
#define access _access
#else
#include <unistd.h>
#endif

#if CHAR_BIT != 8
#error This code does not work for char size not equal to 1 byte!
#endif

uint16_t usedopts[16];
#define checkopt(x) (usedopts[(x) & 0xF] & (1 << ((x) >> 4)))
#define hex2bin(x) ((((uint8_t)x) & 0xF0) == 0x30 ? ((uint8_t)x) - 0x30 : ((uint8_t)x) - 0x37)

int main(int argc, char ** argv)
{
	// Assumption:  argv[0] is program location AND argv[1+] are command line arguments
	if(argc < 6 || argc > 7)
	{
		fprintf(stderr, "nlseek %s by %s;\n%s\n", VERSION, AUTHOR, COPYRIGHT);

		fprintf(stderr, "\nUSAGE: %s OPTIONS SOURCE INPUT OUTPUT OFFSET [prefix]\n"
		                "Use `h` in the OPTIONS menu to show the help message.\n",
				argv[0]);

		return 1;
	}

	// Build used options table

	for(char i = 0; i < 16; i++)
		usedopts[i] = 0;

	uint8_t warnings = 0;

	for(char * ptr = argv[1]; *ptr != '\0'; ptr++)
	{
		if((unsigned char)*ptr > 127)
			warnings |= 2;

		if(checkopt((unsigned char)*ptr) == 0)
			usedopts[(unsigned char)(*ptr) & 0xF] |= 1 << ((unsigned char)(*ptr) >> 4);
		else
			warnings |= 1;
	}

	if(!checkopt('q'))
		fprintf(stderr, "nlseek %s by %s;\n%s\n", VERSION, AUTHOR, COPYRIGHT);

	if(!checkopt('Q') && warnings & 1)
		fprintf(stderr, "Warning: nlseek was given duplicate flags. Ignoring.\n");

	if(!checkopt('Q') && warnings & 2)
		fprintf(stderr, "Warning: nlseek was given non-ascii flags; may have corrupted flags enabled.\n");

	// Check for help
	if(checkopt('h') || checkopt('?'))
	{
		fprintf(stderr, "\nUSAGE: %s OPTIONS SOURCE INPUT OUTPUT OFFSET [prefix]\n"
				"Set `OPTIONS` to include the options desired.\n"
				"Set `SOURCE` to the source file from which the table was created from.\n"
				"Set `INPUT` to the jump table file or `-` for stdin.\n"
				"Set `OUTPUT` to the output file or `-` for stdout.\n"
				"Set `OFFSET` to the line number thy seeks (starting at 1)\n"
				"Optionally set `prefix` to the prefix that should be matched to.\n"
				"\n"
				"Available Options:\n"
				"q  - Quiet mode (suppress information)\n"
				"Q  - QUIET mode (suppress warnings)\n"
				"S  - SHUTUP mode (suppress errors)\n"
				"Y  - Why mode (show debug messages)\n"
				"f  - Force overwrite if output file already exists.\n"
				"p  - Use text output instead of binary output.\n"
				"v  - Show version then quit.\n"
				"h? - Show this help message then quit.\n"
				"\nNote that any other option will be ignored.\n"
				"If `INPUT` is set to stdin, expect sluggishness!\n",
				argv[0]);

		return 0;
	}

	// Check version
	if(checkopt('v'))
	{
		fprintf(stderr, "nlseek:%s:%s\n", VERSION,
#ifdef Limit2FIO32
			"Limit2FIO32"
#else
			"FIO64"
#endif
		       );
		return 0;
	}

	// Do File I/O
	FILE * fsource = NULL, * fin = NULL, * fout = NULL;

	if((strcmp(argv[2], argv[4]) == 0 || strcmp(argv[3], argv[4]) == 0) && strcmp(argv[4], "-") != 0)
	{
		if(!checkopt('S'))
			fprintf(stderr, "Error: Cannot read from input or source that is also output.\n");
		return 2;
	}

	// Open source
	if(checkopt('Y'))
		fprintf(stderr, "Debug: Using `%s` as source...\n", argv[2]);

	fsource = fopen(argv[2], "rb");

	if(fsource == NULL)
	{
		if(!checkopt('S'))
			fprintf(stderr, "Error: Failed to open source.\n");

		return 2;
	}

	// Choose input
	if(strcmp(argv[3], "-") == 0)
	{
		if(checkopt('Y'))
			fprintf(stderr, "Debug: Using stdin as input...\n");

		fin = stdin;
	}
	else
	{
		if(checkopt('Y'))
			fprintf(stderr, "Debug: Using `%s` as file input...\n", argv[3]);

		fin = fopen(argv[3], "rb");
	}

	if(fin == NULL)
	{
		if(!checkopt('S'))
			fprintf(stderr, "Error: Failed to open input.\n");

		fclose(fsource);
		return 2;
	}

	// Chose output
	if(strcmp(argv[4], "-") == 0)
	{
		if(checkopt('Y'))
			fprintf(stderr, "Debug: Using stdout as output...\n");

		fout = stdout;
	}
	else
	{
		if(checkopt('Y'))
			fprintf(stderr, "Debug: Using `%s` as file output...\n", argv[4]);

		if(access(argv[4], F_OK) == 0)
		{
			if(!checkopt('f'))
			{
				if(!checkopt('S'))
					fprintf(stderr, "Error: Cowardly refusing to overwrite file. Use `f` to try and force.\n");

				fclose(fsource);
				if(strcmp(argv[3], "-") != 0)
					fclose(fin);
				return 2;
			}
		}

		if(!checkopt('p'))
			fout = fopen(argv[4], "w");
		else
			fout = fopen(argv[4], "wb");
	}

	if(fout == NULL)
	{
		if(!checkopt('S'))
			fprintf(stderr, "Error: Failed to open output.\n");

		// since by this point, fin was successfully opened, we close it now, if not stdin
		fclose(fsource);
		if(strcmp(argv[3], "-") != 0)
			fclose(fin);

		return 2;
	}

	// Validate magic
	bool usingP = false;
	char magicpfx[] = "nlcache" VERSION "/";
	int magicpfxlen = strlen(magicpfx);

	int ch = 0;
	bool wrongmagic = false;
	for(int i = 0; i < magicpfxlen; i++)
	{
		ch = fgetc(fin);

		if(ch != magicpfx[i])
			wrongmagic = true;
	}

	ch = fgetc(fin);

	switch(ch)
	{
		case 'p':
			usingP = true;
		case 'b':
			break;
		default:
			wrongmagic = true;
	}

	if(wrongmagic)
	{
		if(!checkopt('S'))
			fprintf(stderr, "Error: Not a valid nlcache v1 file!\n");

		fclose(fsource);

		if(strcmp(argv[3], "-") != 0)
			fclose(fin);
		
		if(strcmp(argv[4], "-") != 0)
			fclose(fout);
		return 3;
	}

	// Advance to desired line location
	errno = 0;
	char * endptr = NULL;
	unsigned long neededentryul = strtoul(argv[5], &endptr, 10);
	
	if(endptr == NULL || *endptr != '\0')
	{
		if(!checkopt('S'))
			fprintf(stderr, "Error: Malformed requested line number!\n");

		fclose(fsource);

		if(strcmp(argv[3], "-") != 0)
			fclose(fin);
		
		if(strcmp(argv[4], "-") != 0)
			fclose(fout);
		return 3;
	}

	if(errno == ERANGE || neededentryul > 2147483647 || neededentryul == 0)
	{
		if(!checkopt('S'))
			fprintf(stderr, "Error: Requested line number out of range!\n");

		fclose(fsource);

		if(strcmp(argv[3], "-") != 0)
			fclose(fin);
		
		if(strcmp(argv[4], "-") != 0)
			fclose(fout);
		return 3;
	}

	uint32_t neededentry = neededentryul;

	if(strcmp(argv[3], "-") == 0)
	{
		char buf[8] = {};
		for(uint32_t counter = 1; counter < neededentry; counter++)
		{
			if(usingP) // we don't actually care if they are wellformed within these ranges...
			{
				size_t r = fread(buf, sizeof(char), 8, fin);

				if(r != 8)
				{
					if(!checkopt('S'))
						fprintf(stderr, "Error: Requested line number out of range!\n");

					fclose(fsource);

					if(strcmp(argv[3], "-") != 0)
						fclose(fin);
					
					if(strcmp(argv[4], "-") != 0)
						fclose(fout);
					return 3;
				}
			}
			else
			{
				size_t r = fread(buf, sizeof(char), 4, fin);

				if(r != 4)
				{
					if(!checkopt('S'))
						fprintf(stderr, "Error: Requested line number out of range!\n");

					fclose(fsource);

					if(strcmp(argv[3], "-") != 0)
						fclose(fin);
					
					if(strcmp(argv[4], "-") != 0)
						fclose(fout);
					return 3;
				}
			}
		}
	}
	else
	{
		uint64_t result = neededentry - 1;
		result *= 4;

		if(usingP)
			result *= 2;

		while(result > 0)
		{
#ifdef Limit2FIO32
			foff_t offset = (result < 2147483647 ? result : 2147483647);
#else
			foff_t offset = result;
#endif
			fseeko(fin, offset, SEEK_CUR); // if this ends up EOF, then will catch later

			result -= offset;
		}
	}

	uint32_t sourceoffset = 0;
	char buf[8] = {};

	if(usingP)
	{
		size_t r = fread(buf, sizeof(char), 8, fin);
		
		if(r != 8)
		{
			if(!checkopt('S'))
				fprintf(stderr, "Error: Requested line number out of range!\n");

			fclose(fsource);

			if(strcmp(argv[3], "-") != 0)
				fclose(fin);
			
			if(strcmp(argv[4], "-") != 0)
				fclose(fout);
			return 3;
		}
	}
	else
	{
		size_t r = fread(buf, sizeof(char), 4, fin);

		if(r != 4)
		{
			if(!checkopt('S'))
				fprintf(stderr, "Error: Requested line number out of range!\n");

			fclose(fsource);

			if(strcmp(argv[3], "-") != 0)
				fclose(fin);
			
			if(strcmp(argv[4], "-") != 0)
				fclose(fout);
			return 3;
		}
	}

	if(usingP)
	{
		char trailer[] = "TRAILER!";

		bool istrailer = true;
		bool isvalid = true;
		for(char i = 0; i < 8; i++)
		{
			if(buf[i] != trailer[i])
				istrailer = false;

			if(buf[i] < '0' || buf[i] > '9' && buf[i] < 'A' || buf[i] > 'F')
				isvalid = false;
		}

		if(istrailer)
		{
			if(!checkopt('S'))
				fprintf(stderr, "Error: Requested line number out of range!\n");

			fclose(fsource);

			if(strcmp(argv[3], "-") != 0)
				fclose(fin);
			
			if(strcmp(argv[4], "-") != 0)
				fclose(fout);
			return 3;
		}

		if(!isvalid)
		{
			if(!checkopt('S'))
				fprintf(stderr, "Error: Not a valid nlcache v1 file!\n");

			fclose(fsource);

			if(strcmp(argv[3], "-") != 0)
				fclose(fin);
			
			if(strcmp(argv[4], "-") != 0)
				fclose(fout);
			return 3;
		}
	
		buf[0] = ((uint8_t)hex2bin(buf[0]) << 4) | (uint8_t)hex2bin(buf[1]);
		buf[1] = ((uint8_t)hex2bin(buf[2]) << 4) | (uint8_t)hex2bin(buf[3]);
		buf[2] = ((uint8_t)hex2bin(buf[4]) << 4) | (uint8_t)hex2bin(buf[5]);
		buf[3] = ((uint8_t)hex2bin(buf[6]) << 4) | (uint8_t)hex2bin(buf[7]);
	}
	
	sourceoffset = ((uint32_t)(uint8_t)buf[0]) | ((uint32_t)(uint8_t)buf[1] << 8) |
		       ((uint32_t)(uint8_t)buf[2] << 16) | ((uint32_t)(uint8_t)buf[3] << 24);
	
	if(!usingP && sourceoffset == 0xFFFFFFFF)
	{
		if(!checkopt('S'))
			fprintf(stderr, "Error: Requested line number out of range!\n");

		fclose(fsource);

		if(strcmp(argv[3], "-") != 0)
			fclose(fin);
		
		if(strcmp(argv[4], "-") != 0)
			fclose(fout);
		return 3;
	}

	if(fseeko(fsource, sourceoffset, SEEK_SET) != 0 || ftello(fsource) != sourceoffset)
	{
		if(!checkopt('S'))
			fprintf(stderr, "Error: Offset in source doesn't exist! Looked for 0x%08X.\n", sourceoffset);

		fclose(fsource);

		if(strcmp(argv[3], "-") != 0)
			fclose(fin);
		
		if(strcmp(argv[4], "-") != 0)
			fclose(fout);
		return 4;
	}

	if(ftello(fsource) > 0)
	{
		fseeko(fsource, -1, SEEK_CUR);
		int ch = fgetc(fsource);

		if(ch != '\n')
		{
			if(!checkopt('S'))
				fprintf(stderr, "Error: Table offset does not point to newline! Looked for 0x%08X.\n", sourceoffset);

			fclose(fsource);

			if(strcmp(argv[3], "-") != 0)
				fclose(fin);
			
			if(strcmp(argv[4], "-") != 0)
				fclose(fout);
			return 4;
		}
	}

	if(argc == 7)
	{
		size_t pfxlen = strlen(argv[6]);
		for(size_t idx = 0; idx < pfxlen; idx++)
		{
			int ch = fgetc(fsource);

			if(ch != argv[6][idx])
			{
				if(!checkopt('S'))
					fprintf(stderr, "Error: Prefix does not match! Looked for 0x%08X.\n", sourceoffset);

				fclose(fsource);

				if(strcmp(argv[3], "-") != 0)
					fclose(fin);
				
				if(strcmp(argv[4], "-") != 0)
					fclose(fout);
				return 4;
			}
		}
	}

	int last = -1;
	ch = 0;
	while((ch = fgetc(fsource)) != EOF && ch != '\n')
	{
		if(!checkopt('p'))
			fputc(ch, fout);
		else if(last != -1)
			fputc(last, fout);

		last = ch;
	}
	
	// now we have to worry about not outputting \r for \r\n when checkopt(p)

	if(!(ch == '\n' && last == '\r') && checkopt('p'))
		fputc(last, fout);
	fprintf(fout, "\n");
	
	fclose(fsource);

	if(strcmp(argv[3], "-") != 0)
		fclose(fin);
	
	if(strcmp(argv[4], "-") != 0)
		fclose(fout);

	return 0;
}
