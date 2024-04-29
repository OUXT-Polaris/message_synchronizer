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

#include <gtest/gtest.h>

#include <message_synchronizer/message_synchronizer.hpp>
#include <pcl_type_adapter/pcl_type_adapter.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <string>

using PointCloudType = std::shared_ptr<pcl::PointCloud<pcl::PointXYZ>>;
using AdaptedType = rclcpp::TypeAdapter<PointCloudType, sensor_msgs::msg::PointCloud2>;

class SubNode : public rclcpp::Node
{
public:
  explicit SubNode(const rclcpp::NodeOptions & option)
  : Node("example", option),
    sync_(
      this,
      {"/perception/front_lidar/points_transform_node/output",
       "/perception/rear_lidar/points_transform_node/output"},
      std::chrono::milliseconds{100}, std::chrono::milliseconds{30},
      rclcpp::SubscriptionOptionsWithAllocator<std::allocator<void>>(),
      [](const PointCloudType data) { return pcl_conversions::fromPCL(data->header).stamp; },
      [](const PointCloudType data) { return pcl_conversions::fromPCL(data->header).stamp; })
  {
    sync_.registerCallback(
      std::bind(&SubNode::callback2, this, std::placeholders::_1, std::placeholders::_2));
  }

private:
  message_synchronizer::MessageSynchronizer2<AdaptedType, AdaptedType> sync_;
  void callback2(
    const std::optional<PointCloudType> & msg0, const std::optional<PointCloudType> & msg1)
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

int main(int argc, char ** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
