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
    // using namespace std::chrono_literals;
    // timer_ = this->create_wall_timer(20ms, std::bind(&PubNode::publish, this));
  }
  void publish(const PointCloudType & point_cloud)
  {
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

class Sub2Node : public rclcpp::Node
{
public:
  explicit Sub2Node(
    const rclcpp::NodeOptions & option, const std::function<void()> & cancel_callback)
  : Node("sub", option),
    cancel_callback_(cancel_callback),
    sync2_(
      this, {"point0", "point1"}, std::chrono::milliseconds{100}, std::chrono::milliseconds{30},
      rclcpp::SubscriptionOptionsWithAllocator<std::allocator<void>>(),
      std::bind(&Sub2Node::getTime, this, std::placeholders::_1),
      std::bind(&Sub2Node::getTime, this, std::placeholders::_1))
  {
    sync2_.registerCallback(
      std::bind(&Sub2Node::callback2, this, std::placeholders::_1, std::placeholders::_2));
  }
  bool is_synchronized = false;

private:
  std::function<void()> cancel_callback_;
  rclcpp::Time getTime(const PointCloudType data)
  {
    return pcl_conversions::fromPCL(data->header).stamp;
  }

  message_synchronizer::MessageSynchronizer2<AdaptedType, AdaptedType> sync2_;
  void callback2(
    const std::optional<PointCloudType> & msg0, const std::optional<PointCloudType> & msg1)
  {
    is_synchronized = true;
    EXPECT_TRUE(msg0);
    EXPECT_STREQ(std::string(msg0.value()->header.frame_id).c_str(), "base_link");
    EXPECT_TRUE(msg1);
    EXPECT_STREQ(std::string(msg1.value()->header.frame_id).c_str(), "base_link");
    cancel_callback_();
  }
};

class Sub3Node : public rclcpp::Node
{
public:
  explicit Sub3Node(
    const rclcpp::NodeOptions & option, const std::function<void()> & cancel_callback)
  : Node("sub", option),
    cancel_callback_(cancel_callback),
    sync3_(
      this, {"point0", "point1", "point2"}, std::chrono::milliseconds{100},
      std::chrono::milliseconds{30},
      rclcpp::SubscriptionOptionsWithAllocator<std::allocator<void>>(),
      std::bind(&Sub3Node::getTime, this, std::placeholders::_1),
      std::bind(&Sub3Node::getTime, this, std::placeholders::_1),
      std::bind(&Sub3Node::getTime, this, std::placeholders::_1))
  {
    sync3_.registerCallback(std::bind(
      &Sub3Node::callback3, this, std::placeholders::_1, std::placeholders::_2,
      std::placeholders::_3));
  }
  bool is_synchronized = false;

private:
  std::function<void()> cancel_callback_;
  rclcpp::Time getTime(const PointCloudType data)
  {
    return pcl_conversions::fromPCL(data->header).stamp;
  }

  message_synchronizer::MessageSynchronizer3<AdaptedType, AdaptedType, AdaptedType> sync3_;
  void callback3(
    const std::optional<PointCloudType> & msg0, const std::optional<PointCloudType> & msg1,
    const std::optional<PointCloudType> & msg2)
  {
    is_synchronized = true;
    EXPECT_TRUE(msg0);
    EXPECT_STREQ(std::string(msg0.value()->header.frame_id).c_str(), "base_link");
    EXPECT_TRUE(msg1);
    EXPECT_STREQ(std::string(msg1.value()->header.frame_id).c_str(), "base_link");
    EXPECT_TRUE(msg2);
    EXPECT_STREQ(std::string(msg2.value()->header.frame_id).c_str(), "base_link");
    cancel_callback_();
  }
};

class Sub4Node : public rclcpp::Node
{
public:
  explicit Sub4Node(
    const rclcpp::NodeOptions & option, const std::function<void()> & cancel_callback)
  : Node("sub", option),
    cancel_callback_(cancel_callback),
    sync4_(
      this, {"point0", "point1", "point2", "point3"}, std::chrono::milliseconds{100},
      std::chrono::milliseconds{30},
      rclcpp::SubscriptionOptionsWithAllocator<std::allocator<void>>(),
      std::bind(&Sub4Node::getTime, this, std::placeholders::_1),
      std::bind(&Sub4Node::getTime, this, std::placeholders::_1),
      std::bind(&Sub4Node::getTime, this, std::placeholders::_1),
      std::bind(&Sub4Node::getTime, this, std::placeholders::_1))
  {
    sync4_.registerCallback(std::bind(
      &Sub4Node::callback4, this, std::placeholders::_1, std::placeholders::_2,
      std::placeholders::_3, std::placeholders::_4));
  }
  bool is_synchronized = false;

private:
  std::function<void()> cancel_callback_;
  rclcpp::Time getTime(const PointCloudType data)
  {
    return pcl_conversions::fromPCL(data->header).stamp;
  }

  message_synchronizer::MessageSynchronizer4<AdaptedType, AdaptedType, AdaptedType, AdaptedType>
    sync4_;
  void callback4(
    const std::optional<PointCloudType> & msg0, const std::optional<PointCloudType> & msg1,
    const std::optional<PointCloudType> & msg2, const std::optional<PointCloudType> & msg3)
  {
    is_synchronized = true;
    EXPECT_TRUE(msg0);
    EXPECT_STREQ(std::string(msg0.value()->header.frame_id).c_str(), "base_link");
    EXPECT_TRUE(msg1);
    EXPECT_STREQ(std::string(msg1.value()->header.frame_id).c_str(), "base_link");
    EXPECT_TRUE(msg2);
    EXPECT_STREQ(std::string(msg2.value()->header.frame_id).c_str(), "base_link");
    EXPECT_TRUE(msg3);
    EXPECT_STREQ(std::string(msg3.value()->header.frame_id).c_str(), "base_link");
    cancel_callback_();
  }
};

TEST(TypeAdaptaer, Sync2Topics)
{
  rclcpp::init(0, nullptr);
  rclcpp::NodeOptions options;
  options.use_intra_process_comms(true);
  rclcpp::executors::SingleThreadedExecutor exec;
  const auto sub_node = std::make_shared<Sub2Node>(options, [&]() { exec.cancel(); });
  const auto pub_node = std::make_shared<PubNode>(options);
  exec.add_node(sub_node);
  exec.add_node(pub_node);

  PointCloudType point_cloud = std::make_shared<pcl::PointCloud<pcl::PointXYZ>>();
  std_msgs::msg::Header header;
  header.frame_id = "base_link";
  header.stamp = pub_node->get_clock()->now();
  point_cloud->header = pcl_conversions::toPCL(header);
  pub_node->publish(point_cloud);

  exec.spin();
  rclcpp::shutdown();
  EXPECT_TRUE(sub_node->is_synchronized);
}

TEST(TypeAdaptaer, Sync3Topics)
{
  rclcpp::init(0, nullptr);
  rclcpp::NodeOptions options;
  options.use_intra_process_comms(true);
  rclcpp::executors::SingleThreadedExecutor exec;
  const auto sub_node = std::make_shared<Sub3Node>(options, [&]() { exec.cancel(); });
  const auto pub_node = std::make_shared<PubNode>(options);
  exec.add_node(sub_node);
  exec.add_node(pub_node);

  PointCloudType point_cloud = std::make_shared<pcl::PointCloud<pcl::PointXYZ>>();
  std_msgs::msg::Header header;
  header.frame_id = "base_link";
  header.stamp = pub_node->get_clock()->now();
  point_cloud->header = pcl_conversions::toPCL(header);
  pub_node->publish(point_cloud);

  exec.spin();
  rclcpp::shutdown();
  EXPECT_TRUE(sub_node->is_synchronized);
}

TEST(TypeAdaptaer, Sync4Topics)
{
  rclcpp::init(0, nullptr);
  rclcpp::NodeOptions options;
  options.use_intra_process_comms(true);
  rclcpp::executors::SingleThreadedExecutor exec;
  const auto sub_node = std::make_shared<Sub4Node>(options, [&]() { exec.cancel(); });
  const auto pub_node = std::make_shared<PubNode>(options);
  exec.add_node(sub_node);
  exec.add_node(pub_node);

  PointCloudType point_cloud = std::make_shared<pcl::PointCloud<pcl::PointXYZ>>();
  std_msgs::msg::Header header;
  header.frame_id = "base_link";
  header.stamp = pub_node->get_clock()->now();
  point_cloud->header = pcl_conversions::toPCL(header);
  pub_node->publish(point_cloud);

  exec.spin();
  rclcpp::shutdown();
  EXPECT_TRUE(sub_node->is_synchronized);
}

int main(int argc, char ** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
