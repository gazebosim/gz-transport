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

#include <queue>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <utility>

#include "WorkerPool.hh"

using namespace ignition::transport;

namespace ignition
{
  namespace transport
  {
    /// \brief info needed to perform work
    struct WorkOrder
    {
      /// \brief method that does the work
      std::function<void()> work = std::function<void()>();

      /// \brief callback to invoke after working
      std::function<void()> callback = std::function<void()>();
    };

    /// \brief Private implementation
    class WorkerPoolPrivate
    {
      /// \brief Does work until signaled to shut down
      public: void Worker();

      /// \brief threads that do work
      public: std::vector<std::thread> workers;

      /// \brief queue of work for workers
      public: std::queue<WorkOrder> workOrders;

      /// \brief used to count how many threads are actively working
      public: int activeOrders = 0;

      /// \brief lock for workOrders access
      public: std::mutex queueMtx;

      /// \brief used to signal when all work is done
      public: std::condition_variable signalWorkDone;

      /// \brief used to signal when new work is available
      public: std::condition_variable signalNewWork;

      /// \brief used to signal when the pool is being shut down
      public: bool done = false;
    };
  }
}

//////////////////////////////////////////////////
void WorkerPoolPrivate::Worker()
{
  WorkOrder order;

  // Run until pool is destructed, waiting for work
  while (true)
  {
    // Scoped to release lock before doing work
    {
      std::unique_lock<std::mutex> queueLock(this->queueMtx);

      // Wait for a work order
      while (!this->done && this->workOrders.empty())
      {
        this->signalNewWork.wait(queueLock);
      }
      // Destructor may have signaled to shutdown workers
      if (this->done)
        break;

      // Take a work order from the queue
      ++(this->activeOrders);
      order = std::move(workOrders.front());
      workOrders.pop();
    }

    // Do the work
    if (order.work)
      order.work();

    if (order.callback)
      order.callback();

    {
      std::unique_lock<std::mutex> queueLock(this->queueMtx);
      --(this->activeOrders);
    }
    this->signalWorkDone.notify_all();
  }
}

//////////////////////////////////////////////////
WorkerPool::WorkerPool() : dataPtr(new WorkerPoolPrivate)
{
  unsigned int numWorkers = std::max(std::thread::hardware_concurrency(), 1u);

  // create worker threads
  for (unsigned int w = 0; w < numWorkers; ++w)
  {
    this->dataPtr->workers.push_back(
        std::thread(&WorkerPoolPrivate::Worker, this->dataPtr.get()));
  }
}

//////////////////////////////////////////////////
WorkerPool::~WorkerPool()
{
  // shutdown worker threads
  {
    std::unique_lock<std::mutex> queueLock(this->dataPtr->queueMtx);
    this->dataPtr->done = true;
  }
  this->dataPtr->signalNewWork.notify_all();

  for (auto &t : this->dataPtr->workers)
  {
    t.join();
  }

  // Signal in case anyone is still waiting for work to finish
  this->dataPtr->signalWorkDone.notify_all();
}

//////////////////////////////////////////////////
void WorkerPool::AddWork(std::function<void()> _work,
    std::function<void()> _cb)
{
  WorkOrder order;
  order.work = _work;
  order.callback = _cb;
  {
    std::unique_lock<std::mutex> queueLock(this->dataPtr->queueMtx);
    this->dataPtr->workOrders.push(std::move(order));
  }
  this->dataPtr->signalNewWork.notify_one();
}
