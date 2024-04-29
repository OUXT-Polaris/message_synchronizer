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
#include <std_msgs/msg/header.hpp>
#include <string>

using PointCloudType = std::shared_ptr<pcl::PointCloud<pcl::PointXYZ>>;
using AdaptedType = rclcpp::TypeAdapter<PointCloudType, sensor_msgs::msg::PointCloud2>;

class PubNode : public rclcpp::Node
{
public:
  explicit PubNode(const rclcpp::NodeOptions & option) : Node("pub", option)
  {
    pub0_ = create_publisher<AdaptedType>("point0", 1);
    pub1_ = create_publisher<AdaptedType>("point1", 1);
    pub2_ = create_publisher<AdaptedType>("point2", 1);
    pub3_ = create_publisher<AdaptedType>("point3", 1);
    using namespace std::chrono_literals;
    timer_ = this->create_wall_timer(20ms, std::bind(&PubNode::publish, this));
  }
  void publish()
  {
    PointCloudType point_cloud = std::make_shared<pcl::PointCloud<pcl::PointXYZ>>();
    std_msgs::msg::Header header;
    header.frame_id = "base_link";
    header.stamp = get_clock()->now();
    point_cloud->header = pcl_conversions::toPCL(header);
    pub0_->publish(point_cloud);
    pub1_->publish(point_cloud);
    pub2_->publish(point_cloud);
    pub3_->publish(point_cloud);
  }
  std::shared_ptr<rclcpp::Publisher<AdaptedType>> pub0_;
  std::shared_ptr<rclcpp::Publisher<AdaptedType>> pub1_;
  std::shared_ptr<rclcpp::Publisher<AdaptedType>> pub2_;
  std::shared_ptr<rclcpp::Publisher<AdaptedType>> pub3_;
  rclcpp::TimerBase::SharedPtr timer_;
};

class SubNode : public rclcpp::Node
{
public:
  explicit SubNode(const rclcpp::NodeOptions & option)
  : Node("sub", option),
    sync2_(
      this, {"point0", "point1"}, std::chrono::milliseconds{100}, std::chrono::milliseconds{30},
      rclcpp::SubscriptionOptionsWithAllocator<std::allocator<void>>(),
      std::bind(&SubNode::getTime, this, std::placeholders::_1),
      std::bind(&SubNode::getTime, this, std::placeholders::_1)),
    sync3_(
      this, {"point0", "point1", "point2"}, std::chrono::milliseconds{100},
      std::chrono::milliseconds{30},
      rclcpp::SubscriptionOptionsWithAllocator<std::allocator<void>>(),
      std::bind(&SubNode::getTime, this, std::placeholders::_1),
      std::bind(&SubNode::getTime, this, std::placeholders::_1),
      std::bind(&SubNode::getTime, this, std::placeholders::_1)),
    sync4_(
      this, {"point0", "point1", "point2", "point3"}, std::chrono::milliseconds{100},
      std::chrono::milliseconds{30},
      rclcpp::SubscriptionOptionsWithAllocator<std::allocator<void>>(),
      std::bind(&SubNode::getTime, this, std::placeholders::_1),
      std::bind(&SubNode::getTime, this, std::placeholders::_1),
      std::bind(&SubNode::getTime, this, std::placeholders::_1),
      std::bind(&SubNode::getTime, this, std::placeholders::_1))
  {
    sync2_.registerCallback(
      std::bind(&SubNode::callback2, this, std::placeholders::_1, std::placeholders::_2));
    sync3_.registerCallback(std::bind(
      &SubNode::callback3, this, std::placeholders::_1, std::placeholders::_2,
      std::placeholders::_3));
    sync4_.registerCallback(std::bind(
      &SubNode::callback4, this, std::placeholders::_1, std::placeholders::_2,
      std::placeholders::_3, std::placeholders::_4));
  }

private:
  rclcpp::Time getTime(const PointCloudType data)
  {
    return pcl_conversions::fromPCL(data->header).stamp;
  }

  message_synchronizer::MessageSynchronizer2<AdaptedType, AdaptedType> sync2_;
  void callback2(
    const std::optional<PointCloudType> & msg0, const std::optional<PointCloudType> & msg1)
  {
    if (msg0 && msg1) {
    }
  }

  message_synchronizer::MessageSynchronizer3<AdaptedType, AdaptedType, AdaptedType> sync3_;
  void callback3(
    const std::optional<PointCloudType> & msg0, const std::optional<PointCloudType> & msg1,
    const std::optional<PointCloudType> & msg2)
  {
    if (msg0 && msg1 && msg2) {
    }
  }

  message_synchronizer::MessageSynchronizer4<AdaptedType, AdaptedType, AdaptedType, AdaptedType>
    sync4_;
  void callback4(
    const std::optional<PointCloudType> & msg0, const std::optional<PointCloudType> & msg1,
    const std::optional<PointCloudType> & msg2, const std::optional<PointCloudType> & msg3)
  {
    if (msg0 && msg1 && msg2 && msg3) {
    }
  }
};

TEST(TypeAdaptaer, Sync2Topics)
{
  rclcpp::init(0, nullptr);
  rclcpp::NodeOptions options;
  options.use_intra_process_comms(true);
  const auto sub_node = std::make_shared<SubNode>(options);
}

int main(int argc, char ** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
