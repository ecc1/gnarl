#!/bin/sh -e

project="$1"
src="apps/$project"

if [ ! -d "$src" ]; then
    echo "project source directory $src not found"
    exit 1
fi

project_is_current() {
    [ -d project ] && \
    [ -f project/Makefile ] && \
    grep ^PROJECT_NAME project/Makefile | grep -q $project
}

create_project() {
    rm -fr project
    mkdir project
    cd project

    ln -s ../components .
    sed "s/xyzzy/$project/" < ../mk/Makefile > Makefile
    cp ../mk/sdkconfig .

    mkdir main
    cd main
    > component.mk
    ln -sv ../../$src/* .

    cd ../..
}

if ! project_is_current; then
    create_project
fi

echo Initial setup of $project project is complete.
echo Now change to \"project\" subdirectory and run make with desired flags.
