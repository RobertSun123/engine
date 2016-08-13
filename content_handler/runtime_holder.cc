// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/content_handler/runtime_holder.h"

#include <utility>

#include "flutter/common/threads.h"
#include "flutter/content_handler/rasterizer.h"
#include "flutter/runtime/dart_controller.h"
#include "flutter/services/engine/sky_engine.mojom.h"
#include "lib/ftl/functional/make_copyable.h"
#include "lib/ftl/functional/make_runnable.h"
#include "lib/ftl/logging.h"
#include "lib/ftl/time/time_delta.h"
#include "mojo/public/cpp/application/connect.h"

namespace flutter_content_handler {
namespace {

constexpr int kPipelineDepth = 3;
constexpr ftl::TimeDelta kTargetFrameInterval =
    ftl::TimeDelta::FromMilliseconds(16);

}  // namespace

RuntimeHolder::RuntimeHolder() : weak_factory_(this) {}

RuntimeHolder::~RuntimeHolder() {
  blink::Threads::Gpu()->PostTask(
      ftl::MakeCopyable([rasterizer = std::move(rasterizer_)](){
          // Deletes rasterizer.
      }));
}

void RuntimeHolder::Init(mojo::ApplicationConnectorPtr connector) {
  FTL_DCHECK(!rasterizer_);
  rasterizer_.reset(new Rasterizer());
  mojo::ConnectToService(connector.get(), "mojo:framebuffer",
                         mojo::GetProxy(&framebuffer_provider_));
  framebuffer_provider_->Create(ftl::MakeRunnable([self = GetWeakPtr()](
      mojo::InterfaceHandle<mojo::Framebuffer> framebuffer,
      mojo::FramebufferInfoPtr info) {
    if (self)
      self->DidCreateFramebuffer(std::move(framebuffer), std::move(info));
  }));
  runtime_ = blink::RuntimeController::Create(this);
}

void RuntimeHolder::Run(const std::string& script_uri,
                        std::vector<char> snapshot) {
  runtime_->CreateDartController(script_uri);
  runtime_->dart_controller()->RunFromSnapshot(
      reinterpret_cast<const uint8_t*>(snapshot.data()), snapshot.size());
}

void RuntimeHolder::ScheduleFrame() {
  if (runtime_requested_frame_)
    return;
  runtime_requested_frame_ = true;

  FTL_DCHECK(!did_defer_frame_request_);
  ++outstanding_requests_;

  if (outstanding_requests_ >= kPipelineDepth) {
    did_defer_frame_request_ = true;
    return;
  }

  ScheduleDelayedFrame();
}

void RuntimeHolder::Render(std::unique_ptr<flow::LayerTree> layer_tree) {
  if (!is_ready_to_draw_)
    return;  // Only draw once per frame.
  is_ready_to_draw_ = false;

  blink::Threads::Gpu()->PostTask(ftl::MakeCopyable([
    rasterizer = rasterizer_.get(), layer_tree = std::move(layer_tree),
    self = GetWeakPtr()
  ]() mutable {
    rasterizer->Draw(std::move(layer_tree), [self]() {
      if (self)
        self->OnFrameComplete();
    });
  }));
}

void RuntimeHolder::DidCreateFramebuffer(
    mojo::InterfaceHandle<mojo::Framebuffer> framebuffer,
    mojo::FramebufferInfoPtr info) {
  auto viewport_metrics = sky::ViewportMetrics::New();
  viewport_metrics->physical_width = info->size->width;
  viewport_metrics->physical_height = info->size->height;
  runtime_->SetViewportMetrics(viewport_metrics);

  blink::Threads::Gpu()->PostTask(ftl::MakeCopyable([
    rasterizer = rasterizer_.get(), framebuffer = std::move(framebuffer),
    info = std::move(info)
  ]() mutable {
    rasterizer->SetFramebuffer(std::move(framebuffer), std::move(info));
  }));
}

ftl::WeakPtr<RuntimeHolder> RuntimeHolder::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

void RuntimeHolder::ScheduleDelayedFrame() {
  // TODO(abarth): We should align with vsync or with our own timer pulse.
  blink::Threads::UI()->PostDelayedTask(
      [self = GetWeakPtr()]() {
        if (self)
          self->BeginFrame();
      },
      kTargetFrameInterval);
}

void RuntimeHolder::BeginFrame() {
  FTL_DCHECK(outstanding_requests_ > 0);
  FTL_DCHECK(outstanding_requests_ <= kPipelineDepth) << outstanding_requests_;

  FTL_DCHECK(runtime_requested_frame_);
  runtime_requested_frame_ = false;

  FTL_DCHECK(!is_ready_to_draw_);
  is_ready_to_draw_ = true;
  runtime_->BeginFrame(ftl::TimePoint::Now());
  const bool was_ready_to_draw = is_ready_to_draw_;
  is_ready_to_draw_ = false;

  // If we were still ready to draw when done with the frame, that means we
  // didn't draw anything this frame and we should acknowledge the frame
  // ourselves instead of waiting for the rasterizer to acknowledge it.
  if (was_ready_to_draw)
    OnFrameComplete();
}

void RuntimeHolder::OnFrameComplete() {
  FTL_DCHECK(outstanding_requests_ > 0);
  --outstanding_requests_;

  if (did_defer_frame_request_) {
    did_defer_frame_request_ = false;
    ScheduleDelayedFrame();
  }
}

}  // namespace flutter_content_handler
