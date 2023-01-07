#!/bin/bash
# install.sh
#--------------------------------------------------------------
#
# This program is the installation of ioCoro
#  (c) Author: wyther yang
#
#  github: https://github.com/wythers/ioCoro
#
#----------------------------------------------------------------------
#

# make
GLOBAL_BUILD="make"

# gcc 
GLOBAL_COMPILER="g++"

# 
GLOBAL_DEBIAN=$(which apt)

#
GLOBAL_APTGET="sudo apt install"

#
GLOBAL_GXX_VERSION="g++-12"

#
Usage() {
        echo "Usage:"
        echo "You must install make in the Linux environment."
        echo "Meanwhile, your GCC compiler must have the functions of -std=c++20 and -fcoroutines."
}

# "main" here

which ${GLOBAL_BUILD} 1>/dev/null

if [ "$?" -ne "0" ]
then
        if [ -n "${GLOBAL_DEBIAN}" ]
        then
                ${GLOBAL_APTGET} ${GLOBAL_BUILD}
        else
                Usage
                exit 1
        fi
fi

${GLOBAL_BUILD}

if [ "$?" -ne "0" ]
then
        ${GLOBAL_APTGET} ${GLOBAL_GXX_VERSION} 2>/dev/null

        GLOBAL_GXX=$(which ${GLOBAL_GXX_VERSION})
        if [ ! -n "${GLOBAL_GXX}" ]
        then
                Usage
                exit 1
        else
                echo "alias ${GLOBAL_COMPILER}='${GLOBAL_GXX}'" >> ~/.bashrc

                ${GLOBAL_BUILD} ARG1=${GLOBAL_GXX}
                echo "--- Please, type the following source command,
                Use '. ~/.bashrc' to complete env update."
        fi
fi

exit $?
