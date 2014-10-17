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
    - [Download protobuf 2.6.0 code](https://protobuf.googlecode.com/svn/rc/protobuf-2.6.0.zip)
    - Open protobuf.sln in Microsoft Visual Studio
    - Choose "Release" configuration in the menu of the VS GUI
    - To work around an apparent parallel build problem: From the "Solution Explorer" window, shift-select all 9 projects, then Alt-F7 to modify properties, then "Configuration Properties"->"C/C++"->"Command Line" and in the "Additional Options" box, add "/FS" [docs](http://msdn.microsoft.com/en-us/library/dn502518.aspx)
    - From the Build menu, choose "Build Solution". 
    - Wait for compiling to finish.
    - Add protoc.exe to your PATH. (Control Panel > System > Advanced > Enviroment variable)    
      <path_to_protobuf-2.6.0\vsprojects\Release

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

## Alternative approach to building

Totally experimental, using pre-compiled binaries in a local workspace.

 1. Make a directory to work in, e.g.:

    mkdir ign-ws
    cd ign-ws

 1. Download the following things:

  - [cppzmq](http://packages.osrfoundation.org/win32/deps/cppzmq.zip)
  - [zeromq](http://packages.osrfoundation.org/win32/deps/zeromq-3.2.4.zip)
  - [protobuf](http://packages.osrfoundation.org/win32/deps/protobuf-2.6.0-win32-vc12.zip)

 1. Unzip each of them.

 1. Clone ign-transport:

    hg clone https://bitbucket.org/ignitionrobotics/ign-transport
    cd ign-transport

 1. Load your compiler setup, e.g.:

    "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x86

 1. Run CMake via the supplied configure.bat file (modify as needed):

    configure

 1. Build and install, using nmake:
 
    nmake
    nmake install

You should now have an installation of ign-transport in ign-ws/ign-transport/build/install.
