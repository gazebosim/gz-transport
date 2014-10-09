# Installation on Windows

## Supported compilers

At this moment, compilation has been testewd on Windows 7 and is supported 
when using Visual Studio 2013. Patches for other versions are welcome.

## Dependencies

 - ZeroMQ Libraries
    - [ZeroMQ 4.0.4 32bits installer](http://miru.hk/archive/ZeroMQ-4.0.4~miru1.0-x86.exe)
    - [ZeroMQ 4.0.4 64bits installer](http://miru.hk/archive/ZeroMQ-4.0.4~miru1.0-x64.exe)

 - cppzmq header file
    - [Download it](https://github.com/zeromq/cppzmq) and place at any folder in your system
 
 - Protobuf compiled from source (there are no binaries)
    - [Download protobuf 2.6.0 code](https://protobuf.googlecode.com/svn/rc/protoc-2.6.0-win32.zip)
    - Open protobuf.sln in Microsoft Visual Studio
    - Choose "Release" configuration in the menu of the VS GUI
    - From the Build menu, choose "Build Solution". 
    - Wait for compiling to finish. Some errors could appear about compiling -lite libraries 
      and test. Ignore them for now.
    - Add protoc.exe to your PATH. (Control Panel > System > Advanced > Enviroment variable)    
      protoc-2.6.0\vc

## Configuration (cmake)

### 32bits compilation

    "C:\Program Files(x86)\Microsft Visual Studio 12\VC\vcvarsall.bat" x86
    cmake .. -G"Visual Studio 12" -DZeroMQ_ROOT_DIR="C:\Program Files (x86)\ZeroMQ 4.0.4" -DPROTOBUF_SRC_ROOT_FOLDER="C:\<path_to_protobuf-2.6.0>" -DCPPZMQ_HEADER_PATH="C:\<path_to_cppzmq>"

### 64 bits compilation

   Currently, FindProtobuf.cmake is not support 64bits compilations since the
   binaries are placed under vsprojects\x64\Release and that PATH is not being
   used by the cmake module.

## Compilation
 
    msbuild /p:Configuration=Release ALL_BUILD.vcxproj

## Run tests

    set PATH=%PATH%;c:\Program Files (x86)\ZeroMQ 4.0.4\bin
    ctest -C "Release" --extra-verbose
