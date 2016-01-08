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
    - `ZeroMQ 3.2.4 (64-bit) <http://packages.osrfoundation.org/win32/deps/zeromq-3.2.4-amd64.zip`_

Unzip each of them. The Windows unzip utility will likely create an incorrect directory structure, where a directory with the name of the zip contains the directory that has the source files. Here is an example:

.. code-block:: bash

    ign-ws/cppzmq-noarch/cppzmq

    The correct structure is

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

You should now have an installation of ign-transport in ign-ws/ign-transport/build/install.

Before running any executables, you need to modify your `PATH` to include the `bin` subdirectory of ZeroMQ to let Windows find dynamic libs (similar to `LD_LIBRARY_PATH` on Linux).  Don't put quotes around the path, even if it contains spaces.  E.g., if you're working in ``C:\My Stuff\ign-ws``:

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