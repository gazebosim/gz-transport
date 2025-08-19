\page introduction Introduction

Next Tutorial: \ref installation

## What is Gazebo Transport?

Gazebo Transport is an open source communication library that allows sharing
data between clients. In our context, a client is called a node. Nodes might
be running within the same process on the same machine or on separate machines
located around the world. Gazebo Transport is multi-platform (Linux, Mac OS X,
and Windows), so all the low level details, such as data alignment or
endianness, are hidden for you.

Gazebo Transport uses
[Google Protocol buffers](https://developers.google.com/protocol-buffers/?hl=en)
as the data type for communicating between nodes. Users can define their own
messages using the Protobuf utils, and then exchange them between the nodes.
Gazebo Transport discovers, serializes and delivers messages to the
destinations using a combination of custom code and a middleware backend.
Currently, two middleware backends are available:
[ZeroMQ](http://zeromq.org/) and [Zenoh](https://zenoh.io/).

## How do I specify my middleware backend?

Gazebo Transport support an environment variable `GZ_TRANSPORT_IMPLEMENTATION`
that allows the user to select the middleware backend to use.

| Environment variable        | Allowed values |
| :-------------------------: | :------------: |
| GZ_TRANSPORT_IMPLEMENTATION | zeromq, zenoh  |

zeromq is the default middleware backend and the recommended option for
production code.

Please, review \ref envvars to learn about the environment variables that can
be used with each middleware backend.

## What programming language can I use with Gazebo Transport?

C++ is the native implementation and the only language that has all available
library features. Python implementation is a wrapper around C++ methods using
`pybind11`. It does not support all features like C++, but contains the main
features such as publication, subscription and service request.
