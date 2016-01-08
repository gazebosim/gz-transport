============
Installation
============

Ubuntu Linux
============

Setup your computer to accept software from *packages.osrfoundation.org*:

.. code-block:: cpp

    sudo sh -c 'echo "deb http://packages.osrfoundation.org/gazebo/ubuntu-stable
    `lsb_release -cs` main" > /etc/apt/sources.list.d/gazebo-stable.list'

Setup keys:

.. code-block:: cpp

    wget http://packages.osrfoundation.org/gazebo.key -O - | sudo apt-key add -

Install Ignition Transport:

.. code-block:: cpp

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

.. code-block:: cpp

    ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"

Run the following commands:

.. code-block:: cpp

    brew tap osrf/simulation
    brew install ignition-transport

Windows
=======
