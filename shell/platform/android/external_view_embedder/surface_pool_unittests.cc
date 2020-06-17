// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/fml/make_copyable.h"
#include "flutter/shell/platform/android/external_view_embedder/surface_pool.h"
#include "flutter/shell/platform/android/jni/mock_jni.h"
#include "flutter/shell/platform/android/surface/mock_android_surface.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "third_party/skia/include/gpu/GrContext.h"

namespace flutter {
namespace testing {

using ::testing::ByMove;
using ::testing::Return;

TEST(SurfacePool, GetLayer__AllocateOneLayer) {
  auto pool = std::make_unique<SurfacePool>();

  auto gr_context = GrContext::MakeMock(nullptr);
  auto android_context =
      std::make_shared<AndroidContext>(AndroidRenderingAPI::kSoftware);

  auto mock_jni = std::make_shared<MockJNI>();
  auto window = fml::MakeRefCounted<AndroidNativeWindow>(nullptr);
  EXPECT_CALL(*mock_jni, FlutterViewCreateOverlaySurface())
      .WillOnce(Return(
          ByMove(std::make_unique<PlatformViewAndroidJNI::OverlayMetadata>(
              0, window))));

  auto surface_factory =
      [gr_context, window](std::shared_ptr<AndroidContext> android_context,
                           std::shared_ptr<PlatformViewAndroidJNI> jni_facade) {
        auto mock_surface = std::make_unique<MockAndroidSurface>();
        EXPECT_CALL(*mock_surface, CreateGPUSurface(gr_context.get()));
        EXPECT_CALL(*mock_surface, SetNativeWindow(window));
        return mock_surface;
      };
  auto layer = pool->GetLayer(gr_context.get(), android_context, mock_jni,
                              surface_factory);

  ASSERT_NE(nullptr, layer);
  ASSERT_EQ(gr_context.get(), layer->gr_context);
}

TEST(SurfacePool, GetUnusedLayers) {
  auto pool = std::make_unique<SurfacePool>();

  auto gr_context = GrContext::MakeMock(nullptr);
  auto android_context =
      std::make_shared<AndroidContext>(AndroidRenderingAPI::kSoftware);

  auto mock_jni = std::make_shared<MockJNI>();
  auto window = fml::MakeRefCounted<AndroidNativeWindow>(nullptr);
  EXPECT_CALL(*mock_jni, FlutterViewCreateOverlaySurface())
      .WillOnce(Return(
          ByMove(std::make_unique<PlatformViewAndroidJNI::OverlayMetadata>(
              0, window))));

  auto surface_factory =
      [gr_context, window](std::shared_ptr<AndroidContext> android_context,
                           std::shared_ptr<PlatformViewAndroidJNI> jni_facade) {
        auto mock_surface = std::make_unique<MockAndroidSurface>();
        EXPECT_CALL(*mock_surface, CreateGPUSurface(gr_context.get()));
        EXPECT_CALL(*mock_surface, SetNativeWindow(window));
        return mock_surface;
      };
  auto layer = pool->GetLayer(gr_context.get(), android_context, mock_jni,
                              surface_factory);
  ASSERT_EQ(0UL, pool->GetUnusedLayers().size());

  pool->RecycleLayers();

  ASSERT_EQ(1UL, pool->GetUnusedLayers().size());
  ASSERT_EQ(layer, pool->GetUnusedLayers()[0]);
}

TEST(SurfacePool, GetLayer__Recycle) {
  auto pool = std::make_unique<SurfacePool>();

  auto gr_context_1 = GrContext::MakeMock(nullptr);
  auto mock_jni = std::make_shared<MockJNI>();
  auto window = fml::MakeRefCounted<AndroidNativeWindow>(nullptr);
  EXPECT_CALL(*mock_jni, FlutterViewCreateOverlaySurface())
      .WillOnce(Return(
          ByMove(std::make_unique<PlatformViewAndroidJNI::OverlayMetadata>(
              0, window))));

  auto android_context =
      std::make_shared<AndroidContext>(AndroidRenderingAPI::kSoftware);

  auto gr_context_2 = GrContext::MakeMock(nullptr);
  auto surface_factory =
      [gr_context_1, gr_context_2, window](
          std::shared_ptr<AndroidContext> android_context,
          std::shared_ptr<PlatformViewAndroidJNI> jni_facade) {
        auto mock_surface = std::make_unique<MockAndroidSurface>();
        // Allocate two GPU surfaces for each gr context.
        EXPECT_CALL(*mock_surface, CreateGPUSurface(gr_context_1.get()));
        EXPECT_CALL(*mock_surface, CreateGPUSurface(gr_context_2.get()));
        // Set the native window once.
        EXPECT_CALL(*mock_surface, SetNativeWindow(window));
        return mock_surface;
      };
  auto layer_1 = pool->GetLayer(gr_context_1.get(), android_context, mock_jni,
                                surface_factory);

  pool->RecycleLayers();

  auto layer_2 = pool->GetLayer(gr_context_2.get(), android_context, mock_jni,
                                surface_factory);

  ASSERT_NE(nullptr, layer_1);
  ASSERT_EQ(layer_1, layer_2);
  ASSERT_EQ(gr_context_2.get(), layer_1->gr_context);
  ASSERT_EQ(gr_context_2.get(), layer_2->gr_context);
}

TEST(SurfacePool, GetLayer__AllocateTwoLayers) {
  auto pool = std::make_unique<SurfacePool>();

  auto gr_context = GrContext::MakeMock(nullptr);
  auto android_context =
      std::make_shared<AndroidContext>(AndroidRenderingAPI::kSoftware);

  auto mock_jni = std::make_shared<MockJNI>();
  auto window = fml::MakeRefCounted<AndroidNativeWindow>(nullptr);
  EXPECT_CALL(*mock_jni, FlutterViewCreateOverlaySurface())
      .Times(2)
      .WillOnce(Return(
          ByMove(std::make_unique<PlatformViewAndroidJNI::OverlayMetadata>(
              0, window))))
      .WillOnce(Return(
          ByMove(std::make_unique<PlatformViewAndroidJNI::OverlayMetadata>(
              1, window))));

  auto surface_factory =
      [gr_context, window](std::shared_ptr<AndroidContext> android_context,
                           std::shared_ptr<PlatformViewAndroidJNI> jni_facade) {
        auto mock_surface = std::make_unique<MockAndroidSurface>();
        EXPECT_CALL(*mock_surface, CreateGPUSurface(gr_context.get()));
        EXPECT_CALL(*mock_surface, SetNativeWindow(window));
        return mock_surface;
      };
  auto layer_1 = pool->GetLayer(gr_context.get(), android_context, mock_jni,
                                surface_factory);
  auto layer_2 = pool->GetLayer(gr_context.get(), android_context, mock_jni,
                                surface_factory);
  ASSERT_NE(nullptr, layer_1);
  ASSERT_NE(nullptr, layer_2);
  ASSERT_NE(layer_1, layer_2);
  ASSERT_EQ(0, layer_1->id);
  ASSERT_EQ(1, layer_2->id);
}

}  // namespace testing
}  // namespace flutter
