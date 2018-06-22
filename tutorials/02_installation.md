# Installation

This page contains instructions to install Ignition Transport on all the
support platforms including major Linux distributions, Mac OS X and Windows.

You can find all Ignition Transport versions at [https://ignitionrobotics.org/libs/transport](https://ignitionrobotics.org/libs/transport).

## Ubuntu Linux

Setup your computer to accept software from
*packages.osrfoundation.org*:

```{.sh}
sudo sh -c 'echo "deb http://packages.osrfoundation.org/gazebo/ubuntu-stable `lsb_release -cs` main" > /etc/apt/sources.list.d/gazebo-stable.list'
```

Setup keys:

```
wget http://packages.osrfoundation.org/gazebo.key -O - | sudo apt-key add -
```

Install Ignition Transport:

```
sudo apt-get update
sudo apt-get install libignition-transport4-dev
```

## Mac OS X

Ignition Transport and several of its dependencies can be compiled on OS
X with [Homebrew](http://brew.sh/) using the [osrf/simulation
tap](https://github.com/osrf/homebrew-simulation). Ignition Transport is
straightforward to install on Mac OS X 10.9 (Mavericks) or higher.
Installation on older versions requires changing the default standard
library and rebuilding dependencies due to the use of c++11. For
purposes of this documentation, I will assume OS X 10.9 or greater is in
use. Here are the instructions:

Install homebrew, which should also prompt you to install the XCode
command-line tools:

```
ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
```

Run the following commands:

```
brew tap osrf/simulation
brew install ignition-transport4
```

## Windows

At this moment, compilation has been tested on Windows 7 and 8.1 and is
supported when using [Visual Studio
2013](https://www.visualstudio.com/downloads/). Patches for other
versions are welcome.

This installation procedure uses pre-compiled binaries in a local
workspace. To make things easier, use a MinGW shell for your editing
work (such as the [Git Bash Shell](https://msysgit.github.io/) with
[Mercurial](http://tortoisehg.bitbucket.org/download/index.html)), and
only use the Windows cmd for configuring and building. You might also
need to [disable the Windows
firewall](http://windows.microsoft.com/en-us/windows/turn-windows-firewall-on-off#turn-windows-firewall-on-off=windows-7).

Make a directory to work in, e.g.:

```
mkdir ign-ws
cd ign-ws
```

Download the following dependencies into that directory:

  * [cppzmq](http://packages.osrfoundation.org/win32/deps/cppzmq-noarch.zip)
  * [Protobuf 2.6.0 (32-bit)](http://packages.osrfoundation.org/win32/deps/protobuf-2.6.0-win32-vc12.zip)
  * [Protobuf 2.6.0 (64-bit)](http://packages.osrfoundation.org/win32/deps/protobuf-2.6.0-win64-vc12.zip)

Choose one of these options:

  * [ZeroMQ 4.0.4 (32-bit)](http://packages.osrfoundation.org/win32/deps/zeromq-4.0.4-x86.zip)
  * [ZeroMQ 4.0.4 (64-bit)](http://packages.osrfoundation.org/win32/deps/zeromq-4.0.4-amd64.zip)

Unzip each of them. The Windows unzip utility will likely create an
incorrect directory structure, where a directory with the name of the
zip contains the directory that has the source files. Here is an
example:

```
ign-ws/cppzmq-noarch/cppzmq
```

The correct structure is

```
ign-ws/cppzmq
```

To fix this problem, manually move the nested directories up one level.

Clone and prepare the Ignition Math dependency:

```
hg clone https://bitbucket.org/ignitionrobotics/ign-math
cd ign-math
mkdir build
```

In a Windows Command Prompt, load your compiler setup, e.g.:

```
"C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" amd64
```

In the Windows Command Prompt, configure and build:

```
cd ign-math\build
..\configure
nmake install
```

Clone and prepare the Ignition Msgs dependency:

```
hg clone https://bitbucket.org/ignitionrobotics/ign-msgs
cd ign-msgs
mkdir build
```

In the Windows Command Prompt, configure and build:

```
cd ign-msgs\build
..\configure
nmake install
```

Clone ign-transport:

```
hg clone https://bitbucket.org/ignitionrobotics/ign-transport
cd ign-transport
```

In a Windows Command Prompt, load your compiler setup, e.g.:

```
"C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" amd64
```

Configure and build:

```
mkdir build
cd build
..\configure
nmake
nmake install
```

You should now have an installation of ign-transport in
`ign-ws/ign-transport/build/install`.

Before running any executables, you need to modify your `PATH` to
include the `bin` subdirectory of ZeroMQ to let Windows find dynamic
libs (similar to `LD_LIBRARY_PATH` on Linux). Don't put quotes around
the path, even if it contains spaces. E.g., if you're working in
`C:\My Stuff\ign-ws`:

```
set PATH %PATH%;C:\My Stuff\ign-ws\ZeroMQ 4.0.4\bin
```

Now build the examples:

\code
cd ign-ws\ign-transport\example
mkdir build
cd build
..\configure
nmake
\endcode

Now try an example. In one Windows terminal run:

```
responser
```

In another Windows terminal run:

```
requester
```

### Install from sources (Ubuntu Linux)

For compiling the latest version of Ignition Transport you will need an
Ubuntu distribution equal to 14.04.2 (Trusty) or newer.

Make sure you have removed the Ubuntu pre-compiled binaries before
installing from source:

```
sudo apt-get remove libignition-transport4-dev
```

Install prerequisites. A clean Ubuntu system will need:

```
sudo apt-get install mercurial cmake pkg-config python ruby-ronn libprotoc-dev libprotobuf-dev protobuf-compiler uuid-dev libzmq3-dev libignition-msgs-dev
```

Clone the repository into a directory and go into it:

```
hg clone https://bitbucket.org/ignitionrobotics/ign-transport /tmp/ign-transport
cd /tmp/ign-transport
```

Create a build directory and go there:

```
mkdir build
cd build
```

Configure Ignition Transport (choose either method a or b below):

> A.  Release mode: This will generate optimized code, but will not have
>     debug symbols. Use this mode if you don't need to use GDB.
>
> ```
> cmake ../
> ```
>
> Note: You can use a custom install path to make it easier to switch
> between source and debian installs:
>
> ```
> cmake -DCMAKE_INSTALL_PREFIX=/home/$USER/local ../
> ```
>
> B. Debug mode: This will generate code with debug symbols. Ignition
> Transport will run slower, but you'll be able to use GDB.
>
> ```
> cmake -DCMAKE_BUILD_TYPE=Debug ../
> ```

The output from `cmake ../` may generate a number of errors and warnings
about missing packages. You must install the missing packages that have
errors and re-run `cmake ../`. Make sure all the build errors are
resolved before continuing (they should be there from the earlier step
in which you installed prerequisites).

Make note of your install path, which is output from cmake and should
look something like:

```
-- Install path: /home/$USER/local
```

Build Ignition Transport:

```
make -j4
```

Install Ignition Transport:

```
sudo make install
```

If you decide to install gazebo in a local directory you'll need to
modify your `LD_LIBRARY_PATH`:

```
echo "export LD_LIBRARY_PATH=<install_path>/local/lib:$LD_LIBRARY_PATH" >> ~/.bashrc
```

### Uninstalling Source-based Install

If you need to uninstall Ignition Transport or switch back to a
debian-based install when you currently have installed the library from
source, navigate to your source code directory's build folders and run
`make uninstall`:

\code
cd /tmp/ign-transport/build
sudo make uninstall
\endcode
