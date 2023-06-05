from gz.msgs10.stringmsg_pb2 import StringMsg
from gz.msgs10.vector3d_pb2 import Vector3d
from gz.transport13 import AdvertiseMessageOptions
from gz.transport13 import Node

import time

def main():
    node = Node()
    stringmsg_topic = "/example_stringmsg_topic"
    vector3d_topic = "/example_vector3d_topic"
    stringmsg_type = StringMsg.DESCRIPTOR.full_name
    vector3d_type = Vector3d.DESCRIPTOR.full_name
    pub_options = AdvertiseMessageOptions()
    pub_stringmsg = node.advertise(stringmsg_topic, stringmsg_type, pub_options)
    pub_vector3d = node.advertise(vector3d_topic, vector3d_type, pub_options)

    vector3d_msg = Vector3d()
    vector3d_msg.x = 10
    vector3d_msg.y = 15
    vector3d_msg.z = 20

    stringmsg_msg = StringMsg()
    stringmsg_msg.data = "Hello" 
    try:
        while True:
          if not pub_stringmsg.publish(stringmsg_msg) or not pub_vector3d.publish(vector3d_msg):
              break

          print("Publishing 'Hello' on topic [{}]".format(stringmsg_topic))
          print("Publishing a Vector3d on topic [{}]".format(vector3d_topic))
          time.sleep(1.0)

    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
