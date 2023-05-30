from transport13 import *

class PyGzTransport():
    def publish(self, proto_msg):
        msg_string = proto_msg.SerializeToString()
        msg_type = proto_msg.DESCRIPTOR.name
        Publisher().publish_raw(msg_string, msg_type) 

if __name__ == "__main__":
    test_pub = PyGzTransport()
