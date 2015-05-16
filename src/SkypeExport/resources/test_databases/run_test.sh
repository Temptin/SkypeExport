#!/bin/bash

for DB in *.db; do
	DB=$(echo "${DB}" | sed "s/\.db$//")
	../../_gccbuild/release/SkypeExport --db "${DB}.db" --outpath "./output/${DB}"
done
