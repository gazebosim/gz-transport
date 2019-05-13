## Ignition Transport 7

### Ignition Transport 7.X.X

### Ignition Transport 7.0.0

1. Fix fast constructor-destructor deadlock race condition.
    * [Pull request 384](https://bitbucket.org/ignitionrobotics/ign-transport/pull-requests/384)

1. Added ability to specify partition information on a node through the
   CIface.
    * [Pull request 378](https://bitbucket.org/ignitionrobotics/ign-transport/pull-requests/378)

1. Added ability to advertise a topic through the CIface.
    * [Pull request 377](https://bitbucket.org/ignitionrobotics/ign-transport/pull-requests/377)

1. Added a `-n` argument to the echo command line tool, where `-n` can be used
   to specify the number of messages to echo and then exit. Made the
   `ign.hh` header file private (not installed).
    * [Pull request 367](https://bitbucket.org/ignitionrobotics/ign-transport/pull-requests/367)

1. Added start of C interface, currently it supports only pub/sub.
    * [Pull request 366](https://bitbucket.org/ignitionrobotics/ign-transport/pull-requests/366)
    * [Pull request 370](https://bitbucket.org/ignitionrobotics/ign-transport/pull-requests/370)
    * [Pull request 373](https://bitbucket.org/ignitionrobotics/ign-transport/pull-requests/373)

1. Introduce `IGN_RELAY`.
    * [Pull request 364](https://bitbucket.org/ignitionrobotics/ign-transport/pull-requests/364)

1. Upgrade to ignition-msgs4.
    * [Pull request 371](https://bitbucket.org/ignitionrobotics/ign-transport/pull-requests/371)

## Ignition Transport 6

### Ignition Transport 6.X.X

1. Ignore EPERM and ENOBUFS errors during discovery, generalize cmake for ign tool files
    * [Pull request 380](https://bitbucket.org/ignitionrobotics/ign-transport/pull-requests/380)
    * [Issue 106](https://bitbucket.org/ignitionrobotics/ign-transport/issue/106)

1. Skip `cmd*.rb` generation on windows to fix build
    * [Pull request 363](https://bitbucket.org/ignitionrobotics/ign-transport/pull-requests/363)
    * [Issue 94](https://bitbucket.org/ignitionrobotics/ign-transport/issue/94)

### Ignition Transport 6.0.0

1. Upgrade to proto3, c++17, ignition-cmake2 and ignition-msgs3.
    * [Pull request 312](https://bitbucket.org/ignitionrobotics/ign-transport/pull-requests/312)

## Ignition Transport 5

### Ignition Transport 5.X.X

1. Added support for alternative clock sources during log recording.
    * [Pull request 340](https://bitbucket.org/ignitionrobotics/ign-transport/pull-requests/340)

1. Exposed Log and log Playback time information.
    * [Pull request 342](https://bitbucket.org/ignitionrobotics/ign-transport/pull-requests/342)

1. Added the ability to Seek within the log playback, which makes possible to
   jump to any valid time point of the reproduction.
    * [Pull request 341](https://bitbucket.org/ignitionrobotics/ign-transport/pull-requests/341)

1. Added the ability to Step the advance of the playback from within the log
   replayer.
    * [Pull request 339](https://bitbucket.org/ignitionrobotics/ign-transport/pull-requests/339)

1. Added the ability to Pause/Resume playback from the log replayer.
    * [Pull request 334](https://bitbucket.org/ignitionrobotics/ign-transport/pull-requests/334)

1. Added support for topic remapping when running "ign log playback". Note that
   the string ":=" is not allowed now as part of a partition, namespace or topic
   anymore.
    * [Pull request 331](https://bitbucket.org/ignitionrobotics/ign-transport/pull-requests/331)

1. Added the ability to remap topic names.
    * [Pull request 330](https://bitbucket.org/ignitionrobotics/ign-transport/pull-requests/330)

1. Prevent the log recorder from subscribing to topics that have already
   been added.
    * [Pull request 329](https://bitbucket.org/ignitionrobotics/ign-transport/pull-requests/329)

1. Added log::Recorder::Topics that returns the set of added topics.
    * [Pull request 328](https://bitbucket.org/ignitionrobotics/ign-transport/pull-requests/328)

1. Added log::Recorder::Filename that returns the name of the log file.
    * [Pull request 327](https://bitbucket.org/ignitionrobotics/ign-transport/pull-requests/327)

1. Added a logging tutorial
    * [Pull request 311](https://bitbucket.org/ignitionrobotics/ign-transport/pull-requests/311)

1. Added a migration guide for helping with the transition between major
   versions
    * [Pull request 310](https://bitbucket.org/ignitionrobotics/ign-transport/pull-requests/310)

1. Converted ignition-transport-log into a component
    * [Pull request 298](https://bitbucket.org/ignitionrobotics/ign-transport/pull-requests/298)

1. Added inline versioned namespace to the log library
    * [Pull request 303](https://bitbucket.org/ignitionrobotics/ign-transport/pull-requests/303)

1. Added inline versioned namespace to the main library
    * [Pull request 301](https://bitbucket.org/ignitionrobotics/ign-transport/pull-requests/301)

1. Added --force option to 'ign log record'
    * [Pull request 325](https://bitbucket.org/ignitionrobotics/ign-transport/pull-requests/325)

## Ignition Transport 4

### Ignition Transport 4.X.X

1. Ignore subinterfaces when using determineInterfaces().
    * [Pull request 314](https://bitbucket.org/ignitionrobotics/ign-transport/pull-requests/314)

1. Refactored Playback to return a PlaybackHandle from Start()
    * [Pull request 302](https://bitbucket.org/ignitionrobotics/ign-transport/pull-requests/302)

1. Added command line tool for the logging features
    * [Pull request 276](https://bitbucket.org/ignitionrobotics/ign-transport/pull-requests/276)

1. Added examples using logging features
    * [Pull request 279](https://bitbucket.org/ignitionrobotics/ign-transport/pull-requests/279)

1. Added integration tests for recording
    * [Pull request 275](https://bitbucket.org/ignitionrobotics/ign-transport/pull-requests/275)

1. Added ability to play back ignition transport topics
    * [Pull request 274](https://bitbucket.org/ignitionrobotics/ign-transport/pull-requests/274)

1. Added ability to record ignition transport topics
    * [Pull request 273](https://bitbucket.org/ignitionrobotics/ign-transport/pull-requests/273)

1. Added ability to query log messages by topic name and time received
    * [Pull request 272](https://bitbucket.org/ignitionrobotics/ign-transport/pull-requests/272)

1. Added ability to get all messages from a log file
    * [Pull request 271](https://bitbucket.org/ignitionrobotics/ign-transport/pull-requests/271)

1. Added ability to insert messages into a sqlite3 based log file
    * [Pull request 270](https://bitbucket.org/ignitionrobotics/ign-transport/pull-requests/270)

1. Added TopicUtils::DecomposeFullyQualifiedTopic()
    * [Pull request 269](https://bitbucket.org/ignitionrobotics/ign-transport/pull-requests/269)

### Ignition Transport 4.0.0 (2018-01-XX)

1. Basic authentication for topics.
    * [Pull request 236](https://bitbucket.org/ignitionrobotics/ign-transport/pull-requests/236)

1. Upgrade to ign-cmake.
    * [Pull request 239](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/239)

1. Added a benchmark program to test latency and throughput.
    * [Pull request 225](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/225)

1. Support publication and receipt of raw serialized data.
    * [Pull request 251](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/251)

1. Use zero copy when publishing messages.
    * [Pull request 229](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/229)

1. Added publishing and receiving messages as raw bytes
    * [Pull request 251](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/251)

1. Updated service responder callbacks to return a boolean value. The
   existing functions have been deprecated.
    * [Pull request 260](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/260)
    * [Pull request 228](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/228)

1. Hide ZMQ from public interfaces
    * [Pull request 224](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/224)

## Ignition Transport 3

### Ignition Transport 3.X.X



### Ignition Transport 3.1.0 (2017-11-29)

1. Documentation improvements
    * [Pull request 199](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/199)
    * [Pull request 200](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/200)
    * [Pull request 203](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/203)
    * [Pull request 206](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/206)
    * [Pull request 197](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/197)
    * [Pull request 219](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/219)
    * [Issue 63](https://bitbucket.org/ignitionrobotics/ign-transport/issues/63)
    * [Issue 67](https://bitbucket.org/ignitionrobotics/ign-transport/issues/67)

1. Workaround for the ghost Msbuild warning in Jenkins plugin
    * [Pull request 205](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/205)

1. Added tests for ign.cc
    * [Pull request 209](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/209)

1. Remove manual setting of flags for dynamic linking of the Windows CRT library
    * [Pull request 210](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/210)

1. Add BUILD_TESTING CMake option and tests target
    * [Pull request 208](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/208)

1. Remove unused statement from Header::Unpack
    * [Pull request 212](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/212)

1. Port cmake fixes from sdformat
    * [Pull request 213](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/213)

1. Clean up DefaultFlags.cmake
    * [Pull request 214](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/214)

1. Add the new const methods to overloaded bool operator
    * [Pull request 217](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/217)

1. SubscriptionHandler.hh fix std::move compiler warning
    * [Pull request 222](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/222)

1. Fix ign topic|service fails on MacOS X if system integrity protection is enabled
    * [Pull request 227](https://bitbucket.org/ignitionrobotics/ign-transport/pull-request/227)
    * [Issue 72](https://bitbucket.org/ignitionrobotics/ign-transport/issues/72)

### Ignition Transport 3.0.0

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
