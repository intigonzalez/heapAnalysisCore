#!/bin/bash

JDK_PATH=/usr/lib/jvm/java-7-oracle/

make JDK=${JDK_PATH} OSNAME=linux clean
make JDK=${JDK_PATH} OSNAME=linux all
make JDK=${JDK_PATH} OSNAME=linux plugins
make JDK=${JDK_PATH} OSNAME=linux installplugins
