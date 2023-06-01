from gz.msgs10.stringmsg_pb2 import StringMsg
from gz.transport13 import AdvertiseMessageOptions
from gz.transport13 import Node

import time

def main():
    node = Node()
    topic_name = "/example_topic"
    msg_type = StringMsg.DESCRIPTOR.full_name
    pub_options = AdvertiseMessageOptions()
    pub = node.advertise(topic_name, msg_type, pub_options)

    msg = StringMsg()
    msg.data = "Hello"
    try:
        while True:
          if not pub.publish(msg):
              break

          print("Publishing 'Hello' on topic [{}]".format(topic_name))
          time.sleep(1.0)

    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()