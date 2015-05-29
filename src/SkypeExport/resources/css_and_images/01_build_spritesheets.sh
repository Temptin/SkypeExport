#!/usr/local/opt/php56/bin/php
<?php

/*
	WARNING: On OS X 10.10 Yosemite, they no longer include the PNG functions in the GD library.
	
	To solve that, you must manually install PHP 5.6 so that this script can execute properly:
	
	1. Go to http://brew.sh/ and run the main installation command.
	
	2. To install PHP, type "brew install homebrew/php/php56". In the future, other versions will be newer and this command will have to be adjusted, as well as the binary path at the top of this script.
	
	3. This script will work as soon as the installation is complete!
*/

set_time_limit( 0 );


// Make sure they are on OS X and that the GD-PNG functions exist
if( php_uname( 's' ) != 'Darwin' ){
	die( 'Error: This script only runs on Mac OS X.' );
}
if( !function_exists( 'imagecreatefrompng' ) ){
	die( 'Error: Your PHP installation lacks support for gd\'s PNG reading/writing. Read the comment at the top of this script for instructions on installing a proper version of PHP via Homebrew.' );
}


// Common properties
$skypeapp = '/Applications/Skype.app';
$emoticons_folder = $skypeapp . '/Contents/Resources';
$emoticons_plist = $emoticons_folder . '/Emoticons.plist';
$emoticons_tmp_json = 'build_sprites_emoticons.tmp';

if( !is_dir( $skypeapp ) ){
	die( 'Error: Make sure that Skype is installed in /Applications/Skype.app.' );
}


// Ensure that WorldFlags_16x11.png exists before we proceed
$worldflags_file = $emoticons_folder . '/WorldFlags_16x11.png';
if( !is_file( $worldflags_file ) ){
	die( 'Error: Unable to find WorldFlags_16x11.png.' );
}


// Make a temporary copy of their emoticon plist, and convert it from Binary to JSON
echo '- Copying emoticons.plist from Skype.app and converting to JSON...' . PHP_EOL;
if( !is_file( $emoticons_plist ) ){
	die( 'Error: Unable to find Emoticons.plist.' );
}
copy( $emoticons_plist, $emoticons_tmp_json );
exec( 'plutil -convert json "' . $emoticons_tmp_json . '"' );

// Load the emoticon JSON and delete the temporary file
echo '- Loading emoticons.plist as JSON...' . PHP_EOL;
$emoticons_json = json_decode( file_get_contents( $emoticons_tmp_json ), TRUE );
unlink( $emoticons_tmp_json );
if( NULL === $emoticons_json ){
	die( 'Error: Unable to load Emoticons.plist as JSON.' );
}


// Loop through all emoticons and build a sorted list of the internal names and what file they're contained in
echo '- Building internal list of valid Skype emoticons...' . PHP_EOL;
$emolist = array();
foreach( $emoticons_json['Emoticons'] as $emo ){
	// all we need to know is the internal id, and the static filename
	// note: the internal id is *not* the same as the typed shortcut; we only care about the id, since that's what gets stored in the Skype database to identify the emoticon
	$emo_id = $emo['Identifier']; // smile
	$emo_static = $emo['Destinations']['Static']; // smile (currently they use the same filename as the identifier, but that may change in the future, which is why we grab the static filename here)
	
	// make sure there's no emoticon with that id already saved (that should never happen, since Skype wouldn't mess that up)
	if( array_key_exists( $emo_id, $emolist ) ){
		die( 'Error: Duplicate emoticons found: "' . $emo_id . '".' );
	}
	
	// store the emoticon id together with the proper filename for the static image
	$emolist[$emo_id] = $emo_static . '.png';
}
ksort( $emolist );


// Now make sure that every image file exists and that they are all the exact same dimensions
// Note: Dimensions should all be 20x20, but may change in the future
echo '- Checking that all emoticons exist and have identical dimensions...' . PHP_EOL;
$emo_width = -1;
$emo_height = -1;
foreach( $emolist as $emo_id => $emo_file ){
	$emo_filepath = $emoticons_folder . '/' . $emo_file;
	if( !is_file( $emo_filepath ) ){
		die( 'Error: Missing emoticon "' . $emo_filepath . '".' );
	}
	
	// get the emoticon size
	$size = getimagesize( $emo_filepath );
	
	// if this is the first encountered image, use it to initialize the expected emoticon size
	if( $emo_width == -1 ){ $emo_width = $size[0]; }
	if( $emo_height == -1 ){ $emo_height = $size[1]; }
	
	// verify that the emoticon matches the expected size
	if( $size[0] != $emo_width || $size[1] != $emo_height ){
		die( 'Error: Emoticon dimensions are invalid "' . $emo_filepath . '".' );
	}
}


// Output current statistics
echo '- Discovered ' . count( $emolist ) . ' emoticons with ' . $emo_width . 'x' . $emo_height . 'px dimensions...' . PHP_EOL;


// Generate a truecolor (alpha capable) canvas which can hold all images, including a default "red block" image for future/missing emoticons
$canvas_width = $emo_width * ( 1 + count( $emolist ) ); // +1 is for the "red" default block at the start of the sprite
$canvas_height = $emo_height;
echo '- Generating a blank ' . $canvas_width . 'x' . $canvas_height . 'px sprite canvas...' . PHP_EOL;
$canvas = imagecreatetruecolor( $canvas_width, $canvas_height );
if( FALSE === $canvas ){
	die( 'Error: Unable to create canvas...' );
}
imagesavealpha( $canvas, TRUE ); // set the flag to save full alpha channel information (as opposed to single-color transparency) when saving PNG images based on this canvas
$transparent_color = imagecolorallocatealpha( $canvas, 0, 0, 0, 127 ); // allocates a color with complete transparency
imagefill( $canvas, 0, 0, $transparent_color ); // fill the entire blank canvas with the transparent color
$red_color = imagecolorallocate( $canvas, 255, 0, 0 ); // this is the red "warning" block that will be used for any missing images
imagefilledrectangle( $canvas, 0, 0, $emo_width - 1, $emo_height - 1, $red_color ); // make a red rectangle the size of an emoticon (we subtract 1 since the destination offsets are zero-indexed)


// Fill the canvas with all emoticons from left to right, while simultaneously generating the appropriate CSS string
echo '- Filling sprite canvas with all emoticons and generating their CSS...' . PHP_EOL;
$canvas_css_lines = array();
$canvas_x_offset = $emo_width; // start at the 21st pixel (immediately to the right of the red box)
foreach( $emolist as $emo_id => $emo_file ){
	// load the emoticon from disk
	$emo_filepath = $emoticons_folder . '/' . $emo_file;
	$emo_im = imagecreatefrompng( $emo_filepath );
	
	// write the png onto the canvas (preserves any alpha transparency)
	$success = imagecopy( $canvas, $emo_im, $canvas_x_offset, 0, 0, 0, $emo_width, $emo_height );
	if( FALSE === $success ){
		die( 'Error: Unable to copy image data from "' . $emo_filepath . '".' );
	}
	imagedestroy( $emo_im ); // free memory used by the loaded emoticon
	
	// add the image and its offset to the CSS string
	$canvas_css_lines[] = 'div.ss.' . $emo_id . ' {background-position:-' . $canvas_x_offset . 'px 0}';
	
	// update the offset so that the next image writes to the next location
	$canvas_x_offset += $emo_width;
}


