from ._transport import Node

class Publisher(Node._Publisher):
    def publish(self, proto_msg):
        msg_string = proto_msg.SerializeToString()
        msg_type = proto_msg.DESCRIPTOR.full_name
        self.publish_raw(msg_string, msg_type)