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
    stringmsg_type = StringMsg.DESCRIPTOR.full_name
    vector3d_type = Vector3d.DESCRIPTOR.full_name
    sub_options = SubscribeOptions()

    # subscribe to a topic by registering a callback
    if node.subscribe(topic_stringmsg, stringmsg_cb, stringmsg_type, sub_options):
        print("Subscribing to type {} on topic [{}]".format(
            stringmsg_type, topic_stringmsg))
    else:
        print("Error subscribing to topic [{}]".format(topic_stringmsg))
        return

    # subscribe to a topic by registering a callback
    if node.subscribe(topic_vector3d, vector3_cb, vector3d_type, sub_options):
        print("Subscribing to type {} on topic [{}]".format(
            vector3d_type, topic_vector3d))
    else:
        print("Error subscribing to topic [{}]".format(topic_vector3d))
        return

    # wait for shutdown
    try:
      while True:
        time.sleep(0.001)
    except KeyboardInterrupt:
      pass

if __name__ == "__main__":
    main()
