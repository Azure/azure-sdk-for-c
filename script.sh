#!/bin/bash

shopt -s globstar

for f in /sdk/* sdk/**/* ; do
  echo "Processing $f file..."
  #hexdump -C $f > temp.txt
  grep --color='auto' -P -n "[^\x00-\x7F]" $f
done;
