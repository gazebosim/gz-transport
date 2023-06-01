from ._transport import Node as _Node
from ._transport import AdvertiseMessageOptions

class Publisher():
    def __init__(self, publisher):
        self._publisher = publisher

    def publish(self, proto_msg):
        msg_string = proto_msg.SerializeToString()
        msg_type = proto_msg.DESCRIPTOR.full_name
        return self._publisher.publish_raw(msg_string, msg_type)

    def __getattr__(self, name):
        if name != "publish":
            return self._publisher.__getattribute__(name)
        else:
            return self.publish

class Node(_Node):
    def advertise(self, topic, msg_type_name, options):
        return Publisher(super().advertise(topic, msg_type_name, options))
