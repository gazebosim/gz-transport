# Ignition transport

** Ignition transport classes and functions for robot applications.**

Ignition transport is a component in the ignition framework, a set
of libraries designed to rapidly develop robot applications.

  [http://ignitionrobotics.org](http://ignitionrobotics.org)

## Continuous integration

Please refer to the [drone.io
job](https://drone.io/bitbucket.org/ignitionrobotics/ign-transport).

[![Build Status](https://drone.io/bitbucket.org/ignitionrobotics/ign-transport/status.png)](https://drone.io/bitbucket.org/ignitionrobotics/ign-transport/latest)


## Dependencies

The following dependencies are required to compile ignition-transport from
source:

 - uuid-dev
 - libzmq3-dev
 - cmake
 - ruby-ronn
 - mercurial
 - C++ compiler with c++11 support (eg. GCC>=4.8).

**Note:** *if you are using an Ubuntu platform previous to Saucy, you will need to install zeromq from source, since there is no libzmq3-dev*

    sudo apt-get install build-essential uuid-dev libprotobuf-dev protobuf-compiler libzmq3-dev cmake ruby-ronn mercurial

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