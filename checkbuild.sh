#!/bin/bash

make clean && make urltest && valgrind --leak-check=full ./urltest || { echo 'Unit tests failed' ; exit 1 ; }
scan-build -v -V make && valgrind --leak-check=full ./httpget -u http://www.w3.org/Protocols/rfc2616/rfc2616.html

