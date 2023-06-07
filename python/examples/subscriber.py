from gz.msgs10.stringmsg_pb2 import StringMsg
from gz.msgs10.vector3d_pb2 import Vector3d
from gz.transport13 import SubscribeOptions
from gz.transport13 import Node

import time

def stringmsg_cb(msg: StringMsg):
    print("Received StringMsg: [{}]".format(msg.data))

def vector3_cb(msg: Vector3d):
    print("Received Vector3: [x: {}, y: {}, z: {}]".format(msg.x, msg.y, msg.z))

def main():
    # create a transport node
    node = Node()
    topic_stringmsg = "/example_stringmsg_topic"
    topic_vector3d = "/example_vector3d_topic"

    # subscribe to a topic by registering a callback
    if node.subscribe(StringMsg, topic_stringmsg, stringmsg_cb):
        print("Subscribing to type {} on topic [{}]".format(
            StringMsg, topic_stringmsg))
    else:
        print("Error subscribing to topic [{}]".format(topic_stringmsg))
        return

    # subscribe to a topic by registering a callback
    if node.subscribe(Vector3d, topic_vector3d, vector3_cb):
        print("Subscribing to type {} on topic [{}]".format(
            Vector3d, topic_vector3d))
    else:
        print("Error subscribing to topic [{}]".format(topic_vector3d))
        return

    # wait for shutdown
    try:
      while True:
        time.sleep(0.001)
    except KeyboardInterrupt:
      pass
    # TODO(azeey) The subscriber if we quit without unsubscribing. This may not
    # be needed after https://github.com/gazebosim/gz-transport/pull/381.
    node.unsubscribe(topic_stringmsg)
    node.unsubscribe(topic_vector3d)
    print("Done")

if __name__ == "__main__":
    main()
