=====================
Internal architecture
=====================

The purpose of this section is to describe the internal design of Ignition
Transport. You don't need to read this section if you just want to use the
library in your code. This section will help you to understand our source code
if you're interested in making code contributions.

Ignition Transport's internal architecture can be illustrated with the following
diagram:

::

 +=================================================+    +======================+
 |                         Host #1                 |    |       Host #2        |
 | +-------------------------+ +-----------------+ |    | +------------------+ |
 | |      Process #1         | |  Process #2     | |    | |   Process #3     | |
 | |  +-------+   +-------+  | |  +-------+      | |    | |    +-------+     | |
 | |  |Node #1|   |Node #2|  | |  |Node #3|      | |    | |    |Node #4|     | |
 | |  +-------+   +-------+  | |  +-------+      | |    | |    +-------+     | |
 | |      ⇅           ⇅      | |      ⇅          | |    | |        ⇅         | |
 | | +---------+ +---------+ | | +-------------+ | |    | | +--------------+ | |
 | | |Shared #1| |Shared #2| | | |Shared #3    | | |    | | |Shared #4     | | |
 | | +---------+ +---------+ | | +-------------+ | |    | | +--------------+ | |
 | |  ⇑   ⇅           ⇅   ⇑  | |       ⇅       ⇑ | |    | |        ⇅       ⇑ | |
 | |  | +--------------+  |  | |+------------+ | | |    | | +------------+ | | |
 | |  | | Discovery #1 |  |  | ||Discovery #2| | | |    | | |Discovery #3| | | |
 | |  ⇓ +--------------+  ⇓  | |+------------+ ⇓ | |    | | +------------+ ⇓ | |
 | +-------------------------+ +-----------------+ |    | +------------------+ |
 +=================================================+    +======================+
                ⇅                      ⇅                         ⇅
                ==================================================
                \              Local Area Network                \
                ==================================================

Next, are the most important components of the library:

1. Node.

  This class is the main interface with the users. The ``Node`` class contains
  all the functions that allow users to advertise, subscribe and publish
  messages, as well as advertise and request topics. This is the only class that
  a user should use.

1. NodeShared (shown as ``Shared`` in the diagram for space purposes).

  A single instance of a ``NodeShared`` class is shared between all the
  ``Node`` objects running inside the same process. The ``NodeShared`` instance
  contains all the ZMQ sockets used for sending and receiving data for message
  and service communication. The goal of this class is to share resources
  between a group of nodes.

1. Discovery.

  A discovery layer is required in each process to learn about the location of
  topics and services. Our topics and services don't have any location
  information, they are just plain strings, so we need a way to learn where are
  they located (similar to a DNS service). ``Discovery`` uses a custom protocol
  and UDP multicast for communicating with other ``Discovery`` instances. These
  instances can be located on the same or different machines over the same LAN.
  At this point is not possible to discover a ``Node`` outside of the LAN, this
  is a future request that will eventually be added to the library.


Discovery service
=================

The Discovery class implements a distributed service discovery protocol. The
services are plain strings (``/echo``, ``/my_robot/camera``) and this layer
learns about the meta information associated to each service. The service
location, the unique identifier of the node providing the service or its process
are some examples of the information that the discovery component learns for
each service. The main responsability of the discovery is to keep an updated
list of active services ready to be queried by other entities.

In Ignition Transport we use two discovery objects, each one operating on a
different UDP port. One object is dedicated to topics and the other is dedicated
to services.

API
---

The first thing to do before using a discovery object is to create it. The
``Discovery`` class constructor requires a parameter for specifying the UDP port
to be used by the discovery sockets and the UUID of the process in which the
discovery is running. This UUID will be used when announcing local services.

Once a ``Discovery`` object is created it won't discover anything. You'll need
to call the `Start()` function for enabling the discovery.

Besides discovering services from the outside world, the discovery will announce
the services that are offered in the same process that the discovery is running.
The ``Advertise()`` function will register a local service and announce it over
the network. The symmetric `Unadvertise()` will notify that a service won't be
offered anymore.

``Discover()`` is used to learn about a given topic as soon as possible. It's
important to remark the "as soon as possible" because discovery will eventually
learn about all the topics but this might take some time (depending on
configuration). If a client needs to know about a particular service,
``Discover()`` will trigger a discovery request that will reduce the time needed
to discover the information about a service.

As you can imagine, exchanging messages over the network can be slow and we
cannot block the users waiting for discovery information. We don't even know how
many nodes are on the network so it would be hard and really slow to block and
return all the information to our users when available. The way we tackle the
notification  inside the ``Discovery`` is using callbacks. A discovery user
needs to register two callbacks: one for receiving notifications when new
services are available  and another for notifying when a service is no longer
active. The functions       ``ConnectionsCb()`` and ``DisconnectionsCb()`` allow
the discovery user to set these two notification callbacks. For example, a user
will invoke the `Discover()` call and, after some time, its ``ConnectionCb``
will be executed with the information about the requested service. In the
meantime, other callback invocations could be triggered because ``Discovery``
will proactively learn about all the available services and generate
notifications.

You can check the complete API details
`here <http://ignition-transport.readthedocs.io/en/latest/api/api.html>`_.

[Un]Announce a local service
--------------------------

