@rem Run CMake, pointing to sibling directories containing dependencies.
@rem Note that zmq and cppzmq are relative to the source dir, while
@rem protobuf is relative to the build dir.  Not sure why.
@set build_type=Release
@if not "%1"=="" set build_type=%1
@set build_bitness=32
@if not "%2"=="" set build_bitness=%2
@echo Configuring for build type %build_type%
cmake -DZeroMQ_ROOT_DIR="..\..\ZeroMQ 3.2.4"^
      -DPROTOBUF_SRC_ROOT_FOLDER="..\..\..\protobuf-2.6.0-win%build_bitness%-vc12"^
      -DCPPZMQ_HEADER_PATH="..\..\cppzmq"^
      -DCMAKE_INSTALL_PREFIX="install"^
      -DCMAKE_PREFIX_PATH="..\build\install\%build_type%\lib\cmake\ignition-transport"^
      -G "NMake Makefiles"^
      -DCMAKE_BUILD_TYPE=%build_type%^
      ..
@if %errorlevel% neq 0 exit /b %errorlevel%
@echo Configuration complete.  To build, run `nmake`
