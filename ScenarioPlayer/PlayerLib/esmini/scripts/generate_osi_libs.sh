#!/bin/bash

# 
# This script will build Protobuf and Open Simulation Interface (OSI) libraries  
# needed for esmini to support OSI and output Ground Truth messages.
# No system installations will be done (no admin rights required)
#
# The ambition is to support Windows, Linux and Mac. However the script is under
# development and has not been tested as is on all three platforms.
#
# Prerequisites:
# - Git (with Bash) (https://git-scm.com/download/win)
# - cmake (https://cmake.org/download/)
# - Visual Studio (with C++ toolkit) (https://visualstudio.microsoft.com/downloads/)
#
# Usage: 
# - put this script in an empty folder
# - open Git Bash in that folder 
# - review and adjust system dependent parameters in section below
# - run the script: ./generate_osi_libs.sh
# - wait (the build process will take approx. 15 minutes depending on...)
# 
# The folder osi-visualizer/build/Release (or Debug) will contain everything needed
# 
#

# -----------------------------------------------------------------------------------
# Review and update settings in this section according to your system and preferences

if [ "$OSTYPE" == "msys" ]; then
	# Visual Studio 2019 - toolkit from Visual Studio 2017
	GENERATOR=("Visual Studio 16 2019")
	GENERATOR_TOOLSET="v141"
	GENERATOR_ARGUMENTS="-A x64 -T ${GENERATOR_TOOLSET}"

	# Visual Studio 2019 - default toolkit
	# GENERATOR=("Visual Studio 16 2019")
	# GENERATOR_ARGUMENTS="-A x64 -T ${GENERATOR_TOOLSET}"

	# Visual Studio 2017 - default toolkit
	# GENERATOR=("Visual Studio 15 2017 Win64")
	# GENERATOR_ARGUMENTS="-T ${GENERATOR_TOOLSET}"
elif [ "$OSTYPE" == "darwin"* ] || [ "$OSTYPE" == "linux-gnu"* ]; then
	# Unix Makefiles (for Ubuntu and other Linux systems)
	GENERATOR=("Unix Makefiles")
	GENERATOR_ARGUMENTS=""
else
	echo Unknown OSTYPE: $OSTYPE
fi


PROTOBUF_VERSION=v3.11.4

# ---------------------------------------------------------------------------------------
# From this point no adjustments should be necessary, except fixing bugs in the script :)
# However you might want to adjust versions of software packages being checkout and built


osi_root_dir=$(pwd)

echo ------------------------ Installing zlib ------------------------------------
cd $osi_root_dir

if [ ! -d zlib-1.2.11 ] 
then
 	if [ ! -f zlib1211.zip ]; then
  	    curl "https://zlib.net/zlib1211.zip" -o zlib1211.zip
    fi
    unzip zlib1211.zip
    cd zlib-1.2.11
    mkdir install
    mkdir build
    cd build


	if [ "$OSTYPE" == "linux-gnu"* ]; then
		cmake -G "${GENERATOR[@]}" ${GENERATOR_ARGUMENTS} -D CMAKE_INSTALL_PREFIX=../install -DCMAKE_BUILD_TYPE=Debug .. -DCMAKE_C_FLAGS="-fPIC" 
		cmake --build . --target install
		mv ../install/lib/zlib.a ../install/lib/zlibd.a

		rm CMakeCache.txt
		cmake -G "${GENERATOR[@]}" ${GENERATOR_ARGUMENTS} -D CMAKE_INSTALL_PREFIX=../install -DCMAKE_BUILD_TYPE=Release .. -DCMAKE_C_FLAGS="-fPIC" 
		cmake --build . --target install
	elif [ "$OSTYPE" == "darwin"* ]; then
		cmake -G "${GENERATOR[@]}" ${GENERATOR_ARGUMENTS} -D CMAKE_INSTALL_PREFIX=../install -DCMAKE_BUILD_TYPE=Release .. -DCMAKE_C_FLAGS="-fPIC" 
		cmake --build . --target install
	else
		cmake -G "${GENERATOR[@]}" ${GENERATOR_ARGUMENTS} -D CMAKE_INSTALL_PREFIX=../install ..
		cmake --build . --config Debug --target install
		cmake --build . --config Release --target install --clean-first
	fi

