# Ignition transport

** Ignition transport classes and functions for robot applications.**

Ignition transport is a component in the ignition framework, a set
of libraries designed to rapidly develop robot applications.

  [http://ignitionrobotics.org](http://ignitionrobotics.org)

## Continuous integration

Please refer to the [Bitbucket Pipelines](https://bitbucket.org/ignitionrobotics/ign-transport/addon/pipelines/home#!/).

## Documentation

Check [here](http://ignition-transport.readthedocs.io/en/default/).

[![Documentation Status](https://readthedocs.org/projects/ignition-transport/badge/?version=default)](https://readthedocs.org/projects/ignition-transport/?badge=default)

## Roadmap

### Ignition Transport 4.x.x

* Updated callback signature when advertising service calls
* Zero copy
* Benchmarking
* Allow `ign topic --pub` and `ign topic --echo` to work with custom messages
* Allow communication among nodes outside of the same LAN
* Request the message description to its publisher
* Security and authentication
* UDP support
* Create a wrapper for a scripting language

## Create Documentation & Release

1. Build documentation

```
cd build
make doc
```

1. Upload documentation to ignitionrobotics.org.

```
cd build
sh upload.sh
```

1. If you're creating a new release, then tell ignitionrobotics.org about
   the new version. For example:

```
curl -k -X POST -d '{"libName":"common", "version":"1.0.0", "releaseDate":"2017-10-09T12:10:13+02:00","password":"secret"}' https://api.ignitionrobotics.org/1.0/versions
```

