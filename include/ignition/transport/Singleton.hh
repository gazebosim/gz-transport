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

#ifndef __IGN_TRANSPORT_SINGLETON_HH_INCLUDED__
#define __IGN_TRANSPORT_SINGLETON_HH_INCLUDED__

template< class C >
class Singleton {
  public:
    static C* getInstance(bool _verbose)
    {
      if (Singleton<C>::uniqueInstance == nullptr)
        Singleton<C>::uniqueInstance = new C(_verbose);

      return Singleton<C>::uniqueInstance;
    }

    static void removeInstance()
    {
      if (Singleton<C>::uniqueInstance != nullptr)
      {
        delete Singleton<C>::uniqueInstance;
        Singleton<C>::uniqueInstance = nullptr;
      }
    }

  private:
    static C *uniqueInstance;
};

// Initialize the static member CurrentInstance
template< class C >
C* Singleton<C>::uniqueInstance = nullptr;

#endif
