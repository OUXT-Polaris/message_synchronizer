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

class NodeWithAdapter : public rclcpp::Node
{
public:
  explicit NodeWithAdapter(const rclcpp::NodeOptions & option)
  : Node("example", option),
    sync_(
      this,
      {"/perception/front_lidar/points_transform_node/output",
       "/perception/rear_lidar/points_transform_node/output"},
      std::chrono::milliseconds{100}, std::chrono::milliseconds{30}),
    sub_(
      "/perception/front_lidar/points_transform_node/output", this, std::chrono::milliseconds{100},
      std::chrono::milliseconds{30},
      rclcpp::SubscriptionOptionsWithAllocator<std::allocator<void>>(), 10,
      [](const PointCloudType data) {
        // return rclcpp::Time();
        return pcl_conversions::fromPCL(data->header).stamp;
      })
  {
    // const auto func =
    //   std::bind(&Example::callback, this, std::placeholders::_1, std::placeholders::_2);
    // sync_.registerCallback(func);
  }

private:
  message_synchronizer::MessageSynchronizer2<
    sensor_msgs::msg::PointCloud2, sensor_msgs::msg::PointCloud2>
    sync_;
  message_synchronizer::StampedMessageSubscriber<AdaptedType> sub_;
  //   void callback(
  //     const std::optional<sensor_msgs::msg::PointCloud2> & msg0,
  //     const std::optional<sensor_msgs::msg::PointCloud2> & msg1)
  //   {
  //     if (msg0) {
  //       std::cout << __FILE__ << "," << __LINE__ << std::endl;
  //     }
  //     if (msg1) {
  //       std::cout << __FILE__ << "," << __LINE__ << std::endl;
  //     }
  //     std::cout << __FILE__ << "," << __LINE__ << std::endl;
  //   }
};

int main(int argc, char ** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
