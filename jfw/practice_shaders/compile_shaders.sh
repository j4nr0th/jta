#!/bin/bash

for filename in *; do
  if [[ $filename == *.glsl ]] ; then
    echo "file: ${filename}"
    output_name=$(basename "$filename")
    output_name="${output_name%%.*}"
    echo "$output_name"
    glslangValidator -V --vn "$output_name" "$filename"
  fi
done

