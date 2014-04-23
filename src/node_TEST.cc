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

#include <limits.h>
#include <uuid/uuid.h>
#include <thread>
#include "ignition/transport/node.hh"
#include "ignition/transport/zhelpers.hpp"
#include "gtest/gtest.h"


using namespace ignition;

bool callbackExecuted;

//////////////////////////////////////////////////
/// \brief Function is called everytime a topic update is received.
void cb(const std::string &_topic, const std::string &_data)
{
  assert(_topic != "");
  EXPECT_EQ(_data, "someData");
  callbackExecuted = true;
}

//////////////////////////////////////////////////
void CreateSubscriber()
{
	std::string master = "";
	std::string topic1 = "foo";
	bool verbose = false;
	transport::Node *node = transport::Node::getInstance(master, verbose);
	EXPECT_EQ(node->Subscribe(topic1, cb), 0);
	while (!callbackExecuted)
		sleep(1);
}

//////////////////////////////////////////////////
TEST(DiscZmqTest, PubWithoutAdvertise)
{
	std::string master = "";
	bool verbose = false;
	std::string topic1 = "foo";
	std::string data = "someData";

	// Subscribe to topic1
	transport::Node *node = transport::Node::getInstance(master, verbose);

	// Publish some data on topic1 without advertising it first
	EXPECT_NE(node->Publish(topic1, data), 0);
}

//////////////////////////////////////////////////
TEST(DiscZmqTest, PubSubSameThread)
{
	callbackExecuted = false;
	std::string master = "";
	bool verbose = false;
	std::string topic1 = "foo";
	std::string data = "someData";

	transport::Node *node = transport::Node::getInstance(master, verbose);

	// Advertise topic1
	EXPECT_EQ(node->Advertise(topic1), 0);

	// Subscribe to topic1
	EXPECT_EQ(node->Subscribe(topic1, cb), 0);
	s_sleep(100);

	// Publish some data on topic1
	EXPECT_EQ(node->Publish(topic1, data), 0);
	s_sleep(100);

	// Check that the data was received
	EXPECT_TRUE(callbackExecuted);
	callbackExecuted = false;

	// Publish a second message on topic1
	EXPECT_EQ(node->Publish(topic1, data), 0);
	s_sleep(100);

	// Check that the data was received
	EXPECT_TRUE(callbackExecuted);
	callbackExecuted = false;

	// Unadvertise topic1 and publish a third message
	node->UnAdvertise(topic1);
	EXPECT_NE(node->Publish(topic1, data), 0);
	s_sleep(100);
	EXPECT_FALSE(callbackExecuted);
}

//////////////////////////////////////////////////
TEST(DiscZmqTest, PubSubSameProcess)
{
	callbackExecuted = false;
	std::string master = "";
	bool verbose = false;
	std::string topic1 = "foo";
	std::string data = "someData";

	// Create the transport node
	transport::Node *node = transport::Node::getInstance(master, verbose);
	EXPECT_EQ(node->Advertise(topic1), 0);
	s_sleep(100);

	// Subscribe to topic1 in a different thread
	std::thread subscribeThread(CreateSubscriber);
	s_sleep(100);

	// Advertise and publish some data on topic1
	EXPECT_EQ(node->Publish(topic1, data), 0);
	s_sleep(100);

	subscribeThread.join();

	// Check that the data was received
	EXPECT_TRUE(callbackExecuted);
	callbackExecuted = false;
}

//////////////////////////////////////////////////
/*TEST(DiscZmqTest, NPubSub)
{
	callbackExecuted = false;
	std::string master = "";
	bool verbose = false;
	std::string topic1 = "foo";
	std::string data = "someData";

	// Subscribe to topic1
	Node nodeSub(master, verbose);
	EXPECT_EQ(nodeSub.Subscribe(topic1, cb), 0);
	nodeSub.SpinOnce();

	// Advertise and publish some data on topic1
	Node *nodePub = new Node(master, verbose);
	EXPECT_EQ(nodePub->Advertise(topic1), 0);
	EXPECT_EQ(nodePub->Publish(topic1, data), 0);
	s_sleep(200);
	nodeSub.SpinOnce();

	// Check that the data was received
	EXPECT_TRUE(callbackExecuted);
	callbackExecuted = false;
	delete nodePub;

	// Publish a second message on topic1 with a new node
	nodePub = new Node(master, verbose);
	EXPECT_EQ(nodePub->Advertise(topic1), 0);
	EXPECT_EQ(nodePub->Publish(topic1, data), 0);
	s_sleep(100);
	nodeSub.SpinOnce();

	// Check that the data was received
	EXPECT_TRUE(callbackExecuted);
}*/

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
