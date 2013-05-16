#!/bin/bash

for file in $(ls *.png); do
	echo -e "data:image/png;base64,\c" > "enc_${file}.txt"
	cat $file | openssl enc -base64 | tr -d '\n' >> "enc_${file}.txt"
done