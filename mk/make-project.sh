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
sudo rm -r project/build
rm -fr project
mkdir project
cd project

cp -a ../components .
cp -a ../include .
cp -a ../lib .
sed "s/xyzzy/$project/" < ../mk/Makefile > Makefile
sed "s/xyzzy/$project/" < ../mk/CMakeLists.project > CMakeLists.txt
cp ../mk/sdkconfig .
cp ../mk/partitions.csv .

mkdir main
cd main
cp -av ../../$src/* .
[ -f component.mk ] || cp -v ../../mk/component.mk component.mk
[ -f CMakeLists.txt ] || cp -v ../../mk/CMakeLists.main CMakeLists.txt

cd ..
[ -f main/config.patch ] && patch -p1 < main/config.patch

cd ..

echo Initial setup of $project project is complete.
echo 'Now change to the "project" subdirectory and run "make" or "idf.py build".'
