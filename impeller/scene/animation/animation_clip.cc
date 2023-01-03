// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "impeller/scene/animation/animation_clip.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <memory>
#include <valarray>

#include "impeller/scene/node.h"

namespace impeller {
namespace scene {

AnimationClip::AnimationClip(std::shared_ptr<Animation> animation,
                             Node* bind_target)
    : animation_(std::move(animation)) {
  BindToTarget(bind_target);
}

AnimationClip::~AnimationClip() = default;

AnimationClip::AnimationClip(AnimationClip&&) = default;
AnimationClip& AnimationClip::operator=(AnimationClip&&) = default;

bool AnimationClip::IsPlaying() const {
  return playing_;
}

void AnimationClip::SetPlaying(bool playing) {
  playing_ = playing;
}

void AnimationClip::Play() {
  SetPlaying(true);
}

void AnimationClip::Pause() {
  SetPlaying(false);
}

void AnimationClip::Stop() {
  SetPlaying(false);
  Seek(std::chrono::duration<Scalar>::zero());
}

bool AnimationClip::GetLoop() const {
  return loop_;
}

void AnimationClip::SetLoop(bool looping) {
  loop_ = looping;
}

Scalar AnimationClip::GetPlaybackTimeScale() const {
  return playback_time_scale_;
}

void AnimationClip::SetPlaybackTimeScale(Scalar playback_speed) {
  playback_time_scale_ = playback_speed;
}

Scalar AnimationClip::GetWeight() const {
  return weight_;
}

void AnimationClip::SetWeight(Scalar weight) {
  weight_ = weight;
}

std::chrono::duration<Scalar> AnimationClip::GetPlaybackTime() const {
  return playback_time_;
}

void AnimationClip::Seek(std::chrono::duration<Scalar> time) {
  playback_time_ = std::clamp(time, std::chrono::duration<Scalar>::zero(),
                              animation_->GetEndTime());
}

void AnimationClip::Advance(std::chrono::duration<Scalar> delta_time) {
  if (!playing_ || delta_time <= std::chrono::duration<Scalar>::zero()) {
    return;
  }
  delta_time *= playback_time_scale_;
  playback_time_ += delta_time;

  /// Handle looping behavior.

  auto end_time = animation_->GetEndTime();
  if (end_time == std::chrono::duration<Scalar>::zero()) {
    playback_time_ = std::chrono::duration<Scalar>::zero();
    return;
  }
  if (!loop_ && (playback_time_ < std::chrono::duration<Scalar>::zero() ||
                 playback_time_ > end_time)) {
    // If looping is disabled, clamp to the end (or beginning, if playing in
    // reverse) and pause.
    Pause();
    playback_time_ = std::clamp(
        playback_time_, std::chrono::duration<Scalar>::zero(), end_time);
  } else if (/* loop && */ playback_time_ > end_time) {
    // If looping is enabled and we ran off the end, loop to the beginning.
    playback_time_ = std::chrono::duration<Scalar>(
        std::fmod(std::abs(playback_time_.count()), end_time.count()));
  } else if (/* loop && */ playback_time_ <
             std::chrono::duration<Scalar>::zero()) {
    // If looping is enabled and we ran off the beginning, loop to the end.
    playback_time_ =
        end_time - std::chrono::duration<Scalar>(std::fmod(
                       std::abs(playback_time_.count()), end_time.count()));
  }
}

void AnimationClip::ApplyToBindings() const {
  for (auto& binding : bindings_) {
    binding.channel.resolver->Apply(*binding.node, playback_time_, weight_);
  }
}

void AnimationClip::BindToTarget(Node* node) {
  const auto& channels = animation_->GetChannels();
  bindings_.clear();
  bindings_.reserve(channels.size());

  for (const auto& channel : channels) {
    Node* channel_target;
    if (channel.bind_target.node_name == node->GetName()) {
      channel_target = node;
    } else if (auto result =
                   node->FindChildByName(channel.bind_target.node_name, true)) {
      channel_target = result.get();
    } else {
      continue;
    }
    bindings_.push_back(
        ChannelBinding{.channel = channel, .node = channel_target});
  }
}

}  // namespace scene
}  // namespace impeller
