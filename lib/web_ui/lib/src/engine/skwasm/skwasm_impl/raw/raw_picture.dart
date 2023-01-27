@DefaultAsset('skwasm')
library skwasm_impl;

import 'dart:ffi';

import 'package:ui/src/engine/skwasm/skwasm_impl.dart';

class RawPictureRecorder extends Opaque {}
typedef PictureRecorderHandle = Pointer<RawPictureRecorder>;

class RawPicture extends Opaque {}
typedef PictureHandle = Pointer<RawPicture>;

@Native<PictureRecorderHandle Function()>(
  symbol: 'pictureRecorder_create',
  isLeaf: true)
external PictureRecorderHandle pictureRecorderCreate();

@Native<Void Function(PictureRecorderHandle)>(
  symbol: 'pictureRecorder_destroy',
  isLeaf: true)
external void pictureRecorderDestroy(PictureRecorderHandle picture);

@Native<CanvasHandle Function(PictureRecorderHandle, RawRect)>(
  symbol: 'pictureRecorder_beginRecording',
  isLeaf: true)
external CanvasHandle pictureRecorderBeginRecording(
    PictureRecorderHandle picture, RawRect cullRect);

@Native<PictureHandle Function(PictureRecorderHandle)>(
  symbol: 'pictureRecorder_endRecording',
  isLeaf: true)
external PictureHandle pictureRecorderEndRecording(PictureRecorderHandle picture);

@Native<Void Function(PictureHandle)>(
  symbol: 'pictureRecorder_dispose',
  isLeaf: true)
external void pictureDispose(PictureHandle handle);

@Native<Uint32 Function(PictureHandle)>(
  symbol: 'pictureRecorder_approximateBytesUsed',
  isLeaf: true)
external int pictureApproximateBytesUsed(PictureHandle handle);
