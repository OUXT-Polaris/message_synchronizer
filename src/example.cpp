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

#include <chrono>
#include <memory>
#include <message_synchronizer/message_synchronizer.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <string>

class Example : public rclcpp::Node
{
public:
  explicit Example(const rclcpp::NodeOptions & option)
  : Node("example", option),
    sync_(
      this,
      {"/perception/front_lidar/points_transform_node/output",
       "/perception/rear_lidar/points_transform_node/output"},
      std::chrono::milliseconds{100}, std::chrono::milliseconds{30})
  {
    const auto func =
      std::bind(&Example::callback, this, std::placeholders::_1, std::placeholders::_2);
    sync_.registerCallback(func);
  }

private:
  message_synchronizer::MessageSynchronizer2<
    sensor_msgs::msg::PointCloud2, sensor_msgs::msg::PointCloud2>
    sync_;
  void callback(
    const boost::optional<const std::shared_ptr<sensor_msgs::msg::PointCloud2>> & msg0,
    const boost::optional<const std::shared_ptr<sensor_msgs::msg::PointCloud2>> & msg1)
  {
    if (msg0) {
      std::cout << __FILE__ << "," << __LINE__ << std::endl;
    }
    if (msg1) {
      std::cout << __FILE__ << "," << __LINE__ << std::endl;
    }
    std::cout << __FILE__ << "," << __LINE__ << std::endl;
  }
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
