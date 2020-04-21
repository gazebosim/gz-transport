## Ignition Transport 7

### Ignition Transport 7.x.x (202x-xx-xx)

1. Handle `getpwduid_r` error cases. This addresses issue #118. Solution was
   created in pull request #441 by Poh Zhi-Ee.
    * [BitBucket pull request 444](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/444)

### Ignition Transport 7.4.0 (2020-03-09)

1. Removed a `sleep` from NodeShared. The sleep was meant to guarantee
   message delivery during `connect`. This approach would fail if the delay
   between nodes was too large.
    * [BitBucket pull request 436](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/436)

1. Set default message buffer sizes to 1000, for both send and receive
   buffers.
    * [BitBucket pull request 433](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/433)

1. Added support for configuring message buffers via environment variables.
    * [BitBucket pull request 430](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/430)

### Ignition Transport 7.3.0

1. Write to disk from a background thread in log recorder
    * [BitBucket pull request 428](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/428)

1. Restore original Playback::Start and add overload with new parameter to fix ABI.
    * [BitBucket pull request 427](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/427)

1. Improve compiler support for c++ filesystem.
    * [BitBucket pull request 422](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/422)

### Ignition Transport 7.2.1

1. Updates to C interface subscription options.
    * [BitBucket pull request 417](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/417)

### Ignition Transport 7.2.0

1. Support playback of corrupt log files.
    * [BitBucket pull request 398](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/398)

1. Add signal handler to log playback.
    * [BitBucket pull request 399](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/399)

1. Ignore incompatible discovery messages and reduce console spam.
    * [BitBucket pull request 409](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/409)

1. Added additional publishers and subscribers to the `bench` example program in order to simulate high network traffic conditions.
    * [BitBucket pull request 416](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/416)

1. Added topic subscription to the C interface.
    * [BitBucket pull request 385](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/385)

1. Added fast log playback, where messages are published without waiting.
    * [BitBucket pull request 401](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/401)

### Ignition Transport 7.1.0

1. Added method for determining if a throttled publisher is ready to publish.
    * [BitBucket pull request 395](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/395)

1. Add intraprocess field to MessageInfo. The intraprocess field indicates whether the message is coming from a node within this process.
    * [BitBucket pull request 394](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/394)

### Ignition Transport 7.0.0

1. Fix fast constructor-destructor deadlock race condition.
    * [BitBucket pull request 384](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/384)

1. Added ability to specify partition information on a node through the
   CIface.
    * [BitBucket pull request 378](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/378)

1. Added ability to advertise a topic through the CIface.
    * [BitBucket pull request 377](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/377)

1. Added a `-n` argument to the echo command line tool, where `-n` can be used
   to specify the number of messages to echo and then exit. Made the
   `ign.hh` header file private (not installed).
    * [BitBucket pull request 367](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/367)

1. Added start of C interface, currently it supports only pub/sub.
    * [BitBucket pull request 366](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/366)
    * [BitBucket pull request 370](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/370)
    * [BitBucket pull request 373](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/373)

1. Introduce `IGN_RELAY`.
    * [BitBucket pull request 364](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/364)

1. Upgrade to ignition-msgs4.
    * [BitBucket pull request 371](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/371)

## Ignition Transport 6

### Ignition Transport 6.X.X

