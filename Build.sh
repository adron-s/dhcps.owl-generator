#!/bin/sh

#GCC=gcc
#OBJCOPY=objcopy

GCC=arm-linux-gnueabihf-gcc-4.7
OBJCOPY=arm-linux-gnueabihf-objcopy

$GCC ./templates.c -c
$GCC -static ./dhcps_owlgen.c ./templates.o -o ./dhcps_owlgen
$OBJCOPY --strip-all ./dhcps_owlgen ./dhcps_owlgen
