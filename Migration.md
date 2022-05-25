# Note on deprecations

A tick-tock release cycle allows easy migration to new software versions.
Obsolete code is marked as deprecated for one major release.
Deprecated code produces compile-time warnings. These warning serve as
notification to users that their code should be upgraded. The next major
release will remove the deprecated code.

## Gazebo Transport 11.X to 12.X

### Deprecated

1. The `ignition` namespace is deprecated and will be removed in future versions.  Use `gz` instead.

1. Header files under `ignition/...` are deprecated and will be removed in future versions.
     Use `gz/...` instead.

1. `IGN_TRANSPORT_LOG_SQL_PATH` is deprecated and will be removed in future versions.
     Use `GZ_TRANSPORT_LOG_SQL_PATH` instead. 

## Gazebo Transport 9.X to 10.X

### Addition

1. Dependency on `cli` component of `ignition-utils`.
    * [GitHub pull request 229](https://github.com/gazebosim/gz-transport/pull/229)

## Gazebo Transport 8.X to 9.X

### Removed

1. Remove deprecations before 9.x.x release
    * [GitHub pull request 175](https://github.com/gazebosim/gz-transport/pull/175)

## Gazebo Transport 7.X to 8.X

### Deprecated

1. NodeShared::TriggerSubscriberCallbacks
    * [BitBucket pull request 404](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/404)

1. The discovery wire protocol changed to use gz::msgs::Discovery
   instead of C-structs. The Packet.hh header file is deprecated, which
   contained the Header, SubscriptionMsg, and AdvertiseMessage classes. The
   version of the wire protocal has bumped from 9 to 10. This means Ignition
   Transport 8+ will not work with Gazebo Transport 7 and below.
    * [BitBucket pull request 403](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/403)

## Gazebo Transport 6.X to 7.X

### Removed

1. The `ign.hh` file is not longer installed.
    * [BitBucket pull request 367](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/367)

## Gazebo Transport 4.X to 5.X

## Gazebo Transport 3.X to 4.X

1. Service responder callbacks passed to `Node::Advertise` should now return
   a boolean value instead of taking in a boolean output parameter. The existing
   functions have been deprecated.
    * [BitBucket pull request 260](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/260)
    * [BitBucket pull request 228](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/228)
