## Ignition Transport 2.x

1. Fix issue #55.
    * [Pull request 183](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/183)

1. Protobuf3 support added.
    * [Pull request 181](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/181)

1. ZeroMQ updated from 3.2.4 to 4.0.4 on Windows.
    * [Pull request 171](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/171)

1. Copyright added to `tools/code_check.sh` and `tools/cpplint_to_cppcheckxml.py`
    * [Pull request 168](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/168)

1. Fix case where `std::bad_function_call` could be thrown.
    * [Pull request 317](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/317)

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
