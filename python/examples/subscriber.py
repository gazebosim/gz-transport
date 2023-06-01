from gz.msgs.stringmsg_pb2 import StringMsg
from gz.transport13 import SubscribeOptions
from gz.transport13 import Node

import time

def cb(msg: StringMsg) -> None:
    print("Received message: [{}]".format(msg.data))

def main():
    # create a transport node
    node = Node()
    topic = "/example_topic"
    msg_type = StringMsg.DESCRIPTOR.full_name
    sub_options = SubscribeOptions()

    # subscribe to a topic by registering a callback
    if node.subscribe(topic, cb, msg_type, sub_options):
        print("Subscribing to type {} on topic [{}]".format(
            msg_type, topic))
    else:
        print("Error subscribing to topic [{}]".format(topic))
        return

    # wait for shutdown
    try:
      while True:
        time.sleep(0.001)
    except KeyboardInterrupt:
      pass

if __name__ == "__main__":
    main()
