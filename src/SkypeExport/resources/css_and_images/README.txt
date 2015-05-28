Here is how to update the CSS and images used by the pages:


Step 1: Edit original/style.css, and also tweak images if required. Make sure that any image tweaks are done in "original/images", NOT in the "images_smushed" folder!


Step 2: Compressing any new/modified images to make them smaller:

If you add or modify any images, you must prepare them by making them even smaller:

Run build_smushed_images.sh to crunch all images through Google's ZopfliPNG tool. It optimizes the PNG binary layout without losing any data whatsoever. The process saved ~27kb in total on all the currently included images, leading to a leaner embedded stylesheet size.


Step 3: Compiling the original/style.css file to a compressed stylesheet with all smushed images embedded as Base64 data:

Simply run build_compressed_css.sh, and it will spit out two files: style_compact_data.css (the final, compressed stylesheet) and style_compact_data_css.h (the header-version of the file, which will automatically be embedded in the final binary upon compilation). Just leave those files exactly where they end up, and compile the project to get the new CSS embedded into the binary.

