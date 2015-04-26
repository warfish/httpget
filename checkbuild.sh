#!/bin/bash

make clean && scan-build -v -V make && valgrind --leak-check=full ./httpget http://www.w3.org/Protocols/rfc2616/rfc2616.html 

