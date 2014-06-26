/*
 * Copyright (C) 2014 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

#include <czmq.h>
#include <iostream>
#include <string>
#include "gtest/gtest.h"

//////////////////////////////////////////////////
/// \brief Set broadcast/listen discovery beacon.
TEST(beaconTest, pubRecv)
{
  zctx_t *ctx = zctx_new();
  zbeacon_t *beacon = zbeacon_new(ctx, 11312);
  zbeacon_subscribe(beacon, NULL, 0);

  zbeacon_publish(beacon, reinterpret_cast<byte*>(strdup("A message")), 9);

  char *srcAddr = zstr_recv(zbeacon_socket(beacon));
  zframe_t *frame = zframe_recv(zbeacon_socket(beacon));
  byte *data = zframe_data(frame);
  char *msg = reinterpret_cast<char*>(&data[0]);
  std::string recvMsg = std::string(msg);
  EXPECT_EQ(recvMsg, "A message");

  std::cout << "Received beacon from " << srcAddr << std::endl;

  zbeacon_destroy(&beacon);
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
