#!/bin/bash

make JDK=/usr/lib/jvm/java-1.7.0-openjdk-amd64/ OSNAME=linux clean
make JDK=/usr/lib/jvm/java-1.7.0-openjdk-amd64/ OSNAME=linux all
make JDK=/usr/lib/jvm/java-1.7.0-openjdk-amd64/ OSNAME=linux plugins
make JDK=/usr/lib/jvm/java-1.7.0-openjdk-amd64/ OSNAME=linux installplugins
