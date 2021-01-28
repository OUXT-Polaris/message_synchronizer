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

#ifndef MESSAGE_SYNCHRONIZER__MESSAGE_SYNCHRONIZER_HPP_
#define MESSAGE_SYNCHRONIZER__MESSAGE_SYNCHRONIZER_HPP_

#include <rclcpp/rclcpp.hpp>

#include <boost/circular_buffer.hpp>
#include <boost/optional.hpp>

#include <unordered_map>
#include <exception>
#include <functional>

namespace message_synchronizer
{
template<typename T>
class StampedMessageSubscriber
{
public:
  template<class NodeT, class AllocatorT = std::allocator<void>>
  StampedMessageSubscriber(
    const std::string & topic_name, NodeT && node,
    const rclcpp::SubscriptionOptionsWithAllocator<AllocatorT> & options =
    rclcpp::SubscriptionOptionsWithAllocator<AllocatorT>())
  {
    auto callback = std::bind(&StampedMessageSubscriber::callback, this, std::placeholders::_1);
    buffer_ = boost::circular_buffer<T>(10);
    sub_ = rclcpp::create_subscription<T>(
      node, topic_name,
      rclcpp::QoS(10),
      std::move(callback),
      options);
  }

private:
  double buffer_duration_;
  boost::circular_buffer<T> buffer_;
  std::shared_ptr<rclcpp::Subscription<T>> sub_;
  void callback(const std::shared_ptr<T> msg)
  {
    buffer_.push_back(*msg);
  }
  boost::optional<const std::shared_ptr<T>> query(rclcpp::Time stamp)
  {
    for (const auto buf :  buffer_) {
      const auto stamp = buf.header.stamp;
    }
    return boost::none;
  }
};

template<typename T0, typename T1>
class MessageSynchronizer
{
public:
  template<class NodeT, class AllocatorT = std::allocator<void>>
  MessageSynchronizer(
    NodeT && node, std::vector<std::string> topic_names,
    const rclcpp::SubscriptionOptionsWithAllocator<AllocatorT> & options =
    rclcpp::SubscriptionOptionsWithAllocator<AllocatorT>())
  : sub0_(topic_names[0], node),
    sub1_(topic_names[1], node)
  {
    clock_ptr_ = node->get_clock();
  }
  void registerCallback(
    std::function<void(
      const boost::optional<const std::shared_ptr<T0>> &,
      const boost::optional<const std::shared_ptr<T1>> &)> callback)
  {
    callback_ = callback;
  }

private:
  void query(rclcpp::Time stamp)
  {
    const auto msg0 = sub0_.query(stamp);
    const auto msg1 = sub1_.query(stamp);
    callback_(msg0, msg1);
  }
  std::shared_ptr<rclcpp::Clock> clock_ptr_;
  StampedMessageSubscriber<T0> sub0_;
  StampedMessageSubscriber<T1> sub1_;
  std::function<void(
      const boost::optional<const std::shared_ptr<T0>> &,
      const boost::optional<const std::shared_ptr<T1>> &)> callback_;
};
}  // namespace message_synchronizer

#endif  // MESSAGE_SYNCHRONIZER__MESSAGE_SYNCHRONIZER_HPP_
