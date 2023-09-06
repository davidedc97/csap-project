#!/bin/bash

#Root path
ROOT_PATH="/tmp/local" 

#Set the allowed binaries here
APPS="/bin/bash /usr/bin/rm /usr/bin/su /bin/cp /bin/mv /usr/bin/rm /bin/ls /usr/bin/mkdir /usr/bin/cat /usr/bin/sha256sum"

PATH=/usr/bin:/bin

chmod 777 $ROOT_PATH
cd $ROOT_PATH
mkdir -p dev
mkdir -p bin
mkdir -p lib64
mkdir -p etc
mkdir -p usr/bin
mkdir -p usr/lib64

if [ -e "/lib64/libnss_files.so.2" ]
then
 cp -p /lib64/libnss_files.so.2 ${ROOT_PATH}/lib64/libnss_files.so.2
fi

if [ -e "/lib/x86_64-linux-gnu/libnss_files.so.2" ]
then
  mkdir -p ${ROOT_PATH}/lib/x86_64-linux-gnu
  cp -p /lib/x86_64-linux-gnu/libnss_files.so.2 ${ROOT_PATH}/lib/x86_64-linux-gnu/libnss_files.so.2
fi

for prog in $APPS
do
  cp $prog ${ROOT_PATH}${prog} 
  if ldd $prog > /dev/null
  then
    LIBS=`ldd $prog | grep '/lib' | sed 's/\t/ /g' | sed 's/ /\n/g' | grep "/lib"`
    for l in $LIBS
    do
      mkdir -p ./`dirname $l` > /dev/null 2>&1
      cp $l ./$l  > /dev/null 2>&1
    done
  fi
done
