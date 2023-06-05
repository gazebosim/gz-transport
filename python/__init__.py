from ._transport import Node as _Node
from ._transport import AdvertiseMessageOptions, SubscribeOptions
from importlib import import_module

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
    
    def subscribe(self, topic, callback, msgType, options = SubscribeOptions()):
        def cb_deserialize(proto_msg, msg_size, msg_info):
            module, msg_class = msgType.rsplit('.', 1)
            if "gz.msgs" in msgType:
                module = "gz.msgs10." + msg_class.lower() + "_pb2"
            msg_class = getattr(import_module(module), msg_class)
            deserialized_msg = msg_class()
            deserialized_msg.ParseFromString(proto_msg)
            callback(deserialized_msg)
        return self.subscribe_raw(topic, cb_deserialize, msgType, options)
