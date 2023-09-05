#!/bin/bash

#This script will set up the environment to be chrooted.

#Set your fake root path here
JAILPATH="/tmp/fakeroot" 

#Set the allowed binaries here
APPS="/usr/bin/bash /bin/grep /usr/bin/rm /usr/bin/id /usr/bin/su /bin/ls /bin/pwd /bin/mv /bin/cp /usr/bin/mkdir"

PATH=/usr/bin:/bin

chmod 777 $JAILPATH
cd $JAILPATH
mkdir -p dev
mkdir -p bin
mkdir -p lib64
mkdir -p etc
mkdir -p usr/bin
mkdir -p usr/lib64

if [ -e "/lib64/libnss_files.so.2" ]
then
 cp -p /lib64/libnss_files.so.2 ${JAILPATH}/lib64/libnss_files.so.2
fi

if [ -e "/lib/x86_64-linux-gnu/libnss_files.so.2" ]
then
  mkdir -p ${JAILPATH}/lib/x86_64-linux-gnu
  cp -p /lib/x86_64-linux-gnu/libnss_files.so.2 ${JAILPATH}/lib/x86_64-linux-gnu/libnss_files.so.2
fi

for prog in $APPS
do
  cp $prog ${JAILPATH}${prog} 
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
