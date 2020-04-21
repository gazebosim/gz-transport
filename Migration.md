# Note on deprecations

A tick-tock release cycle allows easy migration to new software versions.
Obsolete code is marked as deprecated for one major release.
Deprecated code produces compile-time warnings. These warning serve as
notification to users that their code should be upgraded. The next major
release will remove the deprecated code.

## Ignition Transport 6.X to 7.X

### Removed

1. The `ign.hh` file is not longer installed.
    * [BitBucket pull request 367](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/367)

## Ignition Transport 4.X to 5.X

## Ignition Transport 3.X to 4.X

1. Service responder callbacks passed to `Node::Advertise` should now return
   a boolean value instead of taking in a boolean output parameter. The existing
   functions have been deprecated.
    * [BitBucket pull request 260](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/260)
    * [BitBucket pull request 228](https://osrf-migration.github.io/ignition-gh-pages/#!/ignitionrobotics/ign-transport/pull-requests/228)
