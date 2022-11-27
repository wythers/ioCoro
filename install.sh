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
GLOBAL_BUILD=make

# gcc 
GLOBAL_COMPILER=g++

# 
GLOBAL_DISTRI=$(lsb_release -a 2>/dev/null | grep "Ubuntu")

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
        if [ -n "${GLOBAL_DISTRI}" ]
        then
                sudo apt install ${GLOBAL_BUILD}
        else
                Usage

                exit 1
        fi
fi

${GLOBAL_BUILD}

if [ $? -ne "0" ]
then
        Usage

        exit 1
fi

exit 0
