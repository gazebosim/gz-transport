#!/usr/bin/ruby

# Copyright (C) 2018 Open Source Robotics Foundation
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

require 'open3'

testfile = 'ign_log_record_no_overwrite.tlog'
File.write(testfile, 'not empty file')

stdout, stderr, status = Open3.capture3("ign log record --file #{testfile}")

File.delete(testfile)

puts '-----stdout-----'
puts stdout
puts '-----stderr-----'
puts stderr
puts '-----status-----'
puts status

exit 1 if status.exitstatus.zero?
exit 2 unless stderr.include? 'Failed to open log'
exit 0
