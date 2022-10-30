#!/bin/bash

echo "#ifndef  __K_SYMTABLE"
echo "#define  __K_SYMTABLE"
echo ""
echo "#include <stdint.h>"
echo ""
echo "typedef struct symbol{"
echo "  const char* name;"
echo "  uint32_t addr;"
echo "}symbol_t;"
echo ""
echo "symbol_t symbols[] = {"
readelf -s --wide kernel.bin | awk '/exported/{print "\t{.addr=0x" $2  ", .name=" "\"" substr($8, 3, length($8) - 11) "\"" "},"}'
echo "};"
echo "#endif"

