// Copyright (c) 2019 OUXT Polaris
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <rclcpp/rclcpp.hpp>

#include <message_synchronizer/message_synchronizer.hpp>

#include <sensor_msgs/msg/point_cloud2.hpp>

#include <memory>
#include <string>

class Example : public rclcpp::Node
{
public:
  explicit Example(const rclcpp::NodeOptions & option)
  : Node("example", option), sync_(this, {"/input0", "/input1"})
  {
  }

private:
  message_synchronizer::MessageSynchronizer<
    sensor_msgs::msg::PointCloud2,
    sensor_msgs::msg::PointCloud2> sync_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::NodeOptions options;
  auto component = std::make_shared<Example>(options);
  rclcpp::spin(component);
  rclcpp::shutdown();
  return 0;
}
