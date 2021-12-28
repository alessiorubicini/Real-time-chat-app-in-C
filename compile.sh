#!/bin/sh

echo 'Compiling server.c'
gcc server.c -o server -pthread

echo 'Compiling client.c'
gcc client.c -o client -lcurses -pthread -w

echo 'Compiled ✓'