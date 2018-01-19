/*
 * Copyright (C) 2017 Open Source Robotics Foundation
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

/// \todo: Use ignition::common::WorkPool when smaller libraries from
/// ign-common are available.
#ifndef IGNITION_TRANSPORT_WORKER_POOL_HH_
#define IGNITION_TRANSPORT_WORKER_POOL_HH_

#include <functional>
#include <memory>

namespace ignition
{
  namespace transport
  {
    /// \brief forward declaration
    class WorkerPoolPrivate;

    /// \brief A pool of worker threads that do stuff in parallel
    class WorkerPool
    {
      /// \brief creates worker threads
      public: WorkerPool();

      /// \brief closes worker threads
      public: ~WorkerPool();

      /// \brief Adds work to the worker pool with optional callback
      /// \param[in] _work function to do one piece of work
      /// \param[in] _cb optional callback when the work is done
      /// \remark Typical work is a function bound with arguments. It must
      //               return within a finite amount of time.
      public: void AddWork(std::function<void()> _work,
                  std::function<void()> _cb = std::function<void()>());

      /// \brief private implementation pointer
      private: std::unique_ptr<WorkerPoolPrivate> dataPtr;
    };
  }
}

#endif