This feature registers a new service in the internal data structure that keeps
all the discovery information. Local and remote services are stored in the same
way, the only difference is that the local services will share the process UUID
with the discovery service. We store what we call a ``Publisher``, which
contains the service name and all the metadata associated.

Each publisher advertises the service with a specific scope as described `here
<http://ignition-transport.readthedocs.io/en/latest/nodesAndTopics/nodesAndTopics.html#topic-scope>`_.
If the service' scope is `PROCESS`` the discovery won't announce it over the
network, otherwise it will send to the multicast group an
``ADVERTISE`` message with the following format:

::
   HEADER
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |            Version            |     Process UUID Length       |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |     Process UUID Length       |         Process UUID          |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                                                               |
  \                          Process UUID                         \
  |                                                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  | Process UUID  |  Message Type |             Flags             |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   [UN]ADVERTISE
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                                                               |
  \                            Header                             \
  |                                                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                                                               |
  \                     Serialized Publisher                      \
  |                                                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+


All discovery nodes will receive this request and should update its discovery
information and notify its user via the notification callbacks if they didn't
have previous information about the service received. An ADVERTISE message
should trigger the connection callback, while an UNADVERTISE message should fire
the disconnection callback.

Trigger a service discovery
---------------------------

A user can call ``Discover()`` for triggering the inmediate discovery of a
service. Over the wire, this call will generate a ``SUBSCRIBE`` message with
the following format:

::
   SUBSCRIBE
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                                                               |
  \                            Header                             \
  |                                                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                         Topic length                          |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                                                               |
  \                            Topic                              \
  |                                                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+


All discovery instances listening on the same port where the SUBSCRIBE message
was sent will receive the message. Each discovery instance with a local service
registered should answer with an ADVERTISE message. The answer is a multicast
message too that should be received by all discovery instances.

Service update
--------------

Each discovery instance should periodically send an ADVERTISE message per local
service announced over the multicast channel to notify that all information
already announced is still valid. The frequency of sending these service update
messages can be changed with the function ``SetHeartbeatInterval()``. By
default, the service update frequency is set to 1 second.

Alternatively, we could replace the send of all ADVERTISE messages with one
HEARTBEAT message that contains the process UUID of the discovery instance. Upon
reception, all other discovery instances should update all their entries
associated with the received process UUID. Although this approach is more
efficient and saves some messages sent over the network, prevents a discovery
instance to learn about services available without explictly asking for them.
We think this is a good feature to have. For example, an introspection tool that
shows all the services available can take advantage of this feature without and
prior knowledge.

Is responsability of each discovery instance to cancel any service that hasn't
been updated for a while. The function ``SilenceInterval()` sets the maximum
time that an entry should be stored in memory without hearing an ADVERTISE
message. Every ADVERTISE message received should refresh the service timestamp
associated with it.

When a discovery instance terminates, it should notify through the discovery
channel that all its services need to invalidated. This is performed by sending
a BYE message with the following format:


::
   BYE
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                                                               |
  \                            Header                             \
  |                                                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

When this message is received, a discovery instance should invalidate all
entries associated with the process UUID contained in the header. Note that this
is the expected behavior when a discovery instance gently terminates. In the
case of an abrupt termination, the lack of service updates will cause the same
result, although it'll take a bit more time.


Threading model
---------------

A discovery instance will create an additional internal thread when the user
calls ``Start()``. This thread takes care of the service update tasks. This
involves the reception of other discovery messages and the update of the
discovery information. Also, it's among its responsabilities to answer with an
ADVERTISE message when a SUBSCRIBE message is received and there are local
services available.

The first time announcement of a local service and the explicit discovery
request of a service happen on the user thread. So, in a regular scenario where
the user doesn't share discovery among other threads, all the discovery
operations will run in two threads, the user thread and the internal discovery
thread spawned after calling ``Start()``. All the functions in the discovery are
thread safe.

Multiple network interfaces
---------------------------

The goal of the discovery service is to discover all topics available. It's not
uncommon these days that a machine has multiple network interfaces for its wired
and wireless connections, a virtual machine, or a localhost device, among
others. By selecting one network interface and listening only on this one, we
would miss the discovery messages that are sent by instances sitting on other
subnets.

E.g.:

Our discovery service handles this problem in severals steps. First, it learns
about the network interfaces that are available locally. For that purpose we
have developed the ``NetUtils`` auxiliar file. The ``determineInterfaces()``
function returns a list of all the network interfaces found on the machine.
When we know all the available network interfaces we create a container of
sockets, one per local IP address. These sockets are used for sending discovery
data over the network, flooding all the subnets and reaching other potential
discovery instances.

We use one of the sockets contained in the vector for receiving data via the
multicast channel. We have to join the multicast group for each local network
interface but we can reuse the same socket. This will guarantee that our socket
will receive the multicast traffic coming from any of our local network
interfaces. This is the reason for having a single `bind()` function in our call
even if we can receive from multiple interfaces. Our receiving socket is the one
we register in the ``zmq::poll()` function for processing incoming discovery
data.

When it's time to send outbound data, we iterate through the list of sockets and
send the message over each one, flooding all the subnets with our discovery
requests.

Note that the result of ``determineInterfaces()`` can be manually set by using
the ``IGN_IP`` environment variable, as described `here <http://ignition-transport.readthedocs.io/en/latest/environment_variables/env_variables.html>`_. This will essentially ignore other network interfaces,
isolating all discovery traffic through the specified interface.
