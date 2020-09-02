#!/bin/bash

shopt -s globstar

files_with_non_ascii=0
for file in . ./* ./**/* ; do
  if [ ! -d "$file" ]; then
    echo "Validating that $file file only contains ASCII bytes..."
    if grep -I --color='auto' -P -n "[^\x00-\x7F]" $file ; then
      ((files_with_non_ascii++))
    fi
  fi
done

echo Files found with non-ASCII characters: $files_with_non_ascii
exit $files_with_non_ascii