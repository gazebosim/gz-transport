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
from gz.transport13 import Node

import unittest

class pubSubTEST(unittest.TestCase):

    def setUp(self):
        # TODO: Call the twoProcsSrvCallReplier_aux file
        # Requester Setup
        self.node = Node()
        self.assertTrue(self.node)
        self.service_name = "/echo"
        self.request = self.response = StringMsg()
        self.request.data = "Hello world"
        self.timeout = 5000

    def test_msg_callback(self):
        self.assertNotEqual(response.data, self.request.data)
        result, response = self.node.request(self.service_name, self.request, StringMsg, StringMsg, self.timeout, self.response)
        self.assertTrue(result)
        self.assertEqual(response.data, self.request.data)