// Analyze the latest WorldFlags file and generate the CSS for each flag
echo '- Analyzing WorldFlags_16x11.png and generating CSS for all valid flags...' . PHP_EOL;
$flag_css_lines = array();
function img_is_black( &$gdimg ){
	$sx = imagesx( $gdimg ); $sy = imagesy( $gdimg );
	for( $x=0; $x<$sx; $x++ ){
		for( $y=0; $y<$sy; $y++ ){
			if( 0 != imagecolorat( $gdimg, $x, $y ) ){
				return FALSE;
			}
		}
	}
	return TRUE;
}
// each flag is 16x11 pixels. the first, empty spacer to the left is 16 pixels wide, and the empty spacer at the top is 11 pixels tall.
$flagimg = imagecreatefrompng( $worldflags_file );
$letters = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ';
foreach( range('A','Z') as $code1 ){
	foreach( range('A','Z') as $code2 ){
		// skype logs use lowercase flag-ids, so we might aswell make ours lowercase to skip conversion
		$countrycode = strtolower( $code1 . $code2 );
		
		// skip the "aa" and "zz" test icons (they're just "?" symbols); there are a few other test icons in there, such as phone icons and such, but we won't skip those since they may be used by countries in the future
		if( $countrycode == 'aa' || $countrycode == 'zz' ){ continue; }
		
		// calculate the pixel offset to the current flag
		$currentflag_x_offset = (1 + strpos( $letters, $code1 )) * 16;
		$currentflag_y_offset = (1 + strpos( $letters, $code2 )) * 11;
		
		// figure out if there's a flag at the current offset: create a temprary image with a black background and copy the flag to it; if the image remains entirely black we know there's no flag
		$tmpimg = imagecreatetruecolor( 16, 11 ); // has black background
		imagecopy( $tmpimg, $flagimg, 0, 0, $currentflag_x_offset, $currentflag_y_offset, 16, 11 );
		$has_flag = ( !img_is_black( $tmpimg ) );
		imagedestroy( $tmpimg );
		
		// if this flag is valid, save the country code and the image offset
		if( $has_flag ){
			$flag_css_lines[] = 'div.flag.' . $countrycode . ' {background-position:-' . $currentflag_x_offset . 'px -' . $currentflag_y_offset . 'px}';
		}
	}
}
imagedestroy( $flagimg );


// Save the emoticons and worldflags images to disk
echo '- Writing emoticons.png and worldflags_16x11.png to disk...' . PHP_EOL;
$success = imagepng( $canvas, 'original/images/emoticons.png' );
if( !$success ){
	die( 'Error: Failed to write to emoticons.png.' );
}
$success = copy( $worldflags_file, 'original/images/worldflags_16x11.png' );
if( !$success ){
	die( 'Error: Failed to write to worldflags_16x11.png.' );
}


// Load the current style.css and inject all changes
echo '- Loading style.css and injecting new emoticons and world flags...' . PHP_EOL;
$style_css = file_get_contents( 'original/style.css' );
// update emoticon base definition using the latest width and height
$style_css = preg_replace( '/(div\.ss \{[^\}]*?width: )\d+(px[^\}]*?height: )\d+/', '${1}' . $emo_width . '${2}' . $emo_height, $style_css );
// remove all old emoticon definitions and replace them with the new ones
$style_css = preg_replace( '/(div\.ss\.\S+\s+\{background-position:[^;\}]+?\}\r\n)+/', '<%INJECT_EMOTICONS%>', $style_css );
$style_css = str_replace( '<%INJECT_EMOTICONS%>', implode( "\r\n", $canvas_css_lines ) . "\r\n", $style_css ); // note: we use windows line endings since style.css uses those
// remove all old world flag definitions and replace them with the new ones
$style_css = preg_replace( '/(div\.flag\.\S+\s+\{background-position:[^;\}]+?\}\r\n)+/', '<%INJECT_FLAGS%>', $style_css );
$style_css = str_replace( '<%INJECT_FLAGS%>', implode( "\r\n", $flag_css_lines ) . "\r\n", $style_css ); // windows line endings again


// Carefully save the new stylesheet in a two-step process to avoid corruption in case of unfinished writes
echo '- Writing new style.css to disk...' . PHP_EOL;
$success = file_put_contents( 'original/style_new.tmp', $style_css );
if( FALSE === $success ){
	die( 'Error: Unable to write to style_new.tmp.' );
}
$success = rename( 'original/style_new.tmp', 'original/style.css' );
if( FALSE === $success ){
	die( 'Error: Unable to rename style_new.tmp to style.css.' );
}


// Complete!
echo PHP_EOL . 'The updated emoticons.png, worldflags_16x11.png and style.css have been written to disk. You *must* now run build_smushed_images.sh to compress the images, and then finish with build_compressed_css.sh which embeds the smushed images directly into the compressed CSS and creates the necessary C++ headers.' . PHP_EOL . PHP_EOL;

?>