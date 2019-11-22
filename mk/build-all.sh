#!/bin/sh -e

gnarl_dir=$(readlink -f $(dirname $0)/..)
echo "GNARL top level: $gnarl_dir"

(cd $IDF_PATH && echo "ESP-IDF version: $(git describe --abbrev=0)")

projects=$(basename -a $gnarl_dir/src/*)

nice_make() {
    ionice -c3 nice make -j
}

nice_cmake() {
    ionice -c3 nice idf.py build
}

build() {
    proj="$1"
    kind="$2"
    logfile=$kind.$proj.log
    rm -fr project
    echo -n "Building $proj with $kind ... "
    if (make $proj && cd project && nice_$kind) > $logfile 2>&1 ; then
	echo "OK"
    else
	echo "FAILED"
	echo "See $logfile for details"
    fi
}

for p in $projects; do
    build $p make
    build $p cmake
done
