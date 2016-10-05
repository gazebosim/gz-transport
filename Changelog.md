## Ignition Transport 3.0.0

1. Subscription options added. The first option is to provide the ability to
   set the received message rate on the subscriber side.
    * [Pull request 174](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/174)

1. Added ign service --req <args ...> for requesting services using the command line.
    * [Pull request 172](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/172)

1. Do not allow to advertise a topic that is currently advertised on the same node.
   See [issue #54] (https://bitbucket.org/ignitionrobotics/ign-transport/issues/54)
    * [Pull request 169](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/169)

1. ZeroMQ updated from 3.2.4 to 4.0.4 on Windows.
    * [Pull request 171](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/171)

## Ignition Transport 2.x

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
