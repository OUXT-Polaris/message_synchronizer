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
#include <deque>
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
template <typename M, bool is_adapter = rclcpp::is_type_adapter<M>::value>
struct message_type;

template <typename M>
struct message_type<M, true>
{
  using type = typename M::custom_type;
};

template <typename M>
struct message_type<M, false>
{
  using type = M;
};

template <typename M>
using message_type_t = typename message_type<M>::type;

template <typename MessageType, typename NativeMessageType = message_type_t<MessageType>>
class StampedMessageSubscriber
{
public:
  template <class NodeT, class AllocatorT = std::allocator<void>>
  StampedMessageSubscriber(
    const std::string & topic_name, NodeT && node, const std::chrono::milliseconds & poll_duration,
    const std::chrono::milliseconds & allow_delay,
    const rclcpp::SubscriptionOptionsWithAllocator<AllocatorT> & options =
      rclcpp::SubscriptionOptionsWithAllocator<AllocatorT>(),
    size_t buffer_size = 10,
    const std::function<rclcpp::Time(const NativeMessageType)> & get_timestamp_function =
      [](const auto & data) { return data.header.stamp; })
  : topic_name(topic_name),
    poll_duration(poll_duration),
    allow_delay(allow_delay),
    get_timestamp_function_(get_timestamp_function),
    buffer_size_(buffer_size)
  {
    if constexpr (
      rosidl_generator_traits::is_message<MessageType>::value ||
      rclcpp::is_type_adapter<MessageType>::value) {
      sub_ = rclcpp::create_subscription<MessageType>(
        node, topic_name, rclcpp::QoS(buffer_size),
        [this](const std::shared_ptr<NativeMessageType> msg) {
          NativeMessageType data = *msg;
          buffer_.push_back(data);
          while (buffer_.size() > buffer_size_) {
            buffer_.pop_front();
          }
        },
        options);
    }
  }
  std::optional<const NativeMessageType> query(const rclcpp::Time & stamp)
  {
    std::vector<double> diff;
    std::vector<NativeMessageType> messages;
    double poll_start_diff = std::chrono::duration<double>(poll_duration).count() * -1;
    double poll_end_diff = std::chrono::duration<double>(allow_delay).count();
    for (const auto & data : buffer_) {
      double diff_seconds = (stamp - get_timestamp_function_(data)).seconds() * -1;
      if (diff_seconds >= poll_start_diff && poll_end_diff >= diff_seconds) {
        diff.emplace_back(std::abs(diff_seconds));
        messages.emplace_back(data);
      }
    }
    if (diff.size() == 0) {
      return std::nullopt;
    }
    std::vector<double>::iterator iter = std::min_element(diff.begin(), diff.end());
    size_t index = std::distance(diff.begin(), iter);
    return messages[index];
  }
  const std::string topic_name;
  const std::chrono::milliseconds poll_duration;
  const std::chrono::milliseconds allow_delay;

private:
  double buffer_duration_;
  std::deque<NativeMessageType> buffer_;
  std::shared_ptr<rclcpp::Subscription<MessageType>> sub_;
  const std::function<rclcpp::Time(const NativeMessageType &)> get_timestamp_function_;
  const size_t buffer_size_;
};

class SynchronizerBase
{
public:
  template <class NodeT, class AllocatorT = std::allocator<void>>
  SynchronizerBase(
    NodeT && node, const std::chrono::milliseconds & poll_duration,
    const std::chrono::milliseconds & allow_delay)
  : poll_duration(poll_duration), allow_delay(allow_delay)
  {
    callback_registered_ = false;
    clock_ptr_ = node->get_clock();
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

template <
  typename MessageType0, typename MessageType1,
  typename NativeMessageType0 = message_type_t<MessageType0>,
  typename NativeMessageType1 = message_type_t<MessageType1>>
class MessageSynchronizer2 : public SynchronizerBase
{
public:
  template <class NodeT, class AllocatorT = std::allocator<void>>
  MessageSynchronizer2(
    NodeT && node, const std::vector<std::string> & topic_names,
    const std::chrono::milliseconds & poll_duration, const std::chrono::milliseconds & allow_delay,
    const rclcpp::SubscriptionOptionsWithAllocator<AllocatorT> & options =
      rclcpp::SubscriptionOptionsWithAllocator<AllocatorT>(),
    const std::function<rclcpp::Time(const NativeMessageType0 &)> & get_timestamp_function_topic0 =
      [](const auto & data) { return data.header.stamp; },
    const std::function<rclcpp::Time(const NativeMessageType1 &)> & get_timestamp_function_topic1 =
      [](const auto & data) { return data.header.stamp; })
  : SynchronizerBase(node, poll_duration, allow_delay),
    sub0_(
      topic_names[0], node, poll_duration, allow_delay, options, 10, get_timestamp_function_topic0),
    sub1_(
      topic_names[1], node, poll_duration, allow_delay, options, 10, get_timestamp_function_topic1)
  {
    timer_ = node->create_wall_timer(poll_duration, std::bind(&MessageSynchronizer2::poll, this));
  }
  void registerCallback(
    std::function<
      void(const std::optional<NativeMessageType0> &, const std::optional<NativeMessageType1> &)>
      callback)
  {
    callback_ = callback;
    callback_registered_ = true;
  }
  void poll() override
  {
    poll_timestamp_ = clock_ptr_->now();
    if (callback_registered_) {
      callback_(
        sub0_.query(poll_timestamp_ - allow_delay), sub1_.query(poll_timestamp_ - allow_delay));
    }
  }

private:
  StampedMessageSubscriber<MessageType0> sub0_;
  StampedMessageSubscriber<MessageType1> sub1_;
  std::function<void(
    const std::optional<const NativeMessageType0> &,
    const std::optional<const NativeMessageType1> &)>
    callback_;
};

template <
  typename MessageType0, typename MessageType1, typename MessageType2,
  typename NativeMessageType0 = message_type_t<MessageType0>,
  typename NativeMessageType1 = message_type_t<MessageType1>,
  typename NativeMessageType2 = message_type_t<MessageType2>>
class MessageSynchronizer3 : public SynchronizerBase
{
public:
  template <class NodeT, class AllocatorT = std::allocator<void>>
  MessageSynchronizer3(
    NodeT && node, const std::vector<std::string> & topic_names,
    const std::chrono::milliseconds & poll_duration, const std::chrono::milliseconds & allow_delay,
    const rclcpp::SubscriptionOptionsWithAllocator<AllocatorT> & options =
      rclcpp::SubscriptionOptionsWithAllocator<AllocatorT>(),
    const std::function<rclcpp::Time(const NativeMessageType0 &)> & get_timestamp_function_topic0 =
      [](const auto & data) { return data.header.stamp; },
    const std::function<rclcpp::Time(const NativeMessageType1 &)> & get_timestamp_function_topic1 =
      [](const auto & data) { return data.header.stamp; },
    const std::function<rclcpp::Time(const NativeMessageType2 &)> & get_timestamp_function_topic2 =
      [](const auto & data) { return data.header.stamp; })
  : SynchronizerBase(node, poll_duration, allow_delay),
    sub0_(
      topic_names[0], node, poll_duration, allow_delay, options, 10, get_timestamp_function_topic0),
    sub1_(
      topic_names[1], node, poll_duration, allow_delay, options, 10, get_timestamp_function_topic1),
    sub2_(
      topic_names[2], node, poll_duration, allow_delay, options, 10, get_timestamp_function_topic2)
  {
    timer_ = node->create_wall_timer(poll_duration, std::bind(&MessageSynchronizer3::poll, this));
  }
  void registerCallback(
    std::function<void(
      const std::optional<NativeMessageType0> &, const std::optional<NativeMessageType1> &,
      const std::optional<NativeMessageType2> &)>
      callback)
  {
    callback_ = callback;
    callback_registered_ = true;
  }
  void poll() override
  {
    poll_timestamp_ = clock_ptr_->now();
    if (callback_registered_) {
      callback_(
        sub0_.query(poll_timestamp_ - allow_delay), sub1_.query(poll_timestamp_ - allow_delay),
        sub2_.query(poll_timestamp_ - allow_delay));
    }
  }

private:
  StampedMessageSubscriber<MessageType0> sub0_;
  StampedMessageSubscriber<MessageType1> sub1_;
  StampedMessageSubscriber<MessageType2> sub2_;
  std::function<void(
    const std::optional<const NativeMessageType0> &,
    const std::optional<const NativeMessageType1> &,
    const std::optional<const NativeMessageType2> &)>
    callback_;
};

template <
  typename MessageType0, typename MessageType1, typename MessageType2, typename MessageType3,
  typename NativeMessageType0 = message_type_t<MessageType0>,
  typename NativeMessageType1 = message_type_t<MessageType1>,
  typename NativeMessageType2 = message_type_t<MessageType2>,
  typename NativeMessageType3 = message_type_t<MessageType3>>
class MessageSynchronizer4 : public SynchronizerBase
{
public:
  template <class NodeT, class AllocatorT = std::allocator<void>>
  MessageSynchronizer4(
    NodeT && node, const std::vector<std::string> & topic_names,
    const std::chrono::milliseconds & poll_duration, const std::chrono::milliseconds & allow_delay,
    const rclcpp::SubscriptionOptionsWithAllocator<AllocatorT> & options =
      rclcpp::SubscriptionOptionsWithAllocator<AllocatorT>(),
    const std::function<rclcpp::Time(const NativeMessageType0 &)> & get_timestamp_function_topic0 =
      [](const auto & data) { return data.header.stamp; },
    const std::function<rclcpp::Time(const NativeMessageType1 &)> & get_timestamp_function_topic1 =
      [](const auto & data) { return data.header.stamp; },
    const std::function<rclcpp::Time(const NativeMessageType2 &)> & get_timestamp_function_topic2 =
      [](const auto & data) { return data.header.stamp; },
    const std::function<rclcpp::Time(const NativeMessageType3 &)> & get_timestamp_function_topic3 =
      [](const auto & data) { return data.header.stamp; })
  : SynchronizerBase(node, poll_duration, allow_delay),
    sub0_(
      topic_names[0], node, poll_duration, allow_delay, options, 10, get_timestamp_function_topic0),
    sub1_(
      topic_names[1], node, poll_duration, allow_delay, options, 10, get_timestamp_function_topic1),
    sub2_(
      topic_names[2], node, poll_duration, allow_delay, options, 10, get_timestamp_function_topic2),
    sub3_(
      topic_names[3], node, poll_duration, allow_delay, options, 10, get_timestamp_function_topic3)
  {
    timer_ = node->create_wall_timer(poll_duration, std::bind(&MessageSynchronizer4::poll, this));
  }
  void registerCallback(
    std::function<void(
      const std::optional<NativeMessageType0> &, const std::optional<NativeMessageType1> &,
      const std::optional<NativeMessageType2> &, const std::optional<NativeMessageType3> &)>
      callback)
  {
    callback_ = callback;
    callback_registered_ = true;
  }
  void poll() override
  {
    poll_timestamp_ = clock_ptr_->now();
    if (callback_registered_) {
      callback_(
        sub0_.query(poll_timestamp_ - allow_delay), sub1_.query(poll_timestamp_ - allow_delay),
        sub2_.query(poll_timestamp_ - allow_delay), sub3_.query(poll_timestamp_ - allow_delay));
    }
  }

private:
  StampedMessageSubscriber<MessageType0> sub0_;
  StampedMessageSubscriber<MessageType1> sub1_;
  StampedMessageSubscriber<MessageType2> sub2_;
  StampedMessageSubscriber<MessageType3> sub3_;
  std::function<void(
    const std::optional<const NativeMessageType0> &,
    const std::optional<const NativeMessageType1> &,
    const std::optional<const NativeMessageType2> &,
    const std::optional<const NativeMessageType3> &)>
    callback_;
};

}  // namespace message_synchronizer

#endif  // MESSAGE_SYNCHRONIZER__MESSAGE_SYNCHRONIZER_HPP_
