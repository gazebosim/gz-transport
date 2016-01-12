============
Installation
============

Ubuntu Linux
============

Setup your computer to accept software from *packages.osrfoundation.org*:

.. code-block:: bash

    sudo sh -c 'echo "deb http://packages.osrfoundation.org/gazebo/ubuntu-stable
    `lsb_release -cs` main" > /etc/apt/sources.list.d/gazebo-stable.list'

Setup keys:

.. code-block:: bash

    wget http://packages.osrfoundation.org/gazebo.key -O - | sudo apt-key add -

Install Ignition Transport:

.. code-block:: bash

    sudo apt-get update
    sudo apt-get install libignition-transport0-dev

Mac OS X
========

Ignition Transport and several of its dependencies can be compiled on OS X with
`Homebrew <http://brew.sh/>`_ using the
`osrf/simulation tap <https://github.com/osrf/homebrew-simulation>`_. Ignition
Transport is straightforward to install on Mac OS X 10.9 (Mavericks) or higher.
Installation on older versions requires changing the default standard library
and rebuilding dependencies due to the use of c++11. For purposes of this
documentation, I will assume OS X 10.9 or greater is in use. Here are the
instructions:

Install homebrew, which should also prompt you to install the XCode command-line tools:

.. code-block:: bash

    ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"

Run the following commands:

.. code-block:: bash

    brew tap osrf/simulation
    brew install ignition-transport

Windows
=======

At this moment, compilation has been tested on Windows 7 and 8.1 and is
supported when using
`Visual Studio 2013 <https://www.visualstudio.com/downloads/>`_. Patches for
other versions are welcome.

This installation procedure uses pre-compiled binaries in a local workspace. To make things easier, use a MinGW shell for your editing work (such as the
`Git Bash Shell <https://msysgit.github.io/>`_ with
`Mercurial <http://tortoisehg.bitbucket.org/download/index.html>`_), and only
use the Windows cmd for configuring and building. You might also need to
`disable the Windows firewall <http://windows.microsoft.com/en-us/windows/turn-windows-firewall-on-off#turn-windows-firewall-on-off=windows-7>`_.

Make a directory to work in, e.g.:

.. code-block:: bash

    mkdir ign-ws
    cd ign-ws

Download the following dependencies into that directory:

    - `cppzmq <http://packages.osrfoundation.org/win32/deps/cppzmq-noarch.zip>`_
    - `Protobuf 2.6.0 (32-bit) <http://packages.osrfoundation.org/win32/deps/protobuf-2.6.0-win32-vc12.zip>`_
    - `Protobuf 2.6.0 (64-bit) <http://packages.osrfoundation.org/win32/deps/protobuf-2.6.0-win64-vc12.zip>`_

    Choose one of these options:

    - `ZeroMQ 3.2.4 (32-bit) <http://packages.osrfoundation.org/win32/deps/zeromq-3.2.4-x86.zip>`_
    - `ZeroMQ 3.2.4 (64-bit) <http://packages.osrfoundation.org/win32/deps/zeromq-3.2.4-amd64.zip>`_

Unzip each of them. The Windows unzip utility will likely create an incorrect directory structure, where a directory with the name of the zip contains the directory that has the source files. Here is an example:

.. code-block:: bash

    ign-ws/cppzmq-noarch/cppzmq

The correct structure is

.. code-block:: bash

        ign-ws/cppzmq

To fix this problem, manually move the nested directories up one level.

Clone ign-transport:

.. code-block:: bash

        hg clone https://bitbucket.org/ignitionrobotics/ign-transport
        cd ign-transport

In a Windows Command Prompt, load your compiler setup, e.g.:

.. code-block:: bash

        "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" amd64

Configure and build:


.. code-block:: bash

        mkdir build
        cd build
        ..\configure
        nmake
        nmake install

You should now have an installation of ign-transport in ``ign-ws/ign-transport/build/install``.

Before running any executables, you need to modify your ``PATH`` to include the ``bin`` subdirectory of ZeroMQ to let Windows find dynamic libs (similar to ``LD_LIBRARY_PATH`` on Linux). Don't put quotes around the path, even if it contains spaces.  E.g., if you're working in ``C:\My Stuff\ign-ws``:

.. code-block:: bash

        set PATH %PATH%;C:\My Stuff\ign-ws\ZeroMQ 3.2.4\bin

Now build the examples:

.. code-block:: bash

        cd ign-ws\ign-transport\example
        mkdir build
        cd build
        ..\configure
        nmake

Now try an example. In one Windows terminal run:

.. code-block:: bash

        responser

In another Windows terminal run:

.. code-block:: bash

        requester


Install from sources (Ubuntu Linux)
=======

For compiling the latest version of Ignition Transport you will need an Ubuntu
distribution equal to 14.04.2 (Trusty) or newer.

Make sure you have removed the Ubuntu pre-compiled binaries before installing
from source:

.. code-block:: bash

        sudo apt-get remove libignition-transport0-dev

Install prerequisites. A clean Ubuntu system will need:

.. code-block:: bash

        sudo apt-get install cmake pkg-config python ruby-ronn libprotoc-dev libprotobuf-dev protobuf-compiler uuid-dev libzmq3-dev

Clone the repository into a directory and go into it:

.. code-block:: bash

        hg clone https://bitbucket.org/ignitionrobotics/ign-transport /tmp/ign-transport
        cd /tmp/ign-transport

Create a build directory and go there:

.. code-block:: bash

        mkdir build
        cd build

Configure Ignition Transport (choose either method a or b below):

a. Release mode: This will generate optimized code, but will not have debug symbols. Use this mode if you don't need to use GDB.

.. code-block:: bash

        cmake ../

Note: You can use a custom install path to make it easier to switch between source and debian installs:

.. code-block:: bash

        cmake -DCMAKE_INSTALL_PREFIX=/home/$USER/local ../

b. Debug mode: This will generate code with debug symbols. Ignition Transport
will run slower, but you'll be able to use GDB.

.. code-block:: bash

        cmake -DCMAKE_BUILD_TYPE=Debug ../

The output from ``cmake ../`` may generate a number of errors and warnings about
missing packages. You must install the missing packages that have errors and
re-run ``cmake ../``. Make sure all the build errors are resolved before
continuing (they should be there from the earlier step in which you installed
prerequisites).

Make note of your install path, which is output from cmake and should look something like:

.. code-block:: bash

        -- Install path: /home/$USER/local

Build Ignition Transport:

.. code-block:: bash

        make -j4

Install Ignition Transport:

.. code-block:: bash

        sudo make install

If you decide to install gazebo in a local directory you'll need to modify your
``LD_LIBRARY_PATH``:

.. code-block:: bash

        echo "export LD_LIBRARY_PATH=<install_path>/lib:$LD_LIBRARY_PATH" >> ~/.bashrc

If you need to uninstall Ignition Transport or switch back to a debian-based
install when you currently have installed the library from source, navigate to
your source code directory's build folders and run ``make uninstall``:

.. code-block:: bash

        cd /tmp/ign-transport/build
        sudo make uninstall

Install from sources (MAC OS X)
=======

