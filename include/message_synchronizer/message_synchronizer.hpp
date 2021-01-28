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
  boost::optional<const std::shared_ptr<T>> query(rclcpp::Time stamp)
  {
    std::vector<double> diff;
    for (const auto buf :  buffer_) {
      const auto msg_stamp = buf.header.stamp;
      diff.emplace_back(std::fabs((stamp - msg_stamp).seconds()));
    }
    if (diff.size() == 0) {
      return boost::none;
    }
    std::vector<double>::iterator iter = std::min_element(diff.begin(), diff.end());
    size_t index = std::distance(diff.begin(), iter);
    return std::make_shared<T>(buffer_[index]);
  }

private:
  double buffer_duration_;
  boost::circular_buffer<T> buffer_;
  std::shared_ptr<rclcpp::Subscription<T>> sub_;
  void callback(const std::shared_ptr<T> msg)
  {
    buffer_.push_back(*msg);
  }
};


class SynchronizerBase
{
public:
  template<class NodeT, class AllocatorT = std::allocator<void>>
  SynchronizerBase(
    NodeT && node,
    std::chrono::milliseconds poll_duration,
    std::chrono::milliseconds allow_delay)
  {
    callback_registered_ = false;
    clock_ptr_ = node->get_clock();
    timer_ = node->create_wall_timer(poll_duration, std::bind(&SynchronizerBase::poll, this));
  }
  virtual void poll() = 0;

protected:
  rclcpp::TimerBase::SharedPtr timer_;
  bool callback_registered_;
  std::shared_ptr<rclcpp::Clock> clock_ptr_;
};

template<typename T0, typename T1>
class MessageSynchronizer : public SynchronizerBase
{
public:
  template<class NodeT, class AllocatorT = std::allocator<void>>
  MessageSynchronizer(
    NodeT && node, std::vector<std::string> topic_names,
    std::chrono::milliseconds poll_duration,
    std::chrono::milliseconds allow_delay,
    const rclcpp::SubscriptionOptionsWithAllocator<AllocatorT> & options =
    rclcpp::SubscriptionOptionsWithAllocator<AllocatorT>())
  : SynchronizerBase(node, poll_duration, allow_delay),
    sub0_(topic_names[0], node, options),
    sub1_(topic_names[1], node, options) {}
  void registerCallback(
    std::function<void(
      const boost::optional<const std::shared_ptr<T0>> &,
      const boost::optional<const std::shared_ptr<T1>> &)> callback)
  {
    callback_ = callback;
    callback_registered_ = true;
  }
  void poll() override
  {
    if (callback_registered_) {
      rclcpp::Time stamp = clock_ptr_->now();
      callback_(sub0_.query(stamp), sub1_.query(stamp));
    }
  }

private:
  StampedMessageSubscriber<T0> sub0_;
  StampedMessageSubscriber<T1> sub1_;
  std::function<void(
      const boost::optional<const std::shared_ptr<T0>> &,
      const boost::optional<const std::shared_ptr<T1>> &)> callback_;
};
}  // namespace message_synchronizer

#endif  // MESSAGE_SYNCHRONIZER__MESSAGE_SYNCHRONIZER_HPP_
