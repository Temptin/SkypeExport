/*
 * Reads in a file and converts it to a character array ready for use in headers.
 *
 * In text mode, it reads until the first \0 (NULL byte), or until EOF if no NULL byte is found before that (in that case, it appends a NULL byte of its own).
 * In binary mode, it reads the entire file until EOF.
 *
 * The length variable is the exact size of the data in either mode. In binary mode it's the data size, and in text mode it's the number of bytes + the null byte.
 * For example, the string "hello" would be 6 bytes (5 character bytes + 1 NULL byte). Note that it's NOT usable as the number of characters in case of multibyte character encodings!
 *
 * Also NOTE: If you want to include the resulting headers into multiple files without encasing these statements in any sort of function or member scope, you
 * will have to prepend "static" to the types of the char array and integer, or even "static const" if you want them to be unmodifiable.
 * FIXME: May wanna add an optional 3rd argument for "[sc]" flags which automatically prepends static and/or const when desired.
 */

#include <iostream>
#include <fstream>
#include <string>
#include <stdint.h>

int main( int argc, char** argv )
{
	// verify and parse program arguments
	if( argc != 3 ){
		std::cerr << "This program requires two arguments:\n" << argv[0] << " t|b inputfile" << std::endl;
		return 1;
	}
	bool isText = ( *(argv[1]) == 't' ? true : false ); // treat as text if arg2 is t, otherwise treat as binary
	std::ifstream ifile( argv[2], std::ifstream::in | std::ifstream::binary );
	if( !ifile ){
		std::cerr << "Input file doesn't exist:\n\"" << argv[2] << "\"" << std::endl;
		return 1;
	}
	if( ifile.peek() == std::ifstream::traits_type::eof() ){ // empty input file
		std::cerr << "Input file is empty:\n\"" << argv[2] << "\"" << std::endl;
		return 1;
	}

	// clean up the given filename
	char *inputc = argv[2];
	do{ // VERY rudimentary cleanup which directly modifies the argument. FIXME: it should also ensure that the identifier doesn't begin with multiple underscores, or a digit.
		if( !(	( *inputc >= 'a' && *inputc <= 'z' ) || // a-z
				( *inputc >= 'A' && *inputc <= 'Z' ) || // A-Z
				( *inputc >= '0' && *inputc <= '9' ) ) ){ // 0-9
			*inputc = '_'; // replace illegal characters with an underscore instead
		}
	}while( *(++inputc) );

	// alright we're ready to convert the input file!
	uint64_t len = 0; // used to calculate length as well as to columnize
	unsigned char c = 0; // holds the current character
	std::cout << "unsigned char " << argv[2] << "[] = {";
	while( ifile.good() && ifile.peek() != std::ifstream::traits_type::eof() ){ // NOTE: this is a bit ugly but is done to prevent overwriting "c" with 0xFF when we reach EOF, as we need the last-output value of c.
		c = (unsigned char)ifile.get();

		if( len == 0 ){ // if this is the first byte, we don't want any separators, just a newline and two spaces
			std::cout << "\n  ";
		}else{ // otherwise, determine separators and line splitting based on length so far
			if( len % 12 == 0 ){ std::cout << ",\n  "; } // if this is the last column in the row, insert a comma, newline and two spaces
			else{ std::cout << ", "; } // separate each column with a comma and space, as long as this is not the last column in a row
		}
		
		printf("0x%X", (unsigned char)c); // FIXME: may want to go fully C++ by replacing it with stomething like "std::cout << std::hex << blabla << std::backtonormal"
		len++;

		if( isText && c == '\0' ){ break; } // if we're doing text mode output and we've just encountered a null character, we should stop processing here (this also won't increment length to count the null byte)
	}
	ifile.close();

	// finish up processing and output length
	if( isText && c != '\0'){
		// if this is text mode and it wasn't already null terminated, we add a null character and count it
		if( len % 12 == 0 ){ std::cout << ",\n  "; } // remember to columnize
		else{ std::cout << ", "; } // and to separate if we didn't have to columnize
		std::cout << "0x0";
		len++;
	}
	std::cout	<< "\n};\n"
				<< "unsigned int " << argv[2] << "_len = " << len << ";"; // NOTE: in text mode, length includes the null byte
}