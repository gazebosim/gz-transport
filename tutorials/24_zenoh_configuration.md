\page zenohconfig Zenoh Configuration

Previous Tutorial: \ref topicstatistics

## Overview

**Note: This feature is only available when using the zenoh middleware
backend.**

When using the Zenoh middleware backend, you can customize its behavior through
configuration files. Zenoh provides extensive configuration options that allow
you to control connection endpoints, scouting behavior, transport settings,
quality of service parameters, and more.

This tutorial demonstrates how to configure Zenoh when using Gazebo Transport,
including examples using the Zenoh router for centralized discovery and
routing data through the router.

## Configuration methods

There are two ways to configure Zenoh when using Gazebo Transport:

1. **Environment variable**: Set the `ZENOH_CONFIG` environment variable to
   point to a JSON5 configuration file.
2. **Default configuration**: If no configuration is specified, Zenoh uses its
   default settings which enable peer-to-peer multicast scouting.

For detailed information about all available configuration options, refer to the
official [Zenoh configuration documentation](https://zenoh.io/docs/manual/configuration/).

## Using the Zenoh router

One common use case for Zenoh configuration is connecting nodes through a
centralized router. The Zenoh router (`zenohd`) can be useful for scenarios
where multicast discovery is not available or when you need more control over
message routing.

### Prerequisites

Install the Zenoh router by following the
[official Zenoh installation guide](https://zenoh.io/docs/getting-started/installation/).
We recommend installing the latest version of `zenohd` from the
[Zenoh releases page](https://github.com/eclipse-zenoh/zenoh/releases).

Alternatively, if you have ROS 2 installed with the `rmw_zenoh_cpp` package,
you can use the router bundled with it:

```bash
ros2 run rmw_zenoh_cpp rmw_zenohd
```

**Note: Version compatibility**

The Zenoh wire protocol is
[backward-compatible within the 1.x series](https://zenoh.io/blog/2024-04-30-zenoh-electrode/),
so the router does not need to exactly match the version of the Zenoh library
that Gazebo Transport was compiled against.

Then, create the following session configuration file. It connects to the
router in `"peer"` mode with multicast scouting disabled, so that discovery
is handled by the router:

**Session configuration** (`/tmp/gz_zenoh_session.json5`):

```json5
{
  mode: "peer",
  connect: {
    endpoints: [
      "tcp/localhost:7447"
    ],
  },
  scouting: {
    multicast: {
      enabled: false,
    },
  },
}
```

For more advanced options, refer to the
[Zenoh configuration reference](https://github.com/eclipse-zenoh/zenoh/blob/main/DEFAULT_CONFIG.json5).

### Example: Publisher and subscriber with router discovery

This example demonstrates how to run the Gazebo Transport publisher and
subscriber examples using the Zenoh router for discovery. In this default
configuration, the router helps nodes discover each other but data flows
directly between peers.

**Terminal 1 - Start the Zenoh router:**

```bash
zenohd
```

**Terminal 2 - Run the publisher:**

```bash
GZ_TRANSPORT_IMPLEMENTATION=zenoh ZENOH_CONFIG=/tmp/gz_zenoh_session.json5 ./build/bin/publisher
```

**Terminal 3 - Run the subscriber:**

```bash
GZ_TRANSPORT_IMPLEMENTATION=zenoh ZENOH_CONFIG=/tmp/gz_zenoh_session.json5 ./build/bin/subscriber
```

The subscriber should receive messages from the publisher. Note that
communication between the publisher and subscriber is not interrupted if the
router is stopped after the communication has started.

### Routing data through the router

By default, the session configuration uses `"peer"` mode, where the router
assists with discovery but data flows directly between nodes. If you want all
data to flow through the router, change the session mode to `"client"` in
`/tmp/gz_zenoh_session.json5`:

```json5
  mode: "client",
```

With this change, all messages are routed through the router. You can verify
this by stopping the router while the publisher and subscriber are running —
communication will be interrupted.

### Verifying router dependency

To verify that communication requires the router, repeat the above steps
**without** launching the Zenoh router. In this case, the subscriber should
not receive any messages, confirming that the configured endpoints require the
router.
