#!/usr/bin/php
<?php

require( '_cssmin-v3.0.1.php' );

echo 'Building stylesheet with embedded Base64 images...' . PHP_EOL;

// read the original CSS into memory
$css = file_get_contents( './original/style.css' );
echo '- Loaded source stylesheet (' . strlen( $css ) . ' bytes).' . PHP_EOL;

// convert each smushed image to base64 and merge with the in-memory stylesheet
$imagedir = './original/images_smushed';
$files = scandir( $imagedir );
foreach( $files as $file ){
	if( substr( $file, -4 ) == '.png' ){
		echo '- Converting ' . $file . ' to Base64 and embedding in CSS...' . PHP_EOL;
		$img_base64 = 'data:image/png;base64,' . base64_encode( file_get_contents( $imagedir . '/' . $file ) );
		
		// merge the base64 data into the stylesheet
		$css = str_replace( 'url(images/' . $file . ')', 'url(' . $img_base64 . ')', $css);
	}
}

// minify the final stylesheet and save it to disk
echo '- Minifying stylesheet to compress it...';
$cssmin = CssMin::minify( $css );
file_put_contents( './style_compact_data.css', $cssmin );
echo ' (before: ' . strlen( $css ) . ' bytes, after: ' . strlen( $cssmin ) . ' bytes)' . PHP_EOL;

// this is a simplified PHP version of the file2header program, which only processes text here
// note: we use windows line endings so that the header will be readable for people stuck on windows
function text2header( $varname, $str ){
	$outstr = "static const unsigned char " . $varname . "[] = {";
	
	$c = ""; // holds the current character
	for( $pos=0,$len=strlen( $str ); $pos<$len; ++$pos ){
		// determine separators
		if( $pos == 0 ){ // if this is the first byte, we don't want any separators, just a newline and two spaces
			$outstr .= "\r\n  ";
		}else{ // otherwise, determine separators and line splitting based on length so far
			if( $pos % 12 == 0 ){ $outstr .= ",\r\n  "; } // if this is the last column in the row, insert a comma, newline and two spaces
			else{ $outstr .= ", "; } // separate each column with a comma and space, as long as this is not the last column in a row
		}
		
		// add the current character to the output
		$c = ord( $str[$pos] );
		$outstr .= "0x" . strtoupper( base_convert( $c, 10, 16 ) );
		
		// if the current character is a null byte, we should stop processing here since we only want text-data
		if( $c == 0 ){
			break;
		}
	}
	
	// finish up processing and output length
	if( $c != 0 ){
		// if it wasn't already null terminated, we add a null character and count that too
		if( $pos % 12 == 0 ){ $outstr .= ",\r\n  "; } // remember to columnize
		else{ $outstr .= ", "; } // and to separate if we didn't have to columnize
		$outstr .= "0x0";
		$pos++;
	}
	
	$outstr .= "\r\n};\r\n" .
	           "static const unsigned int " . $varname . "_len = " . $pos . ";"; // NOTE: length includes the null byte which terminates the text

	return $outstr;
}

// convert the minified and data-containing CSS into a C header and save it to disk
echo '- Saving final stylesheet as C header...' . PHP_EOL;
$cssheader = text2header( 'style_compact_data_css', $cssmin );
file_put_contents( 'style_compact_data_css.h', $cssheader );
echo '- Header saved to style_compact_data_css.h!' . PHP_EOL;

?>