@rem Run CMake, pointing to sibling directories containing dependencies.
@rem Note that zmq and cppzmq are relative to the source dir, while 
@rem protobuf is relative to the build dir.  Not sure why.
cmake -DZeroMQ_ROOT_DIR="..\..\ZeroMQ 3.2.4" -DPROTOBUF_SRC_ROOT_FOLDER="..\..\..\protobuf-2.6.0-win32-vc12" -DCPPZMQ_HEADER_PATH="..\..\cppzmq" -DCMAKE_INSTALL_PREFIX="install" -DCMAKE_PREFIX_PATH="../build/install" -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release .. 
@if %errorlevel% neq 0 exit /b %errorlevel%
@echo Configuration complete.  To build, run `nmake`
