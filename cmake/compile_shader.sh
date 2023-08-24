#!/bin/sh

# $1 - Variable name
# $2 - Env (eg. vulkan1.0, opengl)
# $3 - Output file
# $4 - Input file

if command -v glslangValidator;
then
  rm -f "$3.c"
  (glslangValidator --vn "$1" --target-env "$2"  -o "$3.c" "$4" && \
  sed -i "s/pragma once/include \"$3.h\"/g" "$3.c") || \
  (echo "Compilation failed" && exit)
elif command -v glslc; then
  ADDITION="\#include \\\"$3.h\\\"\nconst uint32_t $1[] = "
  rm -f "$3.c"
  (glslc -mfmt=c --target-env="$2" -o "$3.c" "$4" && \
  sed -i "1s/^/$ADDITION/" "$3.c" && \
  echo ";" >> "$3.c") || \
  (echo "Compilation failed" && exit)
else
  echo "glslc nor glslangValidator was found. Can't compile shaders"
  exit
fi

echo "const uint64_t $1_size = sizeof($1);" >> "$3.c"
{
  echo "#ifndef $1_H" && \
  echo "#define $1_H" && \
  echo "#include <stdint.h>" && \
  echo "extern const uint32_t $1[];" && \
  echo "extern const uint64_t $1_size;" && \
  echo "#endif";
} > "$3.h"
