#!/bin/bash

for DB in *.db; do
	DB=$(echo "${DB}" | sed "s/\.db$//")
	../../_gccbuild/macosx/release/SkypeExport --db "${DB}.db" --outpath "./output/${DB}"
done
