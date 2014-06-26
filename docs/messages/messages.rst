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
		  while (!publisher.Interrupted())
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
defined in *hello.pb.h".

The line *#include <ignition/transport/Node.hh>* contains the ignition transport
headers for using the transport library.

