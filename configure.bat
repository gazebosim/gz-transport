@rem Run CMake, pointing to sibling directories containing dependencies.
@rem Note that zmq and cppzmq are relative to the source dir, while
@rem protobuf is relative to the build dir.  Not sure why.

@set build_type=Release
@if not "%1"=="" set build_type=%1

@set build_bitness=64
@if not "%2"=="" set build_bitness=%2

@set PROTOBUF_PATH=%cd%\..\..\protobuf-2.6.0-win%build_bitness%-vc12
@set ZEROMQ_PATH=%cd%\..\..\ZeroMQ 4.0.4
@set CPPZMQ_PATH=%cd%\..\..\cppzmq
@set IGNITION_MSGS_PATH=%cd%\..\..\ign-msgs\build\install\%build_type%
@set IGNITION_MATH_PATH=%cd%\..\..\ign-math\build\install\%build_type%

@echo Configuring for build type %build_type% for %build_bitness% bits
cmake -G "NMake Makefiles"^
      -DCMAKE_PREFIX_PATH="%IGNITION_MSGS_PATH%;%IGNITION_MATH_PATH%;"^
      -DIGNITION-MSGS_ROOT_DIR="%IGNITION_MSGS_PATH%"^
      -DZeroMQ_ROOT_DIR="%ZEROMQ_PATH%"^
      -DPROTOBUF_SRC_ROOT_FOLDER="%PROTOBUF_PATH%"^
      -DIGNITION-MSGS_FOLDER="%IGNITION_MSGS_PATH%"^
      -DCPPZMQ_HEADER_PATH="%CPPZMQ_PATH%"^
      -DCMAKE_INSTALL_PREFIX="install/%build_type%"^
      -DCMAKE_BUILD_TYPE="%build_type%"^
      ..

@if %errorlevel% neq 0 exit /b %errorlevel%
@echo Configuration complete.  To build, run `nmake`
