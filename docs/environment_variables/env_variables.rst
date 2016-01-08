=======================================
Configuration via environment variables
=======================================

In a similar you can programatically customize the behavior of your nodes or
specify some options when you advertise a topic, it is possible to use an
environment variable to tweak the behavior of Ignition Transport. Next you can
see a description of the available environment variables:

====================  ==============        ===========
Environment variable  Values allowed        Description
====================  ==============        ===========
*IGN_PARTITION*       Any partition value   Specifies a partition name for all
                                            the nodes declared inside this
                                            process. Note that an alternative
                                            partition name declared
                                            programatically and passed to the
                                            constructor of a Node class will
                                            take priority over *IGN_PARTITION*.
*IGN_IP*              Any local IP address  This setting is needed in situations
                                            where you have multiple addresses
                                            for a computer and need to force
                                            Ignition Transport to use a
                                            particular one. This setting is only
                                            required if you are advertising a
                                            topic or a service. If you are only
                                            subscribing to topics or requesting
                                            services you don't need to use this
                                            options because the discovery
                                            service will try all the available
                                            network interfaces during the search
                                            of the topic/service.
*IGN_VERBOSE*         1                     Show debug information.
============           =============        ===========