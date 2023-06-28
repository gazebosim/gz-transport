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

from ._transport import Node as _Node
from ._transport import AdvertiseMessageOptions, SubscribeOptions
from typing import TypeVar, Callable

# The "ProtoMsg" TypeVar represents an actual msg of a protobuf type.
# On the other hand, the "ProtoMsgType" TypeVar represents the protobuf type.
# For example, let's take the following code: 
#   from gz.msgs10.stringmsg_pb2 import StringMsg
#   proto_msg = StringMsg()
# The variable `proto_msg` would be the expected input for a "ProtoMsg"
# type argument and `StringMsg` would be the expected input for a 
# "ProtoMsgType" type argument.
ProtoMsg = TypeVar("ProtoMsg")
ProtoMsgType = TypeVar("ProtoMsgType")


class Publisher(_Node.Publisher):
    def publish(self, proto_msg: ProtoMsg):
        msg_string = proto_msg.SerializeToString()
        msg_type = proto_msg.DESCRIPTOR.full_name
        return self.publish_raw(msg_string, msg_type)


class Node(_Node):
    def advertise(self, topic: str, msg_type: ProtoMsg,
                  options=AdvertiseMessageOptions()):
        return Publisher(
            _Node.advertise(self, topic, msg_type.DESCRIPTOR.full_name,
                            options))

    def subscribe(self, msg_type: ProtoMsg, topic: str,
                  callback: Callable, options=SubscribeOptions()):
        def cb_deserialize(proto_msg, msg_size, msg_info):
            deserialized_msg = msg_type()
            deserialized_msg.ParseFromString(proto_msg)
            callback(deserialized_msg)

        return self.subscribe_raw(
            topic, cb_deserialize, msg_type.DESCRIPTOR.full_name, options
        )

    def request(self, service: str, request: ProtoMsg,
                request_type: ProtoMsgType,
                response_type: ProtoMsgType, timeout: int):
        result, serialized_response = self.request_raw(
            service,
            request.SerializeToString(),
            request_type.DESCRIPTOR.full_name,
            response_type.DESCRIPTOR.full_name,
            timeout,
        )
        deserialized_response = response_type()
        deserialized_response.ParseFromString(serialized_response)
        return result, deserialized_response
