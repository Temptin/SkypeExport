Here is how to update the CSS and images used by the pages:


Step 1: If Skype has released new/changed emoticons or world flags, simply run 01_build_spritesheets.sh on a Mac computer to automatically generate Emoticon and WorldFlags sprite sheets as well as injecting the finished CSS into style.css. There is no danger in running this script multiple times, since it accurately injects any emoticon/flag changes, and results in the same style.css file if there were no changes.

Warning: This script requires PHP with both GD and PNG support. As of OS X Yosemite, the bundled PHP no longer contains any PNG support. Therefore, the script looks for the Homebrew version of PHP instead. Open the script in a text editor and read the top comment to see how to install homebrew, if this applies to you. However, if you've already got an installed PHP version which supports GD/PNG, then you can try running the script directly via "php 01_build_spritesheets.sh", and it will run perfectly if PNG support is enabled; otherwise it will simply warn you, and you'll have to install the Homebrew version.


Step 2: To change the general html design, edit original/style.css and also feel free to add/tweak any "original/images" PNG files (except for the auto-generated emoticons.png and worldflags_16x11.png). Do NOT touch "original/images_smushed"!


Step 3: Compressing any new/modified images to make them smaller:

If you add or modify any images (even via the build_spritesheets script), you MUST now prepare them by making them even smaller:

Run 02_build_smushed_images.sh to crunch all images through Google's ZopfliPNG tool. It optimizes the PNG binary layout without losing any visible data whatsoever. The process saved ~19kb in total on all the currently included images, leading to a leaner embedded stylesheet size.

Warning: Not running the optimizer script means that the new images will not be included in the output.


Step 4: Compiling the original/style.css file to a compressed stylesheet with all smushed images embedded as Base64 data:

Simply run 03_build_compressed_css.sh, and it will spit out two files: style_compact_data.css (the final, compressed stylesheet with embedded images) and style_compact_data_css.h (the header-version of the file, which will automatically be embedded in the final binary upon compilation). Just leave those files exactly where they end up, and compile the project to get the new CSS embedded into the binary.