1. Ignore EPERM and ENOBUFS errors during discovery, generalize cmake for ign tool files
    * [BitBucket pull request 380](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/380)
    * [Issue 106](https://github.com/ignitionrobotics/ign-transport/issues/106)

1. Skip `cmd*.rb` generation on windows to fix build
    * [BitBucket pull request 363](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/363)
    * [Issue 94](https://github.com/ignitionrobotics/ign-transport/issues/94)

### Ignition Transport 6.0.0

1. Upgrade to proto3, c++17, ignition-cmake2 and ignition-msgs3.
    * [BitBucket pull request 312](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/312)

## Ignition Transport 5

### Ignition Transport 5.X.X

1. Added support for alternative clock sources during log recording.
    * [BitBucket pull request 340](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/340)

1. Exposed Log and log Playback time information.
    * [BitBucket pull request 342](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/342)

1. Added the ability to Seek within the log playback, which makes possible to
   jump to any valid time point of the reproduction.
    * [BitBucket pull request 341](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/341)

1. Added the ability to Step the advance of the playback from within the log
   replayer.
    * [BitBucket pull request 339](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/339)

1. Added the ability to Pause/Resume playback from the log replayer.
    * [BitBucket pull request 334](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/334)

1. Added support for topic remapping when running "ign log playback". Note that
   the string ":=" is not allowed now as part of a partition, namespace or topic
   anymore.
    * [BitBucket pull request 331](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/331)

1. Added the ability to remap topic names.
    * [BitBucket pull request 330](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/330)

1. Prevent the log recorder from subscribing to topics that have already
   been added.
    * [BitBucket pull request 329](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/329)

1. Added log::Recorder::Topics that returns the set of added topics.
    * [BitBucket pull request 328](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/328)

1. Added log::Recorder::Filename that returns the name of the log file.
    * [BitBucket pull request 327](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/327)

1. Added a logging tutorial
    * [BitBucket pull request 311](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/311)

1. Added a migration guide for helping with the transition between major
   versions
    * [BitBucket pull request 310](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/310)

1. Converted ignition-transport-log into a component
    * [BitBucket pull request 298](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/298)

1. Added inline versioned namespace to the log library
    * [BitBucket pull request 303](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/303)

1. Added inline versioned namespace to the main library
    * [BitBucket pull request 301](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/301)

1. Added --force option to 'ign log record'
    * [BitBucket pull request 325](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/325)

## Ignition Transport 4

### Ignition Transport 4.X.X

1. Ignore subinterfaces when using determineInterfaces().
    * [BitBucket pull request 314](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/314)

1. Refactored Playback to return a PlaybackHandle from Start()
    * [BitBucket pull request 302](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/302)

1. Added command line tool for the logging features
    * [BitBucket pull request 276](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/276)

1. Added examples using logging features
    * [BitBucket pull request 279](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/279)

1. Added integration tests for recording
    * [BitBucket pull request 275](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/275)

1. Added ability to play back ignition transport topics
    * [BitBucket pull request 274](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/274)

1. Added ability to record ignition transport topics
    * [BitBucket pull request 273](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/273)

1. Added ability to query log messages by topic name and time received
    * [BitBucket pull request 272](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/272)

1. Added ability to get all messages from a log file
    * [BitBucket pull request 271](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/271)

1. Added ability to insert messages into a sqlite3 based log file
    * [BitBucket pull request 270](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/270)

1. Added TopicUtils::DecomposeFullyQualifiedTopic()
    * [BitBucket pull request 269](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/269)

### Ignition Transport 4.0.0 (2018-01-XX)

1. Basic authentication for topics.
    * [BitBucket pull request 236](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/236)

1. Upgrade to ign-cmake.
    * [BitBucket pull request 239](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/239)

1. Added a benchmark program to test latency and throughput.
    * [BitBucket pull request 225](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/225)

1. Support publication and receipt of raw serialized data.
    * [BitBucket pull request 251](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/251)

1. Use zero copy when publishing messages.
    * [BitBucket pull request 229](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/229)

1. Added publishing and receiving messages as raw bytes
    * [BitBucket pull request 251](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/251)

1. Updated service responder callbacks to return a boolean value. The
   existing functions have been deprecated.
    * [BitBucket pull request 260](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/260)
    * [BitBucket pull request 228](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/228)

1. Hide ZMQ from public interfaces
    * [BitBucket pull request 224](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/224)

## Ignition Transport 3

### Ignition Transport 3.X.X



### Ignition Transport 3.1.0 (2017-11-29)

1. Documentation improvements
    * [BitBucket pull request 199](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/199)
    * [BitBucket pull request 200](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/200)
    * [BitBucket pull request 203](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/203)
    * [BitBucket pull request 206](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/206)
    * [BitBucket pull request 197](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/197)
    * [BitBucket pull request 219](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/219)
    * [Issue 63](https://github.com/ignitionrobotics/ign-transport/issues/63)
    * [Issue 67](https://github.com/ignitionrobotics/ign-transport/issues/67)

1. Workaround for the ghost Msbuild warning in Jenkins plugin
    * [BitBucket pull request 205](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/205)

1. Added tests for ign.cc
    * [BitBucket pull request 209](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/209)

1. Remove manual setting of flags for dynamic linking of the Windows CRT library
    * [BitBucket pull request 210](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/210)

1. Add BUILD_TESTING CMake option and tests target
    * [BitBucket pull request 208](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/208)

1. Remove unused statement from Header::Unpack
    * [BitBucket pull request 212](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/212)

1. Port cmake fixes from sdformat
    * [BitBucket pull request 213](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/213)

1. Clean up DefaultFlags.cmake
    * [BitBucket pull request 214](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/214)

1. Add the new const methods to overloaded bool operator
    * [BitBucket pull request 217](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/217)

1. SubscriptionHandler.hh fix std::move compiler warning
    * [BitBucket pull request 222](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/222)

1. Fix ign topic|service fails on MacOS X if system integrity protection is enabled
    * [BitBucket pull request 227](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/227)
    * [Issue 72](https://github.com/ignitionrobotics/ign-transport/issues/72)

### Ignition Transport 3.0.0

1. Added optional message throttling when publishing messages.
    * [BitBucket pull request 194](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/194)

1. Support for an optional MessageInfo parameter in the user callbacks for
   receiving messages. This parameter provides some information about the
   message received (e.g.: topic name).
    * [BitBucket pull request 191](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/191)

1. Added `Node::Publisher::HasConnections` function that can be used to
   check if a Publisher has subscribers.
    * [BitBucket pull request 190](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/190)

1. Add ign topic --echo command line tool.
    * [BitBucket pull request 189](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/189)

1. Support a generic callback signature for receiving messages of any type.
    * [BitBucket pull request 188](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/188)

1. Node::Unadvertise(), Node::Publish() and Node::TopicsAdvertised() removed.
   Node::Advertise() returns a Node::Publisher object that can be used for
   publishing messages. When this object runs out of scope the topic is
   unadvertised.
    * [BitBucket pull request 186](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/186)
    * [BitBucket pull request 185](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/185)
    * [BitBucket pull request 184](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/184)

1. Create AdvertiseMessageOptions and AdvertiseServiceOptions classes.
    * [BitBucket pull request 184](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/184)

1. Subscription options added. The first option is to provide the ability to
   set the received message rate on the subscriber side.
    * [BitBucket pull request 174](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/174)

1. Added ign service --req <args ...> for requesting services using the command line.
    * [BitBucket pull request 172](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/172)

1. Do not allow to advertise a topic that is currently advertised on the same node.
   See [issue #54](https://github.com/ignitionrobotics/ign-transport/issues/54)
    * [BitBucket pull request 169](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/169)

1. ZeroMQ updated from 3.2.4 to 4.0.4 on Windows.
    * [BitBucket pull request 171](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/171)

## Ignition Transport 2.x

1. Fix issue #55.
    * [BitBucket pull request 183](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/183)

1. Protobuf3 support added.
    * [BitBucket pull request 181](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/181)

1. ZeroMQ updated from 3.2.4 to 4.0.4 on Windows.
    * [BitBucket pull request 171](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/171)

1. Copyright added to `tools/code_check.sh` and `tools/cpplint_to_cppcheckxml.py`
    * [BitBucket pull request 168](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/168)

1. Fix case where `std::bad_function_call` could be thrown.
    * [BitBucket pull request 317](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/317)

### Ignition Transport 2.0.0

1. Move ZMQ initialization from constructor to separate function in
   NodeShared.
    * [BitBucket pull request 166](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/166)

1. `Node::Advertise` returns a publisher id that can be used to publish messages, as an alternative to remembering topic strings.
    * [BitBucket pull request 129](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/129)

## Ignition Transport 1.x

### Ignition Transport 1.2.0

1. Removed duplicate code in NetUtils, and improved speed of DNS lookup
    * [BitBucket pull request 128](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/128)
