// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/common/client_wrapper/include/flutter/basic_message_channel.h"

#include <memory>
#include <string>

#include "flutter/shell/platform/common/client_wrapper/include/flutter/binary_messenger.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/standard_message_codec.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/standard_method_codec.h"
#include "gtest/gtest.h"

namespace flutter {

namespace {

class TestBinaryMessenger : public BinaryMessenger {
 public:
  void Send(const std::string& channel,
            const uint8_t* message,
            const size_t message_size,
            BinaryReply reply) const override {
    send_called_ = true;
    int length = static_cast<int>(message_size);
    last_message_size_ = length;
    last_message_ =
        std::vector<uint8_t>(message, message + length * sizeof(uint8_t));
  }

  void SetMessageHandler(const std::string& channel,
                         BinaryMessageHandler handler) override {
    last_message_handler_channel_ = channel;
    last_message_handler_ = handler;
  }

  bool send_called() { return send_called_; }

  std::string last_message_handler_channel() {
    return last_message_handler_channel_;
  }

  BinaryMessageHandler last_message_handler() { return last_message_handler_; }

  std::vector<uint8_t> last_message() { return last_message_; }

  int last_message_size() { return last_message_size_; }

 private:
  mutable bool send_called_ = false;
  std::string last_message_handler_channel_;
  BinaryMessageHandler last_message_handler_;
  mutable std::vector<uint8_t> last_message_;
  mutable int last_message_size_;
};

}  // namespace

// Tests that SetMessageHandler sets a handler that correctly interacts with
// the binary messenger.
TEST(BasicMessageChannelTest, Registration) {
  TestBinaryMessenger messenger;
  const std::string channel_name("some_channel");
  const StandardMessageCodec& codec = StandardMessageCodec::GetInstance();
  BasicMessageChannel channel(&messenger, channel_name, &codec);

  bool callback_called = false;
  const std::string message_value("hello");
  channel.SetMessageHandler(
      [&callback_called, message_value](const auto& message, auto reply) {
        callback_called = true;
        // Ensure that the wrapper received a correctly decoded message and a
        // reply.
        EXPECT_EQ(std::get<std::string>(message), message_value);
        EXPECT_NE(reply, nullptr);
      });
  EXPECT_EQ(messenger.last_message_handler_channel(), channel_name);
  EXPECT_NE(messenger.last_message_handler(), nullptr);
  // Send a test message to trigger the handler test assertions.
  auto message = codec.EncodeMessage(EncodableValue(message_value));

  messenger.last_message_handler()(
      message->data(), message->size(),
      [](const uint8_t* reply, const size_t reply_size) {});
  EXPECT_EQ(callback_called, true);
}

// Tests that SetMessageHandler with a null handler unregisters the handler.
TEST(BasicMessageChannelTest, Unregistration) {
  TestBinaryMessenger messenger;
  const std::string channel_name("some_channel");
  BasicMessageChannel channel(&messenger, channel_name,
                              &flutter::StandardMessageCodec::GetInstance());

  channel.SetMessageHandler([](const auto& message, auto reply) {});
  EXPECT_EQ(messenger.last_message_handler_channel(), channel_name);
  EXPECT_NE(messenger.last_message_handler(), nullptr);

  channel.SetMessageHandler(nullptr);
  EXPECT_EQ(messenger.last_message_handler_channel(), channel_name);
  EXPECT_EQ(messenger.last_message_handler(), nullptr);
}

// Tests that calling Resize generates the binary message expected by the Dart
// implementation.
TEST(BasicMessageChannelTest, Resize) {
  TestBinaryMessenger messenger;
  const std::string channel_name("flutter/test");
  BasicMessageChannel channel(&messenger, channel_name,
                              &flutter::StandardMessageCodec::GetInstance());

  channel.Resize(3);

  // The expected content was created from the following Dart code:
  //   MethodCall call = MethodCall('resize', ['flutter/test',3]);
  //   StandardMethodCodec().encodeMethodCall(call).buffer.asUint8List();
  const int expected_message_size = 29;

  EXPECT_EQ(messenger.send_called(), true);
  EXPECT_EQ(messenger.last_message_size(), expected_message_size);

  int expected[expected_message_size] = {
      7,   6,   114, 101, 115, 105, 122, 101, 12,  2, 7, 12, 102, 108, 117,
      116, 116, 101, 114, 47,  116, 101, 115, 116, 3, 3, 0,  0,   0};
  for (int i = 0; i < expected_message_size; i++) {
    EXPECT_EQ(messenger.last_message()[i], expected[i]);
  }
}

// Tests that calling SetWarnsOnOverflow generates the binary message expected
// by the Dart implementation.
TEST(BasicMessageChannelTest, SetWarnsOnOverflow) {
  TestBinaryMessenger messenger;

  const std::string channel_name("flutter/test");
  BasicMessageChannel channel(&messenger, channel_name,
                              &flutter::StandardMessageCodec::GetInstance());

  channel.SetWarnsOnOverflow(false);

  // The expected content was created from the following Dart code:
  //   MethodCall call = MethodCall('overflow',['flutter/test', true]);
  //   StandardMethodCodec().encodeMethodCall(call).buffer.asUint8List();
  const int expected_message_size = 27;

  EXPECT_EQ(messenger.send_called(), true);
  EXPECT_EQ(messenger.last_message_size(), expected_message_size);

  int expected[expected_message_size] = {
      7,   8,   111, 118, 101, 114, 102, 108, 111, 119, 12,  2,   7, 12,
      102, 108, 117, 116, 116, 101, 114, 47,  116, 101, 115, 116, 1};
  for (int i = 0; i < expected_message_size; i++) {
    EXPECT_EQ(messenger.last_message()[i], expected[i]);
  }
}

}  // namespace flutter
