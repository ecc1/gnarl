#!/bin/sh -e

project="$1"
src="src/$project"

if [ "$(basename $(pwd))" != gnarl ]; then
    echo "script must be run from the top-level gnarl directory"
    exit 1
fi

if [ ! -d "$src" ]; then
    echo "project source directory $src not found"
    exit 1
fi

mkdir -p project
cd project
rm -fr CMakeLists.txt component.mk components dependencies include main Makefile partitions.csv sdkconfig
if [ -d build ]; then
    # build directory will be owned by root if using docker
    if [ $(stat --format=%U build) = root ]; then
	sudo rm -fr build
    else
	rm -fr build
    fi
fi

cp -a ../$src main
mv main/sdkconfig .
if [ -f main/dependencies ]; then
    mv main/dependencies .
    mkdir components
    for dep in $(cat dependencies); do
	cp -a ../lib/$dep components
    done
fi
cp -a ../include .
sed "s/xyzzy/$project/" < ../mk/Makefile > Makefile
sed "s/xyzzy/$project/" < ../mk/CMakeLists.project > CMakeLists.txt
cp ../mk/partitions.csv .
cp ../mk/CMakeLists.main main/CMakeLists.txt
cp ../mk/component.mk main/component.mk

echo The $project project has been set up in the '"project"' subdirectory.
