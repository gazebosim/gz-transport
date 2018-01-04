/*
 * Copyright (C) 2018 Open Source Robotics Foundation
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

#ifndef IGNITION_TRANSPORT_LOG_TEST_INTEGRATION_CHIRPPARAMS_HH_
#define IGNITION_TRANSPORT_LOG_TEST_INTEGRATION_CHIRPPARAMS_HH_

#include <ignition/msgs/int32.pb.h>

namespace ignition
{
  namespace transport
  {
    namespace log
    {
      namespace test
      {
        /// \brief Parameter used to determine how long the topicChirp_aux
        /// program will wait between emitting message chirps from its topic.
        /// Value is in milliseconds.
        const int DelayBetweenChirps_ms = 1;

        /// \brief Parameter used to determine how long the topicChirp_aux
        /// program will wait (after it advertises) before it begins publishing
        /// its message chirps. Value is in milliseconds.
        const int DelayBeforePublishing_ms = 1000;

        /// \brief This is the message type that will be used by the chirping
        /// topics.
        using ChirpMsgType = ignition::msgs::Int32;
      }
    }
  }
}

#endif
