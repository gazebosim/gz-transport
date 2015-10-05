==============================
Understanding nodes and topics
==============================

Nodes
=====

The communication in Ignition Transport follows a pure distributed architecture,
where there is no central process, broker or similar. All the nodes in the
network can act as publishers, subscribers, provide services and request
services.

A publisher is a node that produces information and a subscriber is a node that
consumes information. There are two categories or ways to communicate in
Ignition transport. First, we could use a publish/subscribe approach, where a
node advertises a topic, and then, publishes periodic updates. On the other
side, one or more nodes subscribe to the same topic registering a function that
will be executed each time a new message is received. An alternative
communication paradigm is based on service calls. A service call is a remote
service that a node offers to the rest of the nodes. A node can request a
service in a similar way a local function is executed.

Topics
======

A topic is just a name for grouping a specific set of messages or a particular
service. Imagine that you have a camera and want to publish periodically its
images. Your node could advertise a topic called */image*, and then, publish a
new message on this topic every time a new image is available. Other nodes, will
subscribe to the same topic and will receive the messages containing the image.
A node could also offer the service *echo* in the topic */echo*. Any node
interested in this service will request a service call on topic */echo*. The
service call will accept arguments and will return a result. In our *echo*
service example, the result will be similar to the input parameter passed to the
service.

There are some rules to follow when selecting a topic name. It should be any
alphanumeric name followed by zero or more slashes. For example: */image*,
*head_position*, */robot1/joints/HeadPitch* are examples of valid topic names.
The next table summarizes the allowed and not allowed topic rules.

============  ========  =======
Topic name    Validity  Comment
============  ========  =======
*/topicA*     Valid
*/topicA/*    Valid     Equivalent to */topicA*
*/a/b*        Valid
              Invalid   Empty string is invalid.
*my topic*    Invalid   Contains white space.
*//image*     Invalid   Contains two consecutive *//*
*/*           Invalid   */* topic is not allowed.
*~myTopic*    Invalid   Symbol *~* not allowed.
============  ========  =======

Topic scope
===========

A topic can be optionally advertised with a scope. A scope allows you to set the
visibility of this topic. The available scopes are *Process*, *Host*, and *All*.
A *Process* scope means that the advertised topic will only be visible in the
nodes within the same process as the advertiser. A topic with a *Host* scope
restricts the visibility of a topic to nodes located in the same machine as the
advertiser. Finally, by specifying a scope with an *All* value, you're allowing
your topic to be visible by any node.