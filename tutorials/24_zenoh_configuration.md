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
including an example using the Zenoh router for centralized message routing.

## Configuration Methods

There are multiple ways to configure Zenoh when using Gazebo Transport:

1. **Environment variable**: Set the `ZENOH_CONFIG` environment variable to
   point to a JSON5 configuration file.
2. **Default configuration**: If no configuration is specified, Zenoh uses its
   default settings which enable peer-to-peer multicast scouting.

For detailed information about all available configuration options, refer to the
official [Zenoh configuration documentation](https://zenoh.io/docs/manual/configuration/).

## Using the Zenoh Router

One common use case for Zenoh configuration is connecting nodes through a
centralized router. The Zenoh router (`zenohd`) acts as a message broker,
which can be useful for scenarios where multicast discovery is not available
or when you need more control over message routing.

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

**Note: Version Compatibility**

The Zenoh wire protocol is
[backward-compatible within the 1.x series](https://zenoh.io/blog/2024-04-30-zenoh-electrode/),
so the router does not need to exactly match the version of the Zenoh library
that Gazebo Transport was compiled against.

Then, download the ROS 2 default configuration files which provide a good
starting point for router-based communication:

```bash
cd /tmp
wget https://raw.githubusercontent.com/ros2/rmw_zenoh/refs/heads/rolling/rmw_zenoh_cpp/config/DEFAULT_RMW_ZENOH_SESSION_CONFIG.json5
wget https://raw.githubusercontent.com/ros2/rmw_zenoh/refs/heads/rolling/rmw_zenoh_cpp/config/DEFAULT_RMW_ZENOH_ROUTER_CONFIG.json5
```

### Example: Publisher and Subscriber with Router

This example demonstrates how to run the Gazebo Transport publisher and
subscriber examples using the Zenoh router for message routing.

**Terminal 1 - Start the Zenoh router:**

```bash
zenohd -c /tmp/DEFAULT_RMW_ZENOH_ROUTER_CONFIG.json5
```

**Terminal 2 - Run the publisher:**

```bash
GZ_TRANSPORT_IMPLEMENTATION=zenoh ZENOH_CONFIG=/tmp/DEFAULT_RMW_ZENOH_SESSION_CONFIG.json5 ./build/bin/publisher
```

**Terminal 3 - Run the subscriber:**

```bash
GZ_TRANSPORT_IMPLEMENTATION=zenoh ZENOH_CONFIG=/tmp/DEFAULT_RMW_ZENOH_SESSION_CONFIG.json5 ./build/bin/subscriber
```

The subscriber should receive messages from the publisher.

### Verifying Router Dependency

To verify that communication is happening through the router, you can repeat
the above steps **without** launching the Zenoh router. In this case, the
subscriber should not receive any messages, confirming that the configured
endpoints require the router for communication.
