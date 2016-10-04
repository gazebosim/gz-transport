# Installation on Windows

## Supported compilers

At this moment, compilation has been tested on Windows 7&10, and is supported
when using Visual Studio 2013. Patches for other versions are welcome.

## Installation

Totally experimental, using pre-compiled binaries in a local workspace.  To make things easier, use a MinGW shell for your editing work, and only use the
Windows `cmd` for configuring and building.  You might also need to [disable the Windows firewall](http://windows.microsoft.com/en-us/windows/turn-windows-firewall-on-off#turn-windows-firewall-on-off=windows-7).  Not sure about that.

1. Install cmake, make sure to select the "Add CMake to system path for all users" option in the install dialog box.

    [CMake](https://cmake.org/download)

1. Make a directory to work in, e.g.:

        mkdir ign-ws
        cd ign-ws

1. Download the following things into that directory:

    - [cppzmq](http://packages.osrfoundation.org/win32/deps/cppzmq-noarch.zip)
    - [Protobuf 2.6.0 (32-bit)](http://packages.osrfoundation.org/win32/deps/protobuf-2.6.0-win32-vc12.zip)
    - [Protobuf 2.6.0 (64-bit)](http://packages.osrfoundation.org/win32/deps/protobuf-2.6.0-win64-vc12.zip)

    Choose one of these options:

    - [ZeroMQ 4.0.4 (32-bit)](http://packages.osrfoundation.org/win32/deps/zeromq-4.0.4-x86.zip)
    - [ZeroMQ 4.0.4 (64-bit)](http://packages.osrfoundation.org/win32/deps/zeromq-4.0.4-amd64.zip)

1. Unzip each of them. The Windows unzip utility will likely create an incorrect directory structure, where a directory with the name of the zip contains the directory that has the source files. Here is an example:

        ign-ws/cppzmq-noarch/cppzmq

    The correct structure is

        ign-ws/cppzmq

    To fix this problem, manually move the nested directories up one level.

1. Clone ign-transport:

        hg clone https://bitbucket.org/ignitionrobotics/ign-transport
        cd ign-transport

1. In a Windows Command Prompt, load your compiler setup, e.g.:

        "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" amd64

1. Configure and build:


        mkdir build
	    cd build
        ..\configure
        nmake
        nmake install


    You should now have an installation of ign-transport in ign-ws/ign-transport/build/install.

1. Before running any executables, you need to modify your `PATH` to include the `bin` subdirectory of ZeroMQ to let Windows find dynamic libs (similar to `LD_LIBRARY_PATH` on Linux).  Don't put quotes around the path, even if it contains spaces.  E.g., if you're working in `C:\My Stuff\ign-ws`:

        set PATH %PATH%;C:\My Stuff\ign-ws\ZeroMQ 4.0.4\bin

1. Now build the examples:

        cd ign-ws\ign-transport\example
        mkdir build
        cd build
        ..\configure
        nmake

1. Now try an example:

    1. In one Windows terminal run:
    
        responser
        
    1. In another Windows terminal run:
    
        requester
        

## Alternative installation: dependencies from upstream and installed on the system

### Dependencies

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

### Configuration (cmake)

    "C:\Program Files(x86)\Microsft Visual Studio 12\VC\vcvarsall.bat" x86
    cmake .. -G"Visual Studio 12" -DZeroMQ_ROOT_DIR="C:\Program Files (x86)\ZeroMQ 4.0.4" -DPROTOBUF_SRC_ROOT_FOLDER="C:\<path_to_protobuf-2.6.0>" -DCPPZMQ_HEADER_PATH="C:\<path_to_cppzmq>"

### Compilation

    msbuild /p:Configuration=Release ALL_BUILD.vcxproj

### Run tests

    set PATH=%PATH%;c:\Program Files (x86)\ZeroMQ 4.0.4\bin
    ctest -C "Release" --extra-verbose
