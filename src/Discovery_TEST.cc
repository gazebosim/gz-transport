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

#include <uuid/uuid.h>
#include <string>
#include "gtest/gtest.h"
#include "ignition/transport/Discovery.hh"

using namespace ignition;

void OnDiscoveryResponse(const std::string &_topic, const std::string &_addr,
  const std::string &_ctrl, const std::string &_procUuid,
  const std::string &_nodeUuid)
{
  std::cout << "New discovery event received" << std::endl;
  std::cout << "\t Topic: " << _topic << std::endl;
  std::cout << "\t Address: " << _addr << std::endl;
  std::cout << "\t Control address: " << _ctrl << std::endl;
  std::cout << "\t Proc UUID: " << _procUuid << std::endl;
  std::cout << "\t Node UUID: " << _nodeUuid << std::endl;
}

class MyClass
{
  public: MyClass(const std::string &_addr, const std::string &_ctrl,
                  const uuid_t &_uuid)
  {
    this->addr = _addr;
    this->ctrl = _ctrl;
    uuid_copy(this->uuid, _uuid);

    this->discovery = new transport::Discovery(this->addr, this->ctrl,
      this->uuid, this->uuid);

    this->discovery->RegisterDiscoverResp(&MyClass::OnDiscResponse, this);
  }

  public: virtual ~MyClass()
  {
    delete discovery;
  }

  public: void OnDiscResponse(const std::string &_topic,
    const std::string &_addr, const std::string &_ctrl,
    const std::string &_procUuid, const std::string &_nodeUuid)
  {
    std::cout << "New discovery event received (member function)" << std::endl;
    std::cout << "\t Topic: " << _topic << std::endl;
    std::cout << "\t Address: " << _addr << std::endl;
    std::cout << "\t Control address: " << _ctrl << std::endl;
    std::cout << "\t Proc UUID: " << _procUuid << std::endl;
    std::cout << "\t Node UUID: " << _nodeUuid << std::endl;
  }

  private: uuid_t uuid;
  private: std::string addr;
  private: std::string ctrl;
  private: transport::Discovery *discovery;
};

//////////////////////////////////////////////////
TEST(DiscoveryTest, Test1)
{
  uuid_t uuid1;
  uuid_generate(uuid1);
  std::string uuid1Str = transport::GetGuidStr(uuid1);

  uuid_t uuid2;
  uuid_generate(uuid2);
  std::string uuid2Str = transport::GetGuidStr(uuid2);

  std::string localAddr1 = "tcp://127.0.0.1:12345";
  std::string controlAddr1 = "tcp://127.0.0.1:12346";

  std::string localAddr2 = "tcp://127.0.0.1:12347";
  std::string controlAddr2  = "tcp://127.0.0.1:12348";

  transport::Discovery *discovery1 =
    new transport::Discovery(localAddr1, controlAddr1, uuid1, uuid1);

  /*
  transport::Discovery discovery2(localAddr2, controlAddr2, uuid2, uuid2, true);
  discovery2.RegisterDiscoverResp(OnDiscoveryResponse);
  */

  discovery1->Advertise("topic1");

  /*
  discovery2.Discover("topic2");
  getchar();
  */

  discovery1->Unadvertise("topic1");
  // getchar();

  delete discovery1;

  MyClass obj(localAddr1, controlAddr1, uuid1);
  // getchar();
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
