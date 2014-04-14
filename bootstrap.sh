#!/bin/sh

# check to see if we have the submodules checked out
if [ ! -f http-parser/http_parser.h ]; then
    git submodule init
    git submodule update
fi

autoreconf -i -f
echo
echo "Now run ./configure [OPTIONS]."
