===========================
What is Ignition Transport?
===========================

Ignition transport is an open source communication library that allows sharing
data between clients. In our context, a client is called a node. Nodes might
be running within the same process in the same machine or in machines located in
different continents. Ignition transport is multi-platform (Linux, Mac OS X, and
Windows), so all the low level details, such as data alignment or endianness are
hidden for you.

Ignition transport uses Google Protocol buffers as the data type for
communicating between nodes. Users can define its own messages using the
Protobuf utils, and then, exchange them between the nodes. Ignition transport
discovers, serializes and delivers messages to the destinations.

* What programming language can I use to interface Ignition Transport?
C++ is our native implementation and so far the only way to use the library. We
might offer different wrappers for the most popular languages in the future.