================================
Node communication via services
================================

In this tutorial, we are going to create two nodes that are going to communicate
via services. One node will be a service provider that offers an *echo* service,
whereas the other node will be the service consumer requesting an *echo* call.

.. code-block:: bash

    mkdir ~/ign_transport_tutorial
    cd ~/ign_transport_tutorial
    mkdir src

Creating the responser
======================

Create the ``src/responser.cc`` file within the ``ign_transport_tutorial``
directory and paste the following code inside it:

.. code-block:: cpp

    #include <cstdio>
    #include <string>
    #include <ignition/transport.hh>
    #include "msgs/stringmsg.pb.h"

    //////////////////////////////////////////////////
    /// \brief Provide an "echo" service.
    void srvEcho(const example::msgs::StringMsg &_req,
      example::msgs::StringMsg &_rep, bool &_result)
    {
      // Set the response's content.
      _rep.set_data(_req.data());

      // The response succeed.
      _result = true;
    }

    //////////////////////////////////////////////////
    int main(int argc, char **argv)
    {
      // Create a transport node.
      ignition::transport::Node node;
      std::string service = "/echo";

      // Advertise a service call.
      if (!node.Advertise(service, srvEcho))
      {
        std::cerr << "Error advertising service [" << service << "]" << std::endl;
        return -1;
      }

      // Wait for requests.
      getchar();
    }

Walkthrough
===========

.. code-block:: cpp

    #include <ignition/transport.hh>
    #include "msgs/stringmsg.pb.h"

The line ``#include <ignition/transport.hh>`` contains the Ignition Transport
header for using the transport library.

The next line includes the generated Protobuf code that we are going to use
for our messages. We are going to use ``StringMsg`` type Protobuf messages
defined in ``stringmsg.pb.h`` for our services.


.. code-block:: cpp

    //////////////////////////////////////////////////
    /// \brief Provide an "echo" service.
    void srvEcho(const example::msgs::StringMsg &_req,
      example::msgs::StringMsg &_rep, bool &_result)
    {
      // Set the response's content.
      _rep.set_data(_req.data());

      // The response succeed.
      _result = true;
    }

As a service provider, our node needs to register a function callback that will
execute every time a new service request is received. The signature of the
callback is always similar to the one shown in this example with the exception
of the Protobuf messages types for the ``_req`` (request) and ``_rep``
(response). The request parameter contains the input parameters of the request.
The response message contains any resulting data from the service call. The ``_result`` parameter denotes if the overall service call was considered
successful or not. In our example, as a simple *echo* service, we just fill the
response with the same data contained in the request.

.. code-block:: cpp

    // Create a transport node.
    ignition::transport::Node node;
    std::string service = "/echo";

    // Advertise a service call.
    if (!node.Advertise(service, srvEcho))
    {
      std::cerr << "Error advertising service [" << service << "]" << std::endl;
      return -1;
    }

We declare a *Node* that will offer all the transport functionality. In our
case, we are interested in offering a service, so the first step is to announce
our service name. Once a service name is advertised, we can accept service
requests.


Creating a synchronous requester
================================

Create the ``src/requester.cc`` file within the ``ign_transport_tutorial``
directory and paste the following code inside it:

.. code-block:: cpp

    #include <iostream>
    #include <ignition/transport.hh>
    #include "msgs/stringmsg.pb.h"

    //////////////////////////////////////////////////
    int main(int argc, char **argv)
    {
      // Create a transport node.
      ignition::transport::Node node;

      // Prepare the input parameters.
      example::msgs::StringMsg req;
      req.set_data("HELLO");

      example::msgs::StringMsg rep;
      bool result;
      unsigned int timeout = 5000;

      // Request the "/echo" service.
      bool executed = node.Request("/echo", req, timeout, rep, result);

      if (executed)
      {
        if (result)
          std::cout << "Response: [" << rep.data() << "]" << std::endl;
        else
          std::cout << "Service call failed" << std::endl;
      }
      else
        std::cerr << "Service call timed out" << std::endl;
    }


Walkthrough
===========

.. code-block:: cpp

    // Create a transport node.
    ignition::transport::Node node;

    // Prepare the input parameters.
    example::msgs::StringMsg req;
    req.set_data("HELLO");

    example::msgs::StringMsg rep;
    bool result;
    unsigned int timeout = 5000;

We declare the *Node* that allows us to request a service. Next, we declare and
fill the message used as an input parameter for our *echo* request. Then, we
declare the Protobuf message that will contain the response and the variable
that will tell us if the service request succeed or failed. In this example, we
will use a synchronous request, meaning that our code will block until the
response is received or a timeout expires. The value of the timeout is expressed
in milliseconds.

