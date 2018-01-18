## Ignition Transport 4.0.0

1. Use zero copy when publishing messages.
    * [Pull request 229](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/229)

1. Added publishing and receiving messages as raw bytes
    * [Pull request 251](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/251)

1. Updated service responder callbacks to return a boolean value. The
   existing functions have been deprecated.
    * [Pull request 260](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/260)
    * [Pull request 228](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/228)

## Ignition Transport 3.0.0

1. Added optional message throttling when publishing messages.
    * [Pull request 194](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/194)

1. Support for an optional MessageInfo parameter in the user callbacks for
   receiving messages. This parameter provides some information about the
   message received (e.g.: topic name).
    * [Pull request 191](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/191)

1. Added `Node::Publisher::HasConnections` function that can be used to
   check if a Publisher has subscribers.
    * [Pull request 190](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/190)

1. Add ign topic --echo command line tool.
    * [Pull request 189](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/189)

1. Support a generic callback signature for receiving messages of any type.
    * [Pull request 188](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/188)

1. Node::Unadvertise(), Node::Publish() and Node::TopicsAdvertised() removed.
   Node::Advertise() returns a Node::Publisher object that can be used for
   publishing messages. When this object runs out of scope the topic is
   unadvertised.
    * [Pull request 186](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/186)
    * [Pull request 185](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/185)
    * [Pull request 184](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/184)

1. Create AdvertiseMessageOptions and AdvertiseServiceOptions classes.
    * [Pull request 184](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/184)

1. Subscription options added. The first option is to provide the ability to
   set the received message rate on the subscriber side.
    * [Pull request 174](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/174)

1. Added ign service --req <args ...> for requesting services using the command line.
    * [Pull request 172](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/172)

1. Do not allow to advertise a topic that is currently advertised on the same node.
   See [issue #54](https://bitbucket.org/ignitionrobotics/ign-transport/issues/54)
    * [Pull request 169](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/169)

1. Hide ZMQ from public interfaces
    * [Pull request 224](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/224)

1. ZeroMQ updated from 3.2.4 to 4.0.4 on Windows.
    * [Pull request 171](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/171)

## Ignition Transport 2.x

1. Fix issue #55.
    * [Pull request 183](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/183)

1. Protobuf3 support added.
    * [Pull request 181](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/181)

1. ZeroMQ updated from 3.2.4 to 4.0.4 on Windows.
    * [Pull request 171](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/171)

1. Copyright added to `tools/code_check.sh` and `tools/cpplint_to_cppcheckxml.py`
    * [Pull request 168](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/168)

### Ignition Transport 2.0.0

1. Move ZMQ initialization from constructor to separate function in
   NodeShared.
    * [Pull request 166](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/166)

1. `Node::Advertise` returns a publisher id that can be used to publish messages, as an alternative to remembering topic strings.
    * [Pull request 129](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/129)

## Ignition Transport 1.x

### Ignition Transport 1.2.0

1. Removed duplicate code in NetUtils, and improved speed of DNS lookup
    * [Pull request 128](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/128)
