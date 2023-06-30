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

from gz.msgs10.stringmsg_pb2 import StringMsg
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
    
    def stringmsg_cb(self, msg: StringMsg):
        with mutex:
            self.msg_counter += 1
            self.received_msg = msg.data

    def setUp(self):
        # Publisher set up
        self.pub_node = Node()
        self.vector3d_topic = "/test_vector3d"
        self.pub = self.pub_node.advertise(self.vector3d_topic, Vector3d)
        self.assertTrue(self.pub)
        self.assertFalse(self.pub.has_connections())

        self.vector3d_msg = Vector3d()
        self.vector3d_msg.x = 10

    # Check that the publisher publishes a message of the appropiate type
    # but doesn't publish when the message is not the appropiate type.
    def test_publish_msg(self):
        string_msg = StringMsg()
        string_msg.data = "Hello"
        self.assertTrue(self.pub.publish(self.vector3d_msg))
        self.assertFalse(self.pub.publish(string_msg))

    # Check that a message is received if the callback does not use the 
    # advertised types.
    def test_msg_callback(self):
        # Subscriber set up
        sub_node = Node()
        self.assertTrue(
            sub_node.subscribe(Vector3d, self.vector3d_topic, self.vector3_cb)
        )
        self.assertTrue(self.pub.has_connections())

        # Publish and expect callback
        self.received_msg = 0
        self.assertEqual(self.received_msg, 0)
        self.assertTrue(self.pub.publish(self.vector3d_msg))
        time.sleep(0.5)
        with mutex:
            self.assertEqual(self.received_msg, self.vector3d_msg.x)
        self.assertTrue(sub_node.unsubscribe(self.vector3d_topic))
        self.assertFalse(self.pub.has_connections())
    
    # Check that a message is not received if the callback does not use
    # the advertised types.
    def test_wrong_msg_type_callback(self):
        # Subscriber set up
        sub_node = Node()
        self.assertTrue(
            sub_node.subscribe(StringMsg, self.vector3d_topic, self.stringmsg_cb)
        )
        self.received_msg = 0
        self.assertFalse(self.pub.has_connections())
        self.assertEqual(self.received_msg, 0)
        self.assertTrue(self.pub.publish(self.vector3d_msg))
        time.sleep(0.5)
        with mutex:
            self.assertNotEqual(self.received_msg, self.vector3d_msg.x)
        self.assertTrue(sub_node.unsubscribe(self.vector3d_topic))
        self.assertFalse(self.pub.has_connections())

    def test_pub_throttle(self):
        # Throttle Publisher set up
        pub_node = Node()
        throttle_topic = "/test_throttle_topic"
        opts = AdvertiseMessageOptions()
        opts.msgs_per_sec = 1
        pub_throttle = pub_node.advertise(throttle_topic, StringMsg, opts)
        self.assertTrue(pub_throttle)
        self.assertFalse(pub_throttle.has_connections())
        msg = StringMsg()
        msg.data = 'Hello'
        
        # Subscriber set up
        sub_node = Node()
        self.assertTrue(
            sub_node.subscribe(StringMsg, throttle_topic, self.stringmsg_cb)
        )
        self.msg_counter = 0
        self.assertTrue(pub_throttle.has_connections())
        self.assertEqual(self.msg_counter, 0)
        # Publish 25 messages in 2.5s
        for _ in range(25):
            self.assertTrue(pub_throttle.publish(msg))
            time.sleep(0.1)
        with mutex:
            self.assertEqual(self.msg_counter, 3)
        self.assertTrue(sub_node.unsubscribe(throttle_topic))
        self.assertFalse(pub_throttle.has_connections())

    def test_sub_throttle(self):
        # Publisher set up
        pub_node = Node()
        throttle_topic = "/test_throttle_topic"
        pub = pub_node.advertise(throttle_topic, StringMsg)
        self.assertTrue(pub)
        self.assertFalse(pub.has_connections())
        msg = StringMsg()
        msg.data = 'Hello'
        
        # Subscriber set up
        sub_node = Node()
        opts = SubscribeOptions()
        opts.msgs_per_sec = 1
        self.assertTrue(
            sub_node.subscribe(StringMsg, throttle_topic, self.stringmsg_cb, opts)
        )
        self.msg_counter = 0
        self.assertTrue(pub.has_connections())
        self.assertEqual(self.msg_counter, 0)
        # Publish 25 messages in 2.5s
        for _ in range(25):
            self.assertTrue(pub.publish(msg))
            time.sleep(0.1)
        with mutex:
            self.assertEqual(self.msg_counter, 3)
        self.assertTrue(sub_node.unsubscribe(throttle_topic))
        self.assertFalse(pub.has_connections())

    def test_topic_list(self):
        # Second Publisher set up
        pub_node = Node()
        string_msg_topic = "/test_stringmsg_topic"
        pub_2 = pub_node.advertise(string_msg_topic, StringMsg)
        self.assertTrue(pub_2)
        self.assertTrue(pub_2.valid())
        self.assertFalse(pub_2.has_connections())

        # Node set up
        node = Node()
        topics = node.topic_list()
        self.assertTrue(topics)
        self.assertEqual(len(topics), 2)
        # Check alphabetical order of the list of topics
        self.assertEqual(topics[0], string_msg_topic)