.. code-block:: cpp

    // Request the "/echo" service.
    bool executed = node.Request("/echo", req, timeout, rep, result);

    if (executed)
    {
      if (result)
        std::cout << "Response: [" << rep.data() << "]" << std::endl;
      else
        std::cout << "Service call failed" << std::endl;
    }
    else
      std::cerr << "Service call timed out" << std::endl;


In this section of the code we use the method ``Request()`` for forwarding the
service call to any service provider of the service ``/echo``.
Ignition Transport will find a node, communicate the input data, capture the
response and pass it to your output parameter. The return value will tell you
if the request expired or the response was received. The ``result`` value will
tell you if the service provider considered the operation valid.

Imagine for example that we are using a division service, where our input
message contains the numerator and denominator. If there are no nodes offering
this service, our request will timeout (return value ``false``). On the other
hand, if there's at least one node providing the service, the request will
return ``true`` signaling that the request was received. However, if we set our
denominator to ``0`` in the input message, ``result`` will be ``false``
reporting that something went wrong in the request. If the input parameters are
valid, we'll receive a result value of ``true`` and we can use our response
message.


Creating an asynchronous requester
================================

Create the ``src/requester_async.cc`` file within the ``ign_transport_tutorial``
directory and paste the following code inside it:

.. code-block:: cpp

    #include <iostream>
    #include <string>
    #include <ignition/transport.hh>
    #include "msgs/stringmsg.pb.h"

    //////////////////////////////////////////////////
    /// \brief Service response callback.
    void responseCb(const example::msgs::StringMsg &_rep, const bool _result)
    {
      if (_result)
        std::cout << "Response: [" << _rep.data() << "]" << std::endl;
      else
        std::cerr << "Service call failed" << std::endl;
    }

    //////////////////////////////////////////////////
    int main(int argc, char **argv)
    {
      // Create a transport node.
      ignition::transport::Node node;

      // Prepare the input parameters.
      example::msgs::StringMsg req;
      req.set_data("HELLO");

      // Request the "/echo" service.
      node.Request("/echo", req, responseCb);

      // Wait for the response.
      std::cout << "Press <ENTER> to exit" << std::endl;
      getchar();
    }


Walkthrough
===========

.. code-block:: cpp

    //////////////////////////////////////////////////
    /// \brief Service response callback.
    void responseCb(const example::msgs::StringMsg &_rep, const bool _result)
    {
      if (_result)
        std::cout << "Response: [" << _rep.data() << "]" << std::endl;
      else
        std::cerr << "Service call failed" << std::endl;
    }

We need to register a function callback that will execute when we receive our
service response. The signature of the callback is always similar to the one
shown in this example with the only exception of the Protobuf message type used
in the response. You should create a function callback with the appropriate
Protobuf type depending on the response type of the service requested. In our
case, we know that the service ``/echo`` will answer with a Protobuf
`StringMsg`` type.

.. code-block:: cpp

    // Create a transport node.
    ignition::transport::Node node;

    // Prepare the input parameters.
    example::msgs::StringMsg req;
    req.set_data("HELLO");

    // Request the "/echo" service.
    node.Request("/echo", req, responseCb);


In this section of the code we declare a node and a Protobuf message that is
filled with the input parameters for our request. Next, we just use the asynchronous variant of the ``Request()`` method that forwards a service call to
any service provider of the service ``/echo``.
Ignition Transport will find a node, communicate the data, capture the response
and pass it to your callback, in addition of the service call result. Note that
this variant of ``Request()`` is asynchronous, so your code will not block while
your service request is handled.


Building the code
=================

Copy this ``CMakeLists.txt`` file within the ``ign_transport_tutorial``
directory. This is the top level cmake file that will check for dependencies.

Copy this ``hello.proto`` file within the ``ign_transport_tutorial/src``
directory. This is the Protobuf message definition that we use in this example.

Copy this ``CMakeLists.txt`` file within the ``ign_transport_tutorial/src``
directory. This is the cmake file that will generate the C++ code from the
Protobuf file and will create the ``responser`` and ``requester`` executables.

Once you have all your files, go ahead and create a ``build/`` folder within
the ``ign_transport_tutorial`` directory.

.. code-block:: bash

    mkdir build
    cd build

Run ``cmake`` and build the code.

.. code-block:: bash

    cmake ..
    make


Running the examples
====================

Open two new terminals and from your ``build/`` directory run the executables.

From terminal 1:

.. code-block:: bash

    ./responser

From terminal 2:

.. code-block:: bash

    ./requester


In your requester terminal, you should expect an output similar to this one,
showing that your requester has received the data:

.. code-block:: bash

    caguero@turtlebot:~/ign_transport_tutorial/build$ ./requester
    Response: [Hello World!]
