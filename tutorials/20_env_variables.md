\page envvars Environment Variables

Next Tutorial: \ref contribute

## Overview

In a similar way you can programatically customize the behavior of your
nodes or specify some options when you advertise a topic, it is possible to
use an environment variable to tweak the behavior of Ignition Transport.
Next you can see a description of the available environment variables:

* **IGN_PARTITION**
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
* **IGN_RELAY**
    * *Value allowed*: Colon delimited list of IP addresses
    * *Description*: This setting is needed when you want to connect the nodes
    of two or more different networks separated by routers. Routers do not
    forward UDP multicast traffic, so the regular discovery protocol only works
    when nodes are within the same local network. If you feel the need to
    connect these networks, make sure to have one node pointing to the IP
    address of another node from the other network. Note that only one IP_RELAY
    link is needed for bidirectional communication between nodes of two
    different networks.
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
* **IGN_TRANSPORT_LOG_SQL_PATH**
    * *Value allowed*: Any path
    * *Description*: Path to the SQL files used by logging. This does not
    normally need to be set. It is useful to developers who are testing changes
    to the schema, and it is used by unit tests.
