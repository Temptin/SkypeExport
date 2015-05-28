#!/bin/bash

# The following ZopfliPNG settings, particularly the filters, are tuned
# with our particular workload in mind. Don't use them with richer
# images that contain more colors and might benefit from other filters!

mkdir -p "original/images_smushed"
for IMGFILE in original/images/*.png; do
	IMGNAME=$(basename "${IMGFILE}")
	./external_tools/zopflipng-e17d185 --iterations=50 --splitting=3 --filters=0m --lossy_8bit "${IMGFILE}" "original/images_smushed/${IMGNAME}"
done
