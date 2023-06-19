from gz.msgs10.vector3d_pb2 import Vector3d
from gz.transport13 import Node, AdvertiseMessageOptions, SubscribeOptions

import unittest

class pubSubTEST(unittest.TestCase):

    def vector3_cb(self, msg: Vector3d):
        self.received_msg = msg.x

    def setUp(self):
        # Publisher set up
        self.pub_node = Node()
        self.assertTrue(self.pub_node)
        vector3d_topic = "/test_vector3d"
        self.pub = self.pub_node.advertise(vector3d_topic, Vector3d)
        self.assertTrue(self.pub)
        self.assertFalse(self.pub.has_connections())

        self.vector3d_msg = Vector3d()
        self.vector3d_msg.x = 10

        # Subscriber set up
        self.sub_node = Node()
        self.assertTrue(self.sub_node)
        self.assertTrue(self.sub_node.subscribe(Vector3d, vector3d_topic, self.vector3_cb))
        self.received_msg = 0
        self.assertTrue(self.pub.has_connections())
    
    def test_msg_callback(self):
        self.assertEqual(self.received_msg, 0)
        self.assertTrue(self.pub.publish(self.vector3d_msg))
        self.assertEqual(self.received_msg, self.vector3d_msg.x)
