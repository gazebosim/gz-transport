================================
Node communication via messages.
================================

In this tutorial, we are going to create two nodes that are going to communicate
via messages. One node will be a publisher that generates the information,
whereas the other node will be the subscriber consuming the information. Our
nodes will be running on different processes within the same machine.

::

    mkdir ~/ign_transport_tutorial
    cd ~/ign_transport_tutorial
    mkdir src

Snippets
========

.. literalinclude:: ../../src/Discovery.cc
   :lines: 1,3,5-10,20-
   :emphasize-lines: 20-25

Creating the publisher
======================

Create the *src/publisher.cc* file within the *ign_transport_tutorial* and paste
the following code inside it:

::

    #include "../build/src/hello.pb.h"
    #include <ignition/transport/Node.hh>
    #include <string>

    using namespace ignition;

    //////////////////////////////////////////////////
    int main(int argc, char **argv)
    {
      std::string topic = "topicA";
      std::string data = "helloWorld";

      // Create a transport node.
      transport::Node publisher;

      // Advertise a topic.
      publisher.Advertise(topic);

      // Prepare the message.
      tutorial::Hello msg;
      msg.set_content(data);

      // Publish messages at 1Hz.
      while (true)
      {
        publisher.Publish(topic, msg);
        sleep(1);
      }
    }

Walkthrough
===========

::

    #include "../build/src/hello.pb.h"
    #include <ignition/transport/Node.hh>

The first line includes the generated protobuf code that we are going to use
for the publisher. We are going to publish *Hello* type protobuf messages
defined in *hello.pb.h*.

The line *#include <ignition/transport/Node.hh>* contains the ignition transport
headers for using the transport library.

::

    // Create a transport node.
    transport::Node publisher;
    // Advertise a topic.
    publisher.Advertise(topic);

First of all we declare a *Node* that will offer all the transport
functionality. In our case, we are interested on publishing topic updates, so
the first step is to announce our topic name. Once a topic name is advertised,
we can start publishing periodic messages.

::

    // Prepare the message.
    tutorial::Hello msg;
    msg.set_content(data);

    // Publish messages at 1Hz.
    while (true)
    {
      publisher.Publish(topic, msg);
      sleep(1);
    }

In this section of the code we create a protobuf message and fill it with
content. Next, we create an infinite loop for publishing messages every second.
The method *Publish()* sends a message to all the subscribers.

Creating the subscriber
=======================

Create the *src/subscriber.cc* file within the *ign_transport_tutorial* and
paste the following code inside it:

::

    #include "../build/src/hello.pb.h"
    #include <ignition/transport/Node.hh>
    #include <cstdio>
    #include <string>

    using namespace ignition;

    //////////////////////////////////////////////////
    /// \brief Function called each time a topic update is received.
    void cb(const std::string &_topic, const tutorial::Hello &_msg)
    {
      std::cout << "Data: [" << _msg.content() << "]" << std::endl;
    }

    //////////////////////////////////////////////////
    int main(int argc, char **argv)
    {
      std::string topic = "topicA";

      // Create a transport node.
      transport::Node publisher;

      // Subscribe to a topic by registering a callback.
      publisher.Subscribe(topic, cb);

      // Wait until the user press <ENTER>.
      getchar();
    }


Walkthrough
===========

::

    //////////////////////////////////////////////////
    /// \brief Function called each time a topic update is received.
    void cb(const std::string &_topic, const tutorial::Hello &_msg)
    {
      std::cout << "Data: [" << _msg.content() << "]" << std::endl;
    }

We are going to need to register a function callback that will execute every
time we receive a new topic update. The signature of the callback is always
similar to the one shown in this example with the only exception of the protobuf
message type. You should create a function callback with the appropriate
protobuf type depending on the type advertised in your topic of interest. In our
case, we know that topic */topicA* will contain a protobuf *Hello* type.

::

    // Create a transport node.
    transport::Node publisher;

    // Subscribe to a topic by registering a callback.
    publisher.Subscribe(topic, cb);

After the node creation, the method *Subscribe()* allows you to subscribe to a
given topic name by specifying your subscription callback function.


Building the code
=================

Copy this *CMakeLists.txt* file within the *ign_transport_tutorial*. This is the
top level cmake file that will check for dependencies.

Copy this *hello.proto* file within the *ign_transport_tutorial/src*. This is
the protobuf message definition that we use in this example.

Copy this *CMakeLists.txt* file within the *ign_transport_tutorial/src*. This is
the cmake file that will generate the c++ code from the protobuf file and will
create the *publisher* and *subscriber* executables.

Once you have all your files, go ahead and create a *build/* directory within
the *ign_transport_tutorial* directory.

::

    mkdir build
    cd build

Run *cmake* and build the code.

::

    cmake ..
    make


Running the examples
====================

Open two new terminals and from your *build/* directory run the executables:

From terminal 1::

    ./publisher

From terminal 2::

    ./subscriber


In your subscriber terminal, you should expect an output similar to this one,
showing that your subscribing is receiving the topic updates:

::

    caguero@turtlebot:~/ign_transport_tutorial/build$ ./subscriber
    Data: [helloWorld]
    Data: [helloWorld]
    Data: [helloWorld]
    Data: [helloWorld]
    Data: [helloWorld]
    Data: [helloWorld]