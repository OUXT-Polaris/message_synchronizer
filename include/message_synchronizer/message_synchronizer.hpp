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

#include <algorithm>
#include <boost/circular_buffer.hpp>
#include <boost/optional.hpp>
#include <exception>
#include <functional>
#include <memory>
#include <rclcpp/rclcpp.hpp>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace message_synchronizer
{
template <typename T>
class StampedMessageSubscriber
{
public:
  template <class NodeT, class AllocatorT = std::allocator<void>>
  StampedMessageSubscriber(
    const std::string & topic_name, NodeT && node, std::chrono::milliseconds poll_duration,
    std::chrono::milliseconds allow_delay,
    const rclcpp::SubscriptionOptionsWithAllocator<AllocatorT> & options =
      rclcpp::SubscriptionOptionsWithAllocator<AllocatorT>())
  : topic_name(topic_name), poll_duration(poll_duration), allow_delay(allow_delay)
  {
    auto callback = std::bind(&StampedMessageSubscriber::callback, this, std::placeholders::_1);
    buffer_ = boost::circular_buffer<T>(10);
    sub_ = rclcpp::create_subscription<T>(
      node, topic_name, rclcpp::QoS(10), std::move(callback), options);
  }
  boost::optional<const std::shared_ptr<T>> query(rclcpp::Time stamp)
  {
    std::vector<double> diff;
    std::vector<T> messages;
    for (const auto buf : buffer_) {
      const auto msg_stamp = buf.header.stamp;
      double poll_start_diff = std::chrono::duration<double>(poll_duration).count() * -1;
      double poll_end_diff = std::chrono::duration<double>(allow_delay).count();
      double diff_seconds = (stamp - msg_stamp).seconds() * -1;
      if (diff_seconds >= poll_start_diff && poll_end_diff >= diff_seconds) {
        diff.emplace_back(std::abs(diff_seconds));
        messages.emplace_back(buf);
      }
    }
    if (diff.size() == 0) {
      return boost::none;
    }
    std::vector<double>::iterator iter = std::min_element(diff.begin(), diff.end());
    size_t index = std::distance(diff.begin(), iter);
    return std::make_shared<T>(messages[index]);
  }
  const std::string topic_name;
  const std::chrono::milliseconds poll_duration;
  const std::chrono::milliseconds allow_delay;

private:
  double buffer_duration_;
  boost::circular_buffer<T> buffer_;
  std::shared_ptr<rclcpp::Subscription<T>> sub_;
  void callback(const std::shared_ptr<T> msg) { buffer_.push_back(*msg); }
};

class SynchronizerBase
{
public:
  template <class NodeT, class AllocatorT = std::allocator<void>>
  SynchronizerBase(
    NodeT && node, std::chrono::milliseconds poll_duration, std::chrono::milliseconds allow_delay)
  : poll_duration(poll_duration), allow_delay(allow_delay)
  {
    callback_registered_ = false;
    clock_ptr_ = node->get_clock();
    timer_ = node->create_wall_timer(poll_duration, std::bind(&SynchronizerBase::poll, this));
  }
  virtual void poll() = 0;
  const rclcpp::Time getPollTimestamp() { return poll_timestamp_; }
  const std::chrono::milliseconds poll_duration;
  const std::chrono::milliseconds allow_delay;

protected:
  rclcpp::Time poll_timestamp_;
  rclcpp::TimerBase::SharedPtr timer_;
  bool callback_registered_;
  std::shared_ptr<rclcpp::Clock> clock_ptr_;
};

template <typename T0, typename T1>
class MessageSynchronizer2 : public SynchronizerBase
{
public:
  template <class NodeT, class AllocatorT = std::allocator<void>>
  MessageSynchronizer2(
    NodeT && node, std::vector<std::string> topic_names, std::chrono::milliseconds poll_duration,
    std::chrono::milliseconds allow_delay,
    const rclcpp::SubscriptionOptionsWithAllocator<AllocatorT> & options =
      rclcpp::SubscriptionOptionsWithAllocator<AllocatorT>())
  : SynchronizerBase(node, poll_duration, allow_delay),
    sub0_(topic_names[0], node, poll_duration, allow_delay, options),
    sub1_(topic_names[1], node, poll_duration, allow_delay, options)
  {
  }
  void registerCallback(std::function<void(
                          const boost::optional<const std::shared_ptr<T0>> &,
                          const boost::optional<const std::shared_ptr<T1>> &)>
                          callback)
  {
    callback_ = callback;
    callback_registered_ = true;
  }
  void poll()
  {
    poll_timestamp_ = clock_ptr_->now();
    if (callback_registered_) {
      callback_(
        sub0_.query(poll_timestamp_ - allow_delay), sub1_.query(poll_timestamp_ - allow_delay));
    }
  }

private:
  StampedMessageSubscriber<T0> sub0_;
  StampedMessageSubscriber<T1> sub1_;
  std::function<void(
    const boost::optional<const std::shared_ptr<T0>> &,
    const boost::optional<const std::shared_ptr<T1>> &)>
    callback_;
};

template <typename T0, typename T1, typename T2>
class MessageSynchronizer3 : public SynchronizerBase
{
public:
  template <class NodeT, class AllocatorT = std::allocator<void>>
  MessageSynchronizer3(
    NodeT && node, std::vector<std::string> topic_names, std::chrono::milliseconds poll_duration,
    std::chrono::milliseconds allow_delay,
    const rclcpp::SubscriptionOptionsWithAllocator<AllocatorT> & options =
      rclcpp::SubscriptionOptionsWithAllocator<AllocatorT>())
  : SynchronizerBase(node, poll_duration, allow_delay),
    sub0_(topic_names[0], node, poll_duration, allow_delay, options),
    sub1_(topic_names[1], node, poll_duration, allow_delay, options),
    sub2_(topic_names[2], node, poll_duration, allow_delay, options)
  {
  }
  void registerCallback(std::function<void(
                          const boost::optional<const std::shared_ptr<T0>> &,
                          const boost::optional<const std::shared_ptr<T1>> &,
                          const boost::optional<const std::shared_ptr<T2>> &)>
                          callback)
  {
    callback_ = callback;
    callback_registered_ = true;
  }
  void poll()
  {
    poll_timestamp_ = clock_ptr_->now();
    if (callback_registered_) {
      callback_(
        sub0_.query(poll_timestamp_ - allow_delay), sub1_.query(poll_timestamp_ - allow_delay),
        sub2_.query(poll_timestamp_ - allow_delay));
    }
  }

private:
  StampedMessageSubscriber<T0> sub0_;
  StampedMessageSubscriber<T1> sub1_;
  StampedMessageSubscriber<T2> sub2_;
  std::function<void(
    const boost::optional<const std::shared_ptr<T0>> &,
    const boost::optional<const std::shared_ptr<T1>> &,
    const boost::optional<const std::shared_ptr<T2>> &)>
    callback_;
};

template <typename T0, typename T1, typename T2, typename T3>
class MessageSynchronizer4 : public SynchronizerBase
{
public:
  template <class NodeT, class AllocatorT = std::allocator<void>>
  MessageSynchronizer4(
    NodeT && node, std::vector<std::string> topic_names, std::chrono::milliseconds poll_duration,
    std::chrono::milliseconds allow_delay,
    const rclcpp::SubscriptionOptionsWithAllocator<AllocatorT> & options =
      rclcpp::SubscriptionOptionsWithAllocator<AllocatorT>())
  : SynchronizerBase(node, poll_duration, allow_delay),
    sub0_(topic_names[0], node, poll_duration, allow_delay, options),
    sub1_(topic_names[1], node, poll_duration, allow_delay, options),
    sub2_(topic_names[2], node, poll_duration, allow_delay, options),
    sub3_(topic_names[3], node, poll_duration, allow_delay, options)
  {
  }
  void registerCallback(std::function<void(
                          const boost::optional<const std::shared_ptr<T0>> &,
                          const boost::optional<const std::shared_ptr<T1>> &,
                          const boost::optional<const std::shared_ptr<T2>> &,
                          const boost::optional<const std::shared_ptr<T3>> &)>
                          callback)
  {
    callback_ = callback;
    callback_registered_ = true;
  }
  void poll()
  {
    poll_timestamp_ = clock_ptr_->now();
    if (callback_registered_) {
      callback_(
        sub0_.query(poll_timestamp_ - allow_delay), sub1_.query(poll_timestamp_ - allow_delay),
        sub2_.query(poll_timestamp_ - allow_delay), sub3_.query(poll_timestamp_ - allow_delay));
    }
  }

private:
  StampedMessageSubscriber<T0> sub0_;
  StampedMessageSubscriber<T1> sub1_;
  StampedMessageSubscriber<T2> sub2_;
  StampedMessageSubscriber<T3> sub3_;
  std::function<void(
    const boost::optional<const std::shared_ptr<T0>> &,
    const boost::optional<const std::shared_ptr<T1>> &,
    const boost::optional<const std::shared_ptr<T2>> &,
    const boost::optional<const std::shared_ptr<T3>> &)>
    callback_;
};

}  // namespace message_synchronizer

#endif  // MESSAGE_SYNCHRONIZER__MESSAGE_SYNCHRONIZER_HPP_
