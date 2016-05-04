@rem Run CMake, pointing to sibling directories containing dependencies.
@rem Note that zmq and cppzmq are relative to the source dir, while
@rem protobuf is relative to the build dir.  Not sure why.

@set build_type=Release
@if not "%1"=="" set build_type=%1

@set build_bitness=64
@if not "%2"=="" set build_bitness=%2

@set ZEROMQ_PATH=%cd%\..\..\ZeroMQ 3.2.4
@set CPPZMQ_PATH=%cd%\..\..\cppzmq
@set IGNITION-MSGS_PATH=%cd%\..\..\ign-msgs\build\install\%build_type%
@set IGNITION-MSGS_CMAKE_PREFIX_PATH=%IGNITION-MSGS_PATH%\CMake
@set IGNITION-MATH_PATH=%cd%\..\..\ign-math\build\install\%build_type%
@set IGNITION-MATH_CMAKE_PREFIX_PATH=%IGNITION-MATH_PATH%\CMake

@echo Configuring for build type %build_type% for %build_bitness% bits
cmake -G "NMake Makefiles"^
      -DCMAKE_PREFIX_PATH="%IGNITION-MSGS_CMAKE_PREFIX_PATH%;%IGNITION-MATH_CMAKE_PREFIX_PATH%;"^
      -DZeroMQ_ROOT_DIR="%ZEROMQ_PATH%"^
      -DCPPZMQ_HEADER_PATH="%CPPZMQ_PATH%"^
      -DCMAKE_INSTALL_PREFIX="install/%build_type%"^
      -DCMAKE_BUILD_TYPE="%build_type%"^
      ..

@if %errorlevel% neq 0 exit /b %errorlevel%
@echo Configuration complete.  To build, run `nmake`
