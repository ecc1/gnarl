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

# project/build directory will be owned by root if using docker
if [ -d project/build ] && [ $(stat --format=%U project/build) = root ]; then
    sudo rm -fr project/build
fi
rm -fr project

mkdir project
cd project

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

cd main
[ -f component.mk ] || cp ../../mk/component.mk component.mk
[ -f CMakeLists.txt ] || cp ../../mk/CMakeLists.main CMakeLists.txt

echo The $project project has been set up in the '"project"' subdirectory.
