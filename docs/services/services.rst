================================
Node communication via services.
================================

In this tutorial, we are going to create two nodes that are going to communicate
via services. One node will be a service provider that offers an *echo* service,
whereas the other node will be the service consumer requesting an *echo* call.

::

    mkdir ~/ign_transport_tutorial
    cd ~/ign_transport_tutorial
    mkdir src

Creating the responser
======================

Create the *src/responser.cc* file within the *ign_transport_tutorial* and paste
the following code inside it:

::

		#include "../build/src/hello.pb.h"
		#include <ignition/transport/Node.hh>
		#include <cstdio>
		#include <string>

		using namespace ignition;

		//////////////////////////////////////////////////
		/// \brief Provide an "echo" service.
		void srvEcho(const std::string &_topic, const tutorial::Hello &_req,
		  tutorial::Hello &_rep, bool &_result)
		{
		  // Set the response's content.
		  _rep.set_content(_req.content());

		  // The response succeed.
		  _result = true;
		}

		//////////////////////////////////////////////////
		int main(int argc, char **argv)
		{
		  std::string topic = "echo";

		  // Create a transport node.
		  transport::Node publisher;

		  // Advertise a service call.
		  publisher.Advertise(topic, srvEcho);

		  // Wait for requests.
		  getchar();
		}

Walkthrough
===========

::

    #include "../build/src/hello.pb.h"
    #include <ignition/transport/Node.hh>

The first line includes the generated protobuf code that we are going to use
for the responser. We are going to use *Hello* type protobuf messages
defined in *hello.pb.h* for our services.

The line *#include <ignition/transport/Node.hh>* contains the ignition transport
headers for using the transport library.

::

		void srvEcho(const std::string &_topic, const tutorial::Hello &_req,
		  tutorial::Hello &_rep, bool &_result)
		{
		  // Set the response's content.
		  _rep.set_content(_req.content());

		  // The response succeed.
		  _result = true;
		}

As a service provider, our node needs to register a function callback that will
execute every time a new service request is received. The signature of the
callback is always similar to the one shown in this example with the exception
of the protobuf messages types for the *_req* (request) and *_rep* (response).
The request parameter contains the input parameters of the request. The response message contains any resulting data from the service call. The result parameter denotes if the overall service call was considered successful or not. In our
example, as a simple *echo* service, we just fill the response with the same
data contained in the request.

::

	  // Create a transport node.
		transport::Node publisher;

		// Advertise a service call.
		publisher.Advertise(topic, srvEcho);

We declare a *Node* that will offer all the transport functionality. In our
case, we are interested on offering a service, so the first step is to announce
our service name. Once a service name is advertised, we can accept service
requests.


Creating the requester
=======================

Create the *src/requester.cc* file within the *ign_transport_tutorial* and
paste the following code inside it:

::

    #include "../build/src/hello.pb.h"
		#include <ignition/transport/Node.hh>
		#include <string>

		using namespace ignition;

		//////////////////////////////////////////////////
		/// \brief Service call response callback.
		void responseCb(const std::string &_topic, const tutorial::Hello &_rep,
		  bool _result)
		{
		  if (_result)
		    std::cout << "Response: [" << _rep.content() << "]" << std::endl;
		  else
		    std::cerr << "Service call failed" << std::endl;
		}

		//////////////////////////////////////////////////
		int main(int argc, char **argv)
		{
		  std::string topic = "echo";

		  // Create a transport node.
		  transport::Node requester;

		  // Prepare the service call input parameter.
		  tutorial::Hello req;
		  req.set_content("Hello World!");

			// Request an asynchronous service call.
		  requester.Request(topic, req, responseCb);

		  getchar();
		}


Walkthrough
===========

::

    //////////////////////////////////////////////////
		/// \brief Service call response callback.
		void responseCb(const std::string &_topic, const tutorial::Hello &_rep,
		  bool _result)
		{
		  if (_result)
		    std::cout << "Response: [" << _rep.content() << "]" << std::endl;
		  else
		    std::cerr << "Service call failed" << std::endl;
		}

We are going to need to register a function callback that will execute every
time we receive a new topic update. The signature of the callback is always
similar to the one shown in this example with the only exception of the protobuf
message type. You should create a function callback with the appropriate
protobuf type depending on the type advertised in your topic of interest. In our
case, we know that topic */topicA* will contain a protobuf *Hello* type.

::

    // Prepare the service call input parameter.
		tutorial::Hello req;
		req.set_content("Hello World!");

		// Request an asynchronous service call.
		requester.Request(topic, req, responseCb);


In this section of the code we create a protobuf message and fill it with the
input parameters for the request. Next, we just use the method *Request()* that
will make a service call to any service provider of the service */echo*.
Ignition transport will find a node, communicate the data, capture the response
and pass it to your callback, in addition of the service call result. Note that
this variant of *Request()* is asynchronous, so your code will not block while
your service request is handled.

Building the code
=================

Copy this *CMakeLists.txt* file within the *ign_transport_tutorial*. This is the
top level cmake file that will check for dependencies.

Copy this *hello.proto* file within the *ign_transport_tutorial/src*. This is
the protobuf message definition that we use in this example.

Copy this *CMakeLists.txt* file within the *ign_transport_tutorial/src*. This is
the cmake file that will generate the c++ code from the protobuf file and will
create the *responser* and *requester* executables.

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

    ./responser

From terminal 2::

    ./requester


In your requester terminal, you should expect an output similar to this one,
showing that your requester has received the data:

::

    caguero@turtlebot:~/ign_transport_tutorial/build$ ./requester
    Response: [Hello World!]
