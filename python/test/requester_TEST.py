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

from gz.msgs10.int32_pb2 import Int32
from gz.transport13 import Node

import os
import subprocess
import unittest


class RequesterTEST(unittest.TestCase):
    def setUp(self):
        # Environment Setup
        gz_partition = "python_requester_test"
        os.environ["GZ_PARTITION"] = gz_partition

        # Subprocess Setup
        cmd = f"{os.getenv('CMAKE_BINARY_DIR')}/INTEGRATION_twoProcsSrvCallReplier_aux {gz_partition}"
        self.service_process = subprocess.Popen(cmd, shell=True)

        # Requester Setup
        self.node = Node()
        self.service_name = "/foo"
        self.request = Int32()
        self.request.data = 100
        self.timeout = 5000

    def tearDown(self):
        self.service_process.kill()

    def test_msg_callback(self):
        result, response = self.node.request(
            self.service_name, self.request, Int32, Int32, self.timeout
        )
        self.assertTrue(result)
        self.assertEqual(response.data, self.request.data)
