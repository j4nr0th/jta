#!/bin/bash

for filename in *; do
  if [[ $filename == *.vert ]]; then
    echo "file: ${filename}"
    output_name=$(basename "$filename")
    output_name="${output_name%%.*}"
    echo "$output_name"
    glslang -V --vn "$output_name" "$filename"
    mv "vert.spv" "$output_name.spv"
  elif [[ $filename == *.frag ]]; then
     echo "file: ${filename}"
     output_name=$(basename "$filename")
     output_name="${output_name%%.*}"
     echo "$output_name"
     glslang -V --vn "$output_name" "$filename"
     mv "frag.spv" "$output_name.spv"
  elif [[ $filename == *.geom ]]; then
     echo "file: ${filename}"
     output_name=$(basename "$filename")
     output_name="${output_name%%.*}"
     echo "$output_name"
     glslang -V --vn "$output_name" "$filename"
     mv "geom.spv" "$output_name.spv"
  fi
done

