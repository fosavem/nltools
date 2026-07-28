#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

#define VERSION "v1"
#define AUTHOR "M. Chang"
#define COPYRIGHT ("Copyright (c) 2026- " AUTHOR "; GNU GPLv3+ Software")

// Windows jerryrig for POSIX access()
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

#if INT_MAX != 2147483647
#warning This code might break with int not 32-bit signed integer
#endif

uint16_t usedopts[16];
#define checkopt(x) (usedopts[(x) & 0xF] & (1 << ((x) >> 4)))

char hexadecimal[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

int main(int argc, char ** argv)
{

	// Assumption:  argv[0] is program location AND argv[1+] are command line arguments
	if(argc != 4)
	{
		fprintf(stderr, "nlcache %s by %s;\n%s\n", VERSION, AUTHOR, COPYRIGHT);

		fprintf(stderr, "\nUsage: %s OPTIONS INPUT OUTPUT\n"
		                "For more information, set OPTIONS to `h` for help.\n",
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
		fprintf(stderr, "nlcache %s by %s;\n%s\n", VERSION, AUTHOR, COPYRIGHT);

	if(!checkopt('Q') && warnings & 1)
		fprintf(stderr, "Warning: nlcache was given duplicate flags. Ignoring.\n");

	if(!checkopt('Q') && warnings & 2)
		fprintf(stderr, "Warning: nlcache was given non-ascii flags; may have corrupted flags enabled.\n");

	// Check for help
	if(checkopt('h') || checkopt('?'))
	{
		fprintf(stderr, "\nUsage: %s OPTIONS INPUT OUTPUT\n"
			        "Set `OPTIONS` to `*` for no OPTIONS.\n"
				"Set `INPUT` to the input file or `-` for stdin.\n"
				"Set `OUTPUT` to the output file or `-` for stdout.\n"
				"\n"
				"Available Options:\n"
				"q  - Quiet mode (suppress information)\n"
				"Q  - QUIET mode (suppress warnings)\n"
				"S  - SHUTUP mode (suppress errors)\n"
				"Y  - Why mode (show debug messages)\n"
				"f  - Force overwrite if output file already exists.\n"
				"b  - Use binary output instead of plaintext output.\n"
				"v  - Show version then quit.\n"
				"h? - Show this help message then quit.\n"
				"\nNote that any other option will be ignored.\n"
				"\nWarning! Note that 4 GiB files, and larger, will overflow the jump table.\n",
				argv[0]);

		return 0;
	}

	// Check version
	if(checkopt('v'))
	{
		fprintf(stderr, "nlcache:%s\n", VERSION);
		return 0;
	}

	// Define file pointers
	FILE * fin = NULL, * fout = NULL;

	if(strcmp(argv[2], argv[3]) == 0 && strcmp(argv[2], "-") != 0)
	{
		if(!checkopt('S'))
			fprintf(stderr, "Error: Cannot read from input that is also output.\n");
		return 2;
	}

	// Choose input
	if(strcmp(argv[2], "-") == 0)
	{
		if(checkopt('Y'))
			fprintf(stderr, "Debug: Using stdin as input...\n");

		fin = stdin;
	}
	else
	{
		if(checkopt('Y'))
			fprintf(stderr, "Debug: Using `%s` as file input...\n", argv[2]);

		fin = fopen(argv[2], "rb");
	}

	if(fin == NULL)
	{
		if(!checkopt('S'))
			fprintf(stderr, "Error: Failed to open input.\n");

		return 2;
	}

	// Chose output
	if(strcmp(argv[3], "-") == 0)
	{
		if(checkopt('Y'))
			fprintf(stderr, "Debug: Using stdout as output...\n");

		fout = stdout;
	}
	else
	{
		if(checkopt('Y'))
			fprintf(stderr, "Debug: Using `%s` as file output...\n", argv[3]);

		if(access(argv[3], F_OK) == 0)
		{
			if(!checkopt('f'))
			{
				if(!checkopt('S'))
					fprintf(stderr, "Error: Cowardly refusing to overwrite file. Use `f` to try and force.\n");
				if(strcmp(argv[2], "-") != 0)
					fclose(fin);
				return 2;
			}
		}

		if(!checkopt('b'))
			fout = fopen(argv[3], "w");
		else
			fout = fopen(argv[3], "wb");
	}

	if(fout == NULL)
	{
		if(!checkopt('S'))
			fprintf(stderr, "Error: Failed to open output.\n");

		// since by this point, fin was successfully opened, we close it now, if not stdin
		if(strcmp(argv[2], "-") != 0)
			fclose(fin);

		return 2;
	}

	// now, we write the 'b' / 'p' flag for binary vs plaintext
	if(checkopt('Y'))
		fprintf(stderr, "Debug: Using `%c` mode...\n", (checkopt('b') ? 'b' : 'p'));

	char magic[] = "nlcache" VERSION "/";  	
	fwrite(magic, sizeof(char), strlen(magic), fout);
	fwrite((checkopt('b') ? "b" : "p"), sizeof(char), 1, fout);

	if(checkopt('Y'))
		fprintf(stderr, "Debug: Done writing magic and binary/plaintext flag...\n");

	int prev = '\0';
	int cur  = '\0';
	uint32_t pos = 0;

	// output 0 offset entry
	if(!checkopt('b'))
		fwrite("00000000", sizeof(char), 8, fout);
	else
		fwrite("\0\0\0\0", sizeof(char), 4, fout);

	while(true)
	{
		prev = cur;
		
		if(prev == '\n')
		{
			// always write little endian
			
			if(checkopt('b'))
			{
				char buf[4] = {
					pos & 0xFF,
					(pos >> 8) & 0xFF,
					(pos >> 16) & 0xFF,
					(pos >> 24) & 0xFF,
				};

				fwrite(buf, sizeof(char), 4, fout);
			}
			else
			{
				char buf[8] = {
					hexadecimal[(pos >> 4) & 0xF],
					hexadecimal[pos & 0xF],
					hexadecimal[(pos >> 12) & 0xF],
					hexadecimal[(pos >> 8) & 0xF],
					hexadecimal[(pos >> 20) & 0xF],
					hexadecimal[(pos >> 16) & 0xF],
					hexadecimal[(pos >> 28) & 0xF],
					hexadecimal[(pos >> 24) & 0xF],
				};

				fwrite(buf, sizeof(char), 8, fout);
			}

			if(ferror(fout))
			{
				if(!checkopt('S'))
					perror("Error: write failed");
				
				if(strcmp(argv[2], "-") != 0)
					fclose(fin);
				if(strcmp(argv[3], "-") != 0)
					fclose(fout);

				return 2;
			}
		}
		
		if((cur = fgetc(fin)) == EOF)
			break;
		
		pos++;
	}

	if(!checkopt('b'))
		fwrite("TRAILER!", sizeof(char), 8, fout);
	else
		fwrite("\xFF\xFF\xFF\xFF", sizeof(char), 4, fout);

	// now, we close both fin and fout;
	if(strcmp(argv[2], "-") != 0)
		fclose(fin);
	if(strcmp(argv[3], "-") != 0)
		fclose(fout);

	if(!checkopt('q'))
		fprintf(stderr, "nlcache is done.\n");

	return 0;
}
