# Ignition transport

** Ignition transport classes and functions for robot applications.**

Ignition transport is a component in the ignition framework, a set
of libraries designed to rapidly develop robot applications.

  [http://ignitionrobotics.org](http://ignitionrobotics.org)

## Continuous integration

Please refer to the [drone.io
job](https://drone.io/bitbucket.org/ignitionrobotics/ign_math).

[![Build Status](https://drone.io/bitbucket.org/ignitionrobotics/ign_transport/status.png)](https://drone.io/bitbucket.org/ignitionrobotics/ign_transport/latest)


## Dependencies

The following dependencies are required to compile ignition-transport from
source:

 - uuid-dev
 - libzmq3-dev
 - czmq
 - robot_msgs
 - cmake
 - ruby-ronn
 - mercurial
 - libprotobuf-dev (robot_msgs)
 - protobuf-compiler (robot_msgs)
 - libprotoc-dev (robot_msgs)
 - libboost-all-dev (robot_msgs)
 - git (czmq)
 - libtool (czmq)
 - automake (czmq)
 - gcc with c++11 support (>=4.8).

    sudo apt-get install build-essential uuid-dev libprotobuf-dev protobuf-compiler libprotoc-dev libzmq3-dev cmake ruby-ronn git mercurial libboost-all-dev libtool automake

    git clone git://github.com/zeromq/czmq.git
    cd czmq
    git checkout v2.2.0 (or latest stable version)
    ./autogen.sh
    ./configure && make check
    sudo make install
    sudo ldconfig
    cd ..

    hg clone https://bitbucket.org/osrf/robot_msgs
    cd robot_msgs
    mkdir build
    cd build
    cmake .. -DCMAKE_INSTALL_PREFIX=/usr
    sudo make install
    cd ..

## Installation

Standard installation can be performed in UNIX systems using the following
steps:

 - mkdir build/
 - cd build/
 - cmake .. -DCMAKE_INSTALL_PREFIX=/usr
 - sudo make install

## Uninstallation

To uninstall the software installed with the previous steps:

 - cd build/
 - sudo make uninstall
