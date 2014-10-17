@echo "NOTE: You should have already run this command:"
@echo "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x86
mkdir build
@if %errorlevel% neq 0 exit /b %errorlevel%
cd build
@rem Run CMake, pointing to sibling directories containing dependencies.
@rem Note that zmq and cppzmq are relative to the source dir, while 
@rem protobuf is relative to the build dir.  Not sure why.
cmake -DZeroMQ_ROOT_DIR="..\ZeroMQ 3.2.4" -DPROTOBUF_SRC_ROOT_FOLDER="..\..\protobuf-2.6.0-win32-vc12" -DCPPZMQ_HEADER_PATH="..\cppzmq" -DCMAKE_INSTALL_PREFIX="install" -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release .. 
@if %errorlevel% neq 0 exit /b %errorlevel%
@echo ""
@echo ""
@echo Configuration complete.  To build, run `nmake`
