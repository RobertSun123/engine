// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/sky/shell/diagnostic/diagnostic_server.h"

#include "dart/runtime/include/dart_api.h"
#include "dart/runtime/include/dart_native_api.h"
#include "flutter/common/threads.h"
#include "flutter/flow/compositor_context.h"
#include "flutter/sky/engine/core/script/embedder_resources.h"
#include "flutter/sky/shell/gpu/picture_serializer.h"
#include "flutter/sky/shell/rasterizer.h"
#include "flutter/sky/shell/shell.h"
#include "flutter/sky/shell/ui/engine.h"
#include "lib/ftl/logging.h"
#include "lib/tonic/dart_binding_macros.h"
#include "lib/tonic/dart_library_natives.h"
#include "lib/tonic/logging/dart_invoke.h"
#include "third_party/skia/include/core/SkStream.h"

namespace mojo {
namespace dart {
extern ResourcesEntry __sky_embedder_diagnostic_server_resources_[];
}
}

namespace sky {
namespace shell {

using tonic::DartLibraryNatives;
using blink::EmbedderResources;
using tonic::DartInvokeField;
using tonic::LogIfError;

namespace {

DartLibraryNatives* g_natives = nullptr;

constexpr char kDiagnosticServerScript[] = "/diagnostic_server.dart";

Dart_NativeFunction GetNativeFunction(Dart_Handle name,
                                      int argument_count,
                                      bool* auto_setup_scope) {
  FTL_CHECK(g_natives);
  return g_natives->GetNativeFunction(name, argument_count, auto_setup_scope);
}

const uint8_t* GetSymbol(Dart_NativeFunction native_function) {
  FTL_CHECK(g_natives);
  return g_natives->GetSymbol(native_function);
}

void SendNull(Dart_Port port_id) {
  Dart_CObject null_object;
  null_object.type = Dart_CObject_kNull;
  Dart_PostCObject(port_id, &null_object);
}

}  // namespace

DART_NATIVE_CALLBACK_STATIC(DiagnosticServer, HandleSkiaPictureRequest);

void DiagnosticServer::Start() {
  if (!g_natives) {
    g_natives = new DartLibraryNatives();
    g_natives->Register({
        DART_REGISTER_NATIVE_STATIC(DiagnosticServer, HandleSkiaPictureRequest),
    });
  }

  EmbedderResources resources(
      &mojo::dart::__sky_embedder_diagnostic_server_resources_[0]);

  const char* source = nullptr;
  int source_length =
      resources.ResourceLookup(kDiagnosticServerScript, &source);
  FTL_DCHECK(source_length != EmbedderResources::kNoSuchInstance);

  Dart_Handle diagnostic_library = Dart_LoadLibrary(
      Dart_NewStringFromCString("dart:diagnostic_server"), Dart_Null(),
      Dart_NewStringFromUTF8(reinterpret_cast<const uint8_t*>(source),
                             source_length),
      0, 0);

  FTL_CHECK(!LogIfError(diagnostic_library));
  FTL_CHECK(!LogIfError(Dart_SetNativeResolver(diagnostic_library,
                                               GetNativeFunction, GetSymbol)));

  FTL_CHECK(!LogIfError(Dart_LibraryImportLibrary(
      Dart_RootLibrary(), diagnostic_library, Dart_Null())));

  FTL_CHECK(!LogIfError(Dart_FinalizeLoading(false)));

  DartInvokeField(Dart_RootLibrary(), "diagnosticServerStart", {});
}

void DiagnosticServer::HandleSkiaPictureRequest(Dart_Handle send_port) {
  Dart_Port port_id;
  FTL_CHECK(!LogIfError(Dart_SendPortGetId(send_port, &port_id)));

  blink::Threads::Gpu()->PostTask([port_id]() { SkiaPictureTask(port_id); });
}

void DiagnosticServer::SkiaPictureTask(Dart_Port port_id) {
  std::vector<ftl::WeakPtr<Rasterizer>> rasterizers;
  Shell::Shared().GetRasterizers(&rasterizers);
  if (rasterizers.size() != 1) {
    SendNull(port_id);
    return;
  }

  Rasterizer* rasterizer = rasterizers[0].get();
  if (rasterizer == nullptr) {
    SendNull(port_id);
    return;
  }

  flow::LayerTree* layer_tree = rasterizer->GetLastLayerTree();
  if (layer_tree == nullptr) {
    SendNull(port_id);
    return;
  }

  SkPictureRecorder recorder;
  recorder.beginRecording(SkRect::MakeWH(layer_tree->frame_size().width(),
                                         layer_tree->frame_size().height()));

  flow::CompositorContext compositor_context;
  flow::CompositorContext::ScopedFrame frame = compositor_context.AcquireFrame(
      nullptr, *recorder.getRecordingCanvas(), false);
  layer_tree->Raster(frame);

  sk_sp<SkPicture> picture = recorder.finishRecordingAsPicture();

  SkDynamicMemoryWStream stream;
  sky::PngPixelSerializer serializer;
  picture->serialize(&stream, &serializer);
  SkAutoTUnref<SkData> picture_data(stream.copyToData());

  Dart_CObject c_object;
  c_object.type = Dart_CObject_kTypedData;
  c_object.value.as_typed_data.type = Dart_TypedData_kUint8;
  c_object.value.as_typed_data.values =
      const_cast<uint8_t*>(picture_data->bytes());
  c_object.value.as_typed_data.length = picture_data->size();

  Dart_PostCObject(port_id, &c_object);
}

}  // namespace shell
}  // namespace sky
