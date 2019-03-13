#!/bin/sh -e

gnarl_dir=$(readlink -f $(dirname $0)/..)
echo "GNARL top level: $gnarl_dir"

projects=$(basename -a $gnarl_dir/src/*)

nice_make() {
    ionice -c3 nice make -j
}

nice_cmake() {
    ionice -c3 nice idf.py build
}

for p in $projects; do
    echo "Building $p with make"
    logfile=make.$p.log
    rm -fr project
    (make $p && cd project && nice_make) > $logfile 2>&1

    echo "Building $p with CMake"
    logfile=cmake.$p.log
    rm -fr project
    (make $p && cd project && nice_cmake) > $logfile 2>&1
done
