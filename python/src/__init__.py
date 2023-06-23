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

class Publisher(_Node.Publisher):

    def publish(self, proto_msg):
        msg_string = proto_msg.SerializeToString()
        msg_type = proto_msg.DESCRIPTOR.full_name
        return self.publish_raw(msg_string, msg_type)

class Node(_Node):

    def advertise(self, topic, msg_type, options=AdvertiseMessageOptions()):
        return Publisher(
            _Node.advertise(self, topic, msg_type.DESCRIPTOR.full_name,
                            options))

    def subscribe(self, msg_type, topic, callback, options=SubscribeOptions()):

        def cb_deserialize(proto_msg, msg_size, msg_info):
            deserialized_msg = msg_type()
            deserialized_msg.ParseFromString(proto_msg)
            callback(deserialized_msg)

        return self.subscribe_raw(topic, cb_deserialize,
                                  msg_type.DESCRIPTOR.full_name, options)
    
    def request(self, service, request, request_type, response_type, timeout, response):
        result, serialized_response = self.request_raw(service, request.SerializeToString(), request_type.DESCRIPTOR.full_name,
                                response_type.DESCRIPTOR.full_name, timeout, response.SerializeToString())
        deserialized_response = response_type()
        deserialized_response.ParseFromString(serialized_response)
        return result, deserialized_response
