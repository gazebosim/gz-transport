# Ignition transport

** Ignition transport classes and functions for robot applications.**

Ignition transport is a component in the ignition framework, a set
of libraries designed to rapidly develop robot applications.

  [http://ignitionrobotics.org](http://ignitionrobotics.org)

## Dependencies

The following dependencies are required to compile ignition transport from
source:

 - uuid-dev
 - libprotobuf-dev
 - libzmq-dev
 - czmq
 - robot_msgs
 - ruby-ronn
 - gcc with c++11 support (>=4.8).

    sudo apt-get install uuid-dev libprotobuf-dev libzmq-dev robot_msgs ruby-ronn

    git clone git://github.com/zeromq/czmq.git
    cd czmq
    ./autogen.sh
    ./configure && make check
    sudo make install
    sudo ldconfig
    cd ..

## Installation

Standard installation can be performed in UNIX systems using the following
steps:

 - mkdir build/
 - cd build/
 - cmake ..
 - sudo make install

## Uninstallation

To uninstall the software installed with the previous steps:

 - cd build/
 - sudo make uninstall
