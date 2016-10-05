================================
Node communication via messages
================================

In this tutorial, we are going to create two nodes that are going to communicate
via messages. One node will be a publisher that generates the information,
whereas the other node will be the subscriber consuming the information. Our
nodes will be running on different processes within the same machine.

.. code-block:: bash

    mkdir ~/ign_transport_tutorial
    cd ~/ign_transport_tutorial

Publisher
======================

Download the `publisher.cc <https://bitbucket.org/ignitionrobotics/ign-transport/raw/ign-transport2/example/publisher.cc>`_ file within the ``ign_transport_tutorial``
folder and open it with your favorite editor:

.. code-block:: cpp

    #include <atomic>
    #include <chrono>
    #include <csignal>
    #include <iostream>
    #include <string>
    #include <thread>
    #include <ignition/msgs.hh>
    #include <ignition/transport.hh>

    /// \brief Flag used to break the publisher loop and terminate the program.
    static std::atomic<bool> g_terminatePub(false);

    //////////////////////////////////////////////////
    /// \brief Function callback executed when a SIGINT or SIGTERM signals are
    /// captured. This is used to break the infinite loop that publishes messages
    /// and exit the program smoothly.
    void signal_handler(int _signal)
    {
      if (_signal == SIGINT || _signal == SIGTERM)
        g_terminatePub = true;
    }

    //////////////////////////////////////////////////
    int main(int argc, char **argv)
    {
      // Install a signal handler for SIGINT and SIGTERM.
      std::signal(SIGINT,  signal_handler);
      std::signal(SIGTERM, signal_handler);

      // Create a transport node and advertise a topic.
      ignition::transport::Node node;
      std::string topic = "/foo";

      auto pubId = node.Advertise<ignition::msgs::StringMsg>(topic);
      if (!pubId)
      {
        std::cerr << "Error advertising topic [" << topic << "]" << std::endl;
        return -1;
      }

      // Prepare the message.
      ignition::msgs::StringMsg msg;
      msg.set_data("HELLO");

      // Publish messages at 1Hz.
      while (!g_terminatePub)
      {
        if (!node.Publish(pubId, msg))
          break;

        std::cout << "Publishing hello on topic [" << topic << "]" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      }

      return 0;
    }

Walkthrough
-----------

.. code-block:: cpp

    #include <ignition/msgs.hh>
    #include <ignition/transport.hh>

The line ``#include <ignition/transport.hh>`` contains all the Ignition
Transport headers for using the transport library.

The next line includes the generated protobuf code that we are going to use
for our messages. We are going to publish ``StringMsg`` type protobuf messages.

.. code-block:: cpp

    // Create a transport node and advertise a topic.
    ignition::transport::Node node;
    std::string topic = "/foo";

    auto pubId = node.Advertise<ignition::msgs::StringMsg>(topic);
    if (!pubId)
    {
      std::cerr << "Error advertising topic [" << topic << "]" << std::endl;
      return -1;
    }

First of all we declare a *Node* that will offer all the transport
functionality. In our case, we are interested on publishing topic updates, so
the first step is to announce our topic name and its type. Once a topic name is
advertised, we can start publishing periodic messages.

.. code-block:: cpp

    // Prepare the message.
    ignition::msgs::StringMsg msg;
    msg.set_data("HELLO");

    // Publish messages at 1Hz.
    while (!g_terminatePub)
    {
      if (!node.Publish(pubId, msg))
        break;

      std::cout << "Publishing hello on topic [" << topic << "]" << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

In this section of the code we create a protobuf message and fill it with
content. Next, we iterate in a loop that publishes one message every second.
The method *Publish()* sends a message to all the subscribers.

Subscriber
==========

Download the `subscriber.cc <https://bitbucket.org/ignitionrobotics/ign-transport/raw/ign-transport2/example/subscriber.cc>`_ file within the ``ign_transport_tutorial``
folder and open it with your favorite editor:

.. code-block:: cpp

    #include <iostream>
    #include <string>
    #include <ignition/msgs.hh>
    #include <ignition/transport.hh>

    //////////////////////////////////////////////////
    /// \brief Function called each time a topic update is received.
    void cb(const ignition::msgs::StringMsg &_msg)
    {
      std::cout << "Msg: " << _msg.data() << std::endl << std::endl;
    }

    //////////////////////////////////////////////////
    int main(int argc, char **argv)
    {
      ignition::transport::Node node;
      std::string topic = "/foo";

      // Subscribe to a topic by registering a callback.
      if (!node.Subscribe(topic, cb))
      {
        std::cerr << "Error subscribing to topic [" << topic << "]" << std::endl;
        return -1;
      }

      // Zzzzzz.
      ignition::transport::waitForShutdown();

      return 0;
    }


Walkthrough
-----------

.. code-block:: cpp

    //////////////////////////////////////////////////
    /// \brief Function called each time a topic update is received.
    void cb(const ignition::msgs::StringMsg &_msg)
    {
      std::cout << "Msg: " << _msg.data() << std::endl << std::endl;
    }

We need to register a function callback that will execute every time we receive
a new topic update. The signature of the callback is always similar to the one
shown in this example with the only exception of the protobuf message type.
You should create a function callback with the appropriate protobuf type
depending on the type of the topic advertised. In our case, we know that topic
``/foo`` will contain a Protobuf ``StringMsg`` type.

.. code-block:: cpp

    ignition::transport::Node node;
    std::string topic = "/foo";

    // Subscribe to a topic by registering a callback.
    if (!node.Subscribe(topic, cb))
    {
      std::cerr << "Error subscribing to topic [" << topic << "]" << std::endl;
      return -1;
    }

After the node creation, the method ``Subscribe()`` allows you to subscribe to a
given topic name by specifying your subscription callback function.


.. code-block:: cpp

    // Zzzzzz.
    ignition::transport::waitForShutdown();

If you don't have any other tasks to do besides waiting for incoming messages,
you can use the call `waitForShutdown()` that will block your current thread
until you hit *CTRL-C*. Note that this function captures the *SIGINT* and
*SIGTERM* signals.

Building the code
=================

Download the `CMakeLists.txt <https://bitbucket.org/ignitionrobotics/ign-transport/raw/ign-transport2/example/CMakeLists.txt>`_ file within the ``ign_transport_tutorial`` folder.

Once you have all your files, go ahead and create a ``build/`` directory within
the ``ign_transport_tutorial`` directory.

.. code-block:: bash

    mkdir build
    cd build

Run ``cmake`` and build the code.

.. code-block:: bash

    cmake ..
    make publisher subscriber


Running the examples
====================

Open two new terminals and from your ``build/`` directory run the executables.

From terminal 1:

.. code-block:: bash

    ./publisher

From terminal 2:

.. code-block:: bash

    ./subscriber


In your subscriber terminal, you should expect an output similar to this one,
showing that your subscriber is receiving the topic updates:

.. code-block:: bash

    caguero@turtlebot:~/ign_transport_tutorial/build$ ./subscriber
    Data: [helloWorld]
    Data: [helloWorld]
    Data: [helloWorld]
    Data: [helloWorld]
    Data: [helloWorld]
    Data: [helloWorld]
