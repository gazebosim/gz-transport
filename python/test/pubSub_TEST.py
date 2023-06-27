# Copyright (C) 2023 Open Source Robotics Foundation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

from gz.msgs10.vector3d_pb2 import Vector3d
from gz.transport13 import Node, AdvertiseMessageOptions, SubscribeOptions

from threading import Lock

import time
import unittest

mutex = Lock()

class PubSubTEST(unittest.TestCase):
    def vector3_cb(self, msg: Vector3d):
        with mutex:
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
        self.assertTrue(
            self.sub_node.subscribe(Vector3d, vector3d_topic, self.vector3_cb)
        )
        self.received_msg = 0
        self.assertTrue(self.pub.has_connections())

    def test_msg_callback(self):
        self.assertEqual(self.received_msg, 0)
        self.assertTrue(self.pub.publish(self.vector3d_msg))
        time.sleep(0.5)
        with mutex:    
            self.assertEqual(self.received_msg, self.vector3d_msg.x)
