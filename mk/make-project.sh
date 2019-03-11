#!/bin/sh -e

project="$1"
src="src/$project"

if [ ! -d "$src" ]; then
    echo "project source directory $src not found"
    exit 1
fi

current_for_make() {
    [ -d project ] && \
    [ -f project/Makefile ] && \
    grep ^PROJECT_NAME project/Makefile | grep -q $project
}

current_for_cmake() {
    [ -d project ] && \
    [ -f project/CMakeLists.txt ] && \
    grep ^project project/CMakeLists.txt | grep -q $project
}

project_is_current() {
    current_for_make && current_for_cmake
}

create_project() {
    rm -fr project
    mkdir project
    cd project

    ln -s ../components .
    ln -s ../include .
    ln -s ../lib .
    sed "s/xyzzy/$project/" < ../mk/Makefile > Makefile
    sed "s/xyzzy/$project/" < ../mk/CMakeLists.project > CMakeLists.txt
    cp ../mk/sdkconfig .

    mkdir main
    cd main
    ln -sv ../../$src/* .
    cp ../../mk/component.mk component.mk
    cp ../../mk/CMakeLists.main CMakeLists.txt

    cd ../..
}

if ! project_is_current; then
    create_project
fi

echo Initial setup of $project project is complete.
echo 'Now change to the "project" subdirectory and run "make" or "idf.py build".'
