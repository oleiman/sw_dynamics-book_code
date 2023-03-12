#!/bin/bash

filename=$(basename -- ${1%.*})

pandoc -V geometry:margin=1.5in --from markdown+tex_math_dollars+raw_tex+escaped_line_breaks+pandoc_title_block --to latex --toc --lua-filter=""$HOME/co/lua-filters/diagram-generator/diagram-generator.lua"" $1  -o "$filename.pdf"
