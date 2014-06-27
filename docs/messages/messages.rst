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