else
    echo zlib folder already exists, continue with next step...
fi

echo ------------------------ Installing Protobuf ------------------------------------
cd $osi_root_dir

if [ ! -d protobuf ] 
then
    git clone https://github.com/protocolbuffers/protobuf.git
    cd protobuf
    git checkout PROTOBUF_VERSION
    mkdir build-code; cd build-code
    if [ "$OSTYPE" == "linux-gnu"* ] || [ "$OSTYPE" == "darwin"* ]; then
        ZLIB_FILE_RELEASE=zlib.a
		ZLIB_FILE_DEBUG=zlibd.a
    elif [[ "$OSTYPE" == "msys" ]]; then
        ZLIB_FILE_RELEASE=zlib.lib
		ZLIB_FILE_DEBUG=zlibd.lib
    fi

    if [ "$OSTYPE" != "darwin"* ]; then
        if [ "$OSTYPE" == "msys" ]; then
            echo Kalle
            cmake ../cmake -G "${GENERATOR[@]}" ${GENERATOR_ARGUMENTS} -DZLIB_LIBRARY=../../zlib-1.2.11/install/lib/$ZLIB_FILE_DEBUG -DZLIB_INCLUDE_DIR=../../zlib-1.2.11/install/include -DCMAKE_INSTALL_PREFIX=../protobuf-install -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_WITH_ZLIB=ON -Dprotobuf_MSVC_STATIC_RUNTIME=OFF -DCMAKE_BUILD_TYPE=Debug 
        else
            cmake ../cmake -G "${GENERATOR[@]}" ${GENERATOR_ARGUMENTS} -DZLIB_LIBRARY=../../zlib-1.2.11/install/lib/$ZLIB_FILE_DEBUG -DZLIB_INCLUDE_DIR=../../zlib-1.2.11/install/include -DCMAKE_INSTALL_PREFIX=../protobuf-install -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_WITH_ZLIB=ON -Dprotobuf_MSVC_STATIC_RUNTIME=OFF -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fPIC" 
        fi

        cmake --build . --config Debug --target install --clean-first  
    fi

    rm CMakeCache.txt
    if [ "$OSTYPE" == "msys" ]; then
        cmake ../cmake -G "${GENERATOR[@]}" ${GENERATOR_ARGUMENTS} -DZLIB_LIBRARY=../../zlib-1.2.11/install/lib/$ZLIB_FILE_RELEASE -DZLIB_INCLUDE_DIR=../../zlib-1.2.11/install/include -DCMAKE_INSTALL_PREFIX=../protobuf-install -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_WITH_ZLIB=ON -Dprotobuf_MSVC_STATIC_RUNTIME=OFF -DCMAKE_BUILD_TYPE=Release 
    else
        cmake ../cmake -G "${GENERATOR[@]}" ${GENERATOR_ARGUMENTS} -DZLIB_LIBRARY=../../zlib-1.2.11/install/lib/$ZLIB_FILE_RELEASE -DZLIB_INCLUDE_DIR=../../zlib-1.2.11/install/include -DCMAKE_INSTALL_PREFIX=../protobuf-install -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_WITH_ZLIB=ON -Dprotobuf_MSVC_STATIC_RUNTIME=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-fPIC"
    fi

    cmake --build . --config Release --target install --clean-first  

else
    echo protobuf folder already exists, continue with next step...
fi


echo ------------------------ Installing OSI proto2cpp -----------------------------------
cd $osi_root_dir

if [ ! -d proto2cpp ] 
then
    git clone https://github.com/OpenSimulationInterface/proto2cpp.git
else
    echo proto2cpp folder already exists, continue with next step...
fi


echo --------------------- Installing OSI -----------------------------
cd $osi_root_dir

if [ ! -d open-simulation-interface ] 
then
    git clone https://github.com/OpenSimulationInterface/open-simulation-interface.git
    cd open-simulation-interface
    sh ./convert-to-proto3.sh
    mkdir build
    cd build

    export INSTALL_ROOT_DIR=../install
    export INSTALL_OSI_LIB_DIR=$INSTALL_ROOT_DIR/osi-lib
    mkdir $INSTALL_ROOT_DIR
    mkdir $INSTALL_OSI_LIB_DIR
    mkdir $INSTALL_OSI_LIB_DIR/lib
    mkdir $INSTALL_OSI_LIB_DIR/include

    export PATH=$PATH:../../graphviz/release/bin:../../protobuf/protobuf-install/bin

    if [ "$OSTYPE" != "darwin"* ]; then
        echo stop
        cmake .. -G "${GENERATOR[@]}" ${GENERATOR_ARGUMENTS} -DCMAKE_INCLUDE_PATH=../protobuf/protobuf-install/include -DFILTER_PROTO2CPP_PY_PATH=../../proto2cpp -DINSTALL_LIB_DIR=$INSTALL_OSI_LIB_DIR/lib -DINSTALL_INCLUDE_DIR=$INSTALL_OSI_LIB_DIR/include -DCMAKE_INSTALL_PREFIX=$INSTALL_OSI_LIB_DIR -DCMAKE_VERBOSE_MAKEFILE=ON -DCMAKE_BUILD_TYPE=Debug -DCMAKE_LIBRARY_PATH=../protobuf/protobuf-install/lib .. 

        # First bild OSI submodule separately since we need to rename the library before linking with the application
        cmake --build . --config Debug --target install

       if [ "$OSTYPE" == "linux-gnu"* ]; then
   	    cp $INSTALL_OSI_LIB_DIR/lib/osi3/libopen_simulation_interface.so $INSTALL_OSI_LIB_DIR/lib/osi3/libopen_simulation_interfaced.so
	    mv $INSTALL_OSI_LIB_DIR/lib/osi3/libopen_simulation_interface_pic.a $INSTALL_OSI_LIB_DIR/lib/osi3/libopen_simulation_interface_picd.a
	    mv $INSTALL_OSI_LIB_DIR/lib/osi3/libopen_simulation_interface_static.a $INSTALL_OSI_LIB_DIR/lib/osi3/libopen_simulation_interface_staticd.a
	    touch $INSTALL_OSI_LIB_DIR/lib/osi3/kalle.txt
       elif [ "$OSTYPE" == "msys" ]; then 
            echo stop2
            mv $INSTALL_OSI_LIB_DIR/lib/osi3/open_simulation_interface.dll $INSTALL_OSI_LIB_DIR/lib/osi3/open_simulation_interfaced.dll 
    	    mv $INSTALL_OSI_LIB_DIR/lib/osi3/open_simulation_interface_pic.lib $INSTALL_OSI_LIB_DIR/lib/osi3/open_simulation_interface_picd.lib 
            mv $INSTALL_OSI_LIB_DIR/lib/osi3/open_simulation_interface_static.lib $INSTALL_OSI_LIB_DIR/lib/osi3/open_simulation_interface_staticd.lib
       fi
    fi

    rm CMakeCache.txt
    
    cmake .. -G "${GENERATOR[@]}" ${GENERATOR_ARGUMENTS} -DCMAKE_INCLUDE_PATH=../protobuf/protobuf-install/include -DFILTER_PROTO2CPP_PY_PATH=../../proto2cpp -DINSTALL_LIB_DIR=$INSTALL_OSI_LIB_DIR/lib -DINSTALL_INCLUDE_DIR=$INSTALL_OSI_LIB_DIR/include -DCMAKE_INSTALL_PREFIX=$INSTALL_OSI_LIB_DIR -DCMAKE_VERBOSE_MAKEFILE=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_LIBRARY_PATH=../protobuf/protobuf-install/lib -DCMAKE_CXX_STANDARD=11 .. 

    cmake --build . --config Release --target install --clean-first

else
    echo open-simulation-interface folder already exists, continue with next step...
fi

echo ------------------------ Done ------------------------------------
cd $osi_root_dir


