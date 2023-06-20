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
