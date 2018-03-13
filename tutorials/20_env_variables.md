# Environment variables

In a similar way you can programatically customize the behavior of your
nodes or specify some options when you advertise a topic, it is possible to
use an environment variable to tweak the behavior of Ignition Transport.
Next you can see a description of the available environment variables:

* **IGN_PARITION**
    * *Value allowed*: Any partition value
    * *Description*: Specifies a partition name for all the nodes declared
    inside this process. Note that an alternative partition name  declared
    programatically and  passed to the constructor of a Node class will take
    priorityover *IGN_PARTITION*.
* **IGN_IP**
    * *Value allowed*: Any local IP address
    * *Description*: This setting is needed in situations where you have
    multiple IP addresses for a computer and need to force Ignition
    Transport to use a particular one. This setting is only required if you
    advertise a topic or a service. If you are only subscribed to topics or
    requesting services you don't need to use this option because the
    discovery service will try all the available network interfaces during
    the search of the topic/service.
* **IGN_VERBOSE**
    * *Value allowed*: 1/0
    * *Description*: Show debug information.
* **IGN_TRANSPORT_USERNAME**
    * *Value allowed*: Any string value
    * *Description*: A username, used in combination with
    *IGN_TRANSPORT_PASSWORD*, for basic authentication. Authentication is
    enabled when both *IGN_TRANSPORT_USERNAME* and *IGN_TRANSPORT_PASSWORD*
    are specified.
* **IGN_TRANSPORT_PASSWORD**
    * *Value allowed*: Any string value
    * *Description*: A password, used in combination with
    *IGN_TRANSPORT_USERNAME*, for basic authentication. Authentication is
    enabled when both *IGN_TRANSPORT_USERNAME* and *IGN_TRANSPORT_PASSWORD*
    are specified.
