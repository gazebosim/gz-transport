/*
 * Copyright (C) 2026 Open Source Robotics Foundation
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

#include <algorithm>
#include <cctype>
#include <string>

#include <gz/utils/Subprocess.hh>

#include "gtest/gtest.h"

#include "test_config.hh"

//////////////////////////////////////////////////
/// \brief Case-insensitive substring search for "panic". Rust's
/// panic infrastructure is committed to the word "panic" in every
/// variant it emits (panicked at, fatal runtime ... panic,
/// panic-handler), so this catches the libzenohc Tokio TLS
/// shutdown panic regardless of the exact phrasing.
bool ContainsPanic(const std::string &_text)
{
  std::string lower = _text;
  std::transform(lower.begin(), lower.end(), lower.begin(),
    [](unsigned char c) { return std::tolower(c); });
  return lower.find("panic") != std::string::npos;
}

//////////////////////////////////////////////////
/// \brief Run zenohShutdown_aux N times and fail if any run exits
/// non-zero or prints a Rust panic on stdout/stderr.
TEST(ZenohShutdownStress, NoTokioPanicAcrossManyExits)
{
  constexpr int kIterations = 10;

  for (int i = 0; i < kIterations; ++i)
  {
    gz::utils::Subprocess proc({test_executables::kZenohShutdownAux});

    const int rc = proc.Join();
    const std::string out = proc.Stdout();
    const std::string err = proc.Stderr();

    EXPECT_EQ(0, rc) << "iteration " << i << " exited rc=" << rc
                     << "\nstdout:\n" << out
                     << "\nstderr:\n" << err;
    EXPECT_FALSE(ContainsPanic(out))
        << "iteration " << i << " panic on stdout:\n" << out;
    EXPECT_FALSE(ContainsPanic(err))
        << "iteration " << i << " panic on stderr:\n" << err;
  }
}
