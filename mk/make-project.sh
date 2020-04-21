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

cp -a ../include .
cp -a ../$src main
mv main/sdkconfig .

sed "s/xyzzy/$project/" < ../mk/Makefile > Makefile
sed "s/xyzzy/$project/" < ../mk/CMakeLists.project > CMakeLists.txt
cp ../mk/partitions.csv .
cp ../mk/CMakeLists.main main/CMakeLists.txt
cp ../mk/component.mk main/component.mk

# Handle a special "network" pseudo-component to allow choosing
# between Bluetooth tethering and WiFi (currently mutually exclusive).
handle_network_component() {
    if egrep -q '^#define\s+USE_BLUETOOTH_TETHERING\b' include/network_config.h ; then
	echo "Using Bluetooth tethering."
	cp -a ../lib/tether components
    else
	echo "Using WiFi networking."
	cp -a ../lib/wifi components
    fi
}

# Create dummy btstack component to replace the existing one, if present.
make_dummy_btstack() {
    mkdir components/btstack
    touch components/btstack/CMakeLists.txt
    touch components/btstack/component.mk
}

mkdir components
if [ -f main/dependencies ]; then
    for dep in $(cat main/dependencies); do
	if [ "$dep" = network ]; then
	    # special case
	    handle_network_component
	else
	    cp -a ../lib/$dep components
	fi
    done
fi
if [ ! -d components/tether ]; then
    make_dummy_btstack
fi

echo The $project project has been set up in the '"project"' subdirectory.
