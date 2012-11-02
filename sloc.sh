#!/bin/sh
find . -name '*.cpp' -o -name '*.h' | xargs wc -l
