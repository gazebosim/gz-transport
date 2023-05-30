/*
 * Copyright (C) 2015 Open Source Robotics Foundation
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

#include <gz/msgs.hh>
#include <gz/transport.hh>

#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>

#include <functional>
#include <tuple>

namespace py = pybind11;

namespace gz
{
namespace transport
{
namespace python
{

PYBIND11_MODULE(transport13, m) {
    py::class_<gz::transport::Node::Publisher>(m, "Publisher")
      .def(py::init<>())
      .def("publish", &gz::transport::Node::Publisher::Publish,
          py::arg("msg"),
          "Publish a message")
      .def("publish_raw", &gz::transport::Node::Publisher::PublishRaw,
          py::arg("msgData"),
          py::arg("msgType"));
}  // gz-transport13 module

}  // python
}  // transport
}  // gz
