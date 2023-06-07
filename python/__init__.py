from ._transport import Node as _Node
from ._transport import AdvertiseMessageOptions, SubscribeOptions
from importlib import import_module


class Publisher(_Node.Publisher):

    def publish(self, proto_msg):
        msg_string = proto_msg.SerializeToString()
        msg_type = proto_msg.DESCRIPTOR.full_name
        return self.publish_raw(msg_string, msg_type)


class Node(_Node):

    def advertise(self, topic, msg_type, options=AdvertiseMessageOptions()):
        return Publisher(
            _Node.advertise(self, topic, msg_type.DESCRIPTOR.full_name,
                            options))

    def subscribe(self, msg_type, topic, callback, options=SubscribeOptions()):

        def cb_deserialize(proto_msg, msg_size, msg_info):
            deserialized_msg = msg_type()
            deserialized_msg.ParseFromString(proto_msg)
            callback(deserialized_msg)

        return self.subscribe_raw(topic, cb_deserialize,
                                  msg_type.DESCRIPTOR.full_name, options)
