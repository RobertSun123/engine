// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package io.flutter.embedding.engine;

import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.SurfaceTexture;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.Surface;

import java.nio.ByteBuffer;
import java.util.Set;
import java.util.concurrent.CopyOnWriteArraySet;

import io.flutter.embedding.engine.dart.PlatformMessageHandler;
import io.flutter.embedding.engine.FlutterEngine.EngineLifecycleListener;
import io.flutter.embedding.engine.renderer.FlutterRenderer;
import io.flutter.embedding.engine.renderer.OnFirstFrameRenderedListener;

/**
 * Interface between Flutter embedding's Java code and Flutter engine's C/C++ code.
 *
 * WARNING: THIS CLASS IS EXPERIMENTAL. DO NOT SHIP A DEPENDENCY ON THIS CODE.
 * IF YOU USE IT, WE WILL BREAK YOU.
 *
 * Flutter's engine is built with C/C++. The Android Flutter embedding is responsible for
 * coordinating Android OS events and app user interactions with the C/C++ engine. Such coordination
 * requires messaging from an Android app in Java code to the C/C++ engine code. This
 * communication requires a JNI (Java Native Interface) API to cross the Java/native boundary.
 *
 * The entirety of Flutter's JNI API is codified in {@code FlutterJNI}. There are multiple reasons
 * that all such calls are centralized in one class. First, JNI calls are inherently static and
 * contain no Java implementation, therefore there is little reason to associate calls with different
 * classes. Second, every JNI call must be registered in C/C++ code and this registration becomes
 * more complicated with every additional Java class that contains JNI calls. Third, most Android
 * developers are not familiar with native development or JNI intricacies, therefore it is in the
 * interest of future maintenance to reduce the API surface that includes JNI declarations. Thus,
 * all Flutter JNI calls are centralized in {@code FlutterJNI}.
 *
 * Despite the fact that individual JNI calls are inherently static, there is state that exists
 * within {@code FlutterJNI}. Most calls within {@code FlutterJNI} correspond to a specific
 * "platform view", of which there may be many. Therefore, each {@code FlutterJNI} instance holds
 * onto a "native platform view ID" after {@link #attachToNative(boolean)}, which is shared with
 * the native C/C++ engine code. That ID is passed to every platform-view-specific native method.
 * ID management is handled within {@code FlutterJNI} so that developers don't have to hold onto
 * that ID.
 *
 * To connect part of an Android app to Flutter's C/C++ engine, instantiate a {@code FlutterJNI} and
 * then attach it to the native side:
 *
 * {@code
 *     // Instantiate FlutterJNI and attach to the native side.
 *     FlutterJNI flutterJNI = new FlutterJNI();
 *     flutterJNI.attachToNative();
 *
 *     // Use FlutterJNI as desired.
 *     flutterJNI.dispatchPointerDataPacket(...);
 *
 *     // Destroy the connection to the native side and cleanup.
 *     flutterJNI.detachFromNativeAndReleaseResources();
 * }
 *
 * To provide a visual, interactive surface for Flutter rendering and touch events, register a
 * {@link FlutterRenderer.RenderSurface} with {@link #setRenderSurface(FlutterRenderer.RenderSurface)}
 *
 * To receive callbacks for certain events that occur on the native side, register listeners:
 *
 * <ol>
 *   <li>{@link #addEngineLifecycleListener(EngineLifecycleListener)}</li>
 *   <li>{@link #addOnFirstFrameRenderedListener(OnFirstFrameRenderedListener)}</li>
 * </ol>
 *
 * To facilitate platform messages between Java and Dart running in Flutter, register a handler:
 *
 * {@link #setPlatformMessageHandler(PlatformMessageHandler)}
 *
 * To invoke a native method that is not associated with a platform view, invoke it statically:
 *
 * {@code
 *    String uri = FlutterJNI.nativeGetObservatoryUri();
 * }
 */
public class FlutterJNI {
  private static final String TAG = "FlutterJNI";

  public static native boolean nativeGetIsSoftwareRenderingEnabled();

  public static native String nativeGetObservatoryUri();
  
  private Long nativePlatformViewId;
  private FlutterRenderer.RenderSurface renderSurface;
  private PlatformMessageHandler platformMessageHandler;
  private final Set<EngineLifecycleListener> engineLifecycleListeners = new CopyOnWriteArraySet<>();
  private final Set<OnFirstFrameRenderedListener> firstFrameListeners = new CopyOnWriteArraySet<>();

  public void setRenderSurface(@Nullable FlutterRenderer.RenderSurface renderSurface) {
    this.renderSurface = renderSurface;
  }

  // TODO(mattcarroll): define "update"
  // Called by native to update the semantics/accessibility tree.
  @SuppressWarnings("unused")
  private void updateSemantics(ByteBuffer buffer, String[] strings) {
    if (renderSurface != null) {
      renderSurface.updateSemantics(buffer, strings);
    }
  }

  // TODO(mattcarroll): define "update"
  // Called by native to update the custom accessibility actions.
  @SuppressWarnings("unused")
  private void updateCustomAccessibilityActions(ByteBuffer buffer, String[] strings) {
    if (renderSurface != null) {
      renderSurface.updateCustomAccessibilityActions(buffer, strings);
    }
  }

  // Called by native to notify first Flutter frame rendered.
  @SuppressWarnings("unused")
  private void onFirstFrame() {
    if (renderSurface != null) {
      renderSurface.onFirstFrameRendered();
    }

    for (OnFirstFrameRenderedListener listener : firstFrameListeners) {
      listener.onFirstFrameRendered();
    }
  }

  /**
   * Sets the handler for all platform messages that come from the attached platform view to Java.
   *
   * Communication between a specific Flutter context (Dart) and the host platform (Java) is
   * accomplished by passing messages. Messages can be sent from Java to Dart with the corresponding
   * {@code FlutterJNI} methods:
   * <ul>
   *   <li>{@link #dispatchPlatformMessage(String, ByteBuffer, int, int)}</li>
   *   <li>{@link #dispatchEmptyPlatformMessage(String, int)}</li>
   * </ul>
   *
   * {@code FlutterJNI} is also the recipient of all platform messages sent from its attached
   * Flutter context (AKA platform view). {@code FlutterJNI} does not know what to do with these
   * messages, so a handler is exposed to allow these messages to be processed in whatever manner is
   * desired:
   *
   * {@code setPlatformMessageHandler(PlatformMessageHandler)}
   *
   * If a message is received but no {@link PlatformMessageHandler} is registered, that message will
   * be dropped (ignored). Therefore, when using {@code FlutterJNI} to integrate a Flutter context
   * in an app, a {@link PlatformMessageHandler} must be registered for 2-way Java/Dart communication
   * to operate correctly. Moreover, the handler must be implemented such that fundamental platform
   * messages are handled as expected. See {@link FlutterNativeView} for an example implementation.
   */
  public void setPlatformMessageHandler(@Nullable PlatformMessageHandler platformMessageHandler) {
    this.platformMessageHandler = platformMessageHandler;
  }

  // Called by native.
  @SuppressWarnings("unused")
  private void handlePlatformMessage(final String channel, byte[] message, final int replyId) {
    if (platformMessageHandler != null) {
      platformMessageHandler.handlePlatformMessage(channel, message, replyId);
    }
  }

  // Called by native to respond to a platform message that we sent.
  @SuppressWarnings("unused")
  private void handlePlatformMessageResponse(int replyId, byte[] reply) {
    if (platformMessageHandler != null) {
      platformMessageHandler.handlePlatformMessageResponse(replyId, reply);
    }
  }

  public void addEngineLifecycleListener(@NonNull EngineLifecycleListener engineLifecycleListener) {
    engineLifecycleListeners.add(engineLifecycleListener);
  }

  public void removeEngineLifecycleListener(@NonNull EngineLifecycleListener engineLifecycleListener) {
    engineLifecycleListeners.remove(engineLifecycleListener);
  }

  public void addOnFirstFrameRenderedListener(@NonNull OnFirstFrameRenderedListener listener) {
    firstFrameListeners.add(listener);
  }

  public void removeOnFirstFrameRenderedListener(@NonNull OnFirstFrameRenderedListener listener) {
    firstFrameListeners.remove(listener);
  }

  // TODO(mattcarroll): rename comments after refactor is done and their origin no longer matters
  //----- Start from FlutterView -----
  public void onSurfaceCreated(@NonNull Surface surface) {
    ensureAttachedToNative();
    nativeSurfaceCreated(nativePlatformViewId, surface);
  }

  private native void nativeSurfaceCreated(long nativePlatformViewId, Surface surface);

  public void onSurfaceChanged(int width, int height) {
    ensureAttachedToNative();
    nativeSurfaceChanged(nativePlatformViewId, width, height);
  }

  private native void nativeSurfaceChanged(long nativePlatformViewId, int width, int height);

  public void onSurfaceDestroyed() {
    ensureAttachedToNative();
    nativeSurfaceDestroyed(nativePlatformViewId);
  }

  private native void nativeSurfaceDestroyed(long nativePlatformViewId);

  public void setViewportMetrics(float devicePixelRatio,
                                 int physicalWidth,
                                 int physicalHeight,
                                 int physicalPaddingTop,
                                 int physicalPaddingRight,
                                 int physicalPaddingBottom,
                                 int physicalPaddingLeft,
                                 int physicalViewInsetTop,
                                 int physicalViewInsetRight,
                                 int physicalViewInsetBottom,
                                 int physicalViewInsetLeft) {
    ensureAttachedToNative();
    nativeSetViewportMetrics(
        nativePlatformViewId,
        devicePixelRatio,
        physicalWidth,
        physicalHeight,
        physicalPaddingTop,
        physicalPaddingRight,
        physicalPaddingBottom,
        physicalPaddingLeft,
        physicalViewInsetTop,
        physicalViewInsetRight,
        physicalViewInsetBottom,
        physicalViewInsetLeft
    );
  }

  private native void nativeSetViewportMetrics(long nativePlatformViewId,
                                               float devicePixelRatio,
                                               int physicalWidth,
                                               int physicalHeight,
                                               int physicalPaddingTop,
                                               int physicalPaddingRight,
                                               int physicalPaddingBottom,
                                               int physicalPaddingLeft,
                                               int physicalViewInsetTop,
                                               int physicalViewInsetRight,
                                               int physicalViewInsetBottom,
                                               int physicalViewInsetLeft);

  public Bitmap getBitmap() {
    ensureAttachedToNative();
    return nativeGetBitmap(nativePlatformViewId);
  }

  private native Bitmap nativeGetBitmap(long nativePlatformViewId);

  public void dispatchPointerDataPacket(ByteBuffer buffer, int position) {
    ensureAttachedToNative();
    nativeDispatchPointerDataPacket(nativePlatformViewId, buffer, position);
  }

  private native void nativeDispatchPointerDataPacket(long nativePlatformViewId,
                                                      ByteBuffer buffer,
                                                      int position);

  public void dispatchSemanticsAction(int id, int action, ByteBuffer args, int argsPosition) {
    ensureAttachedToNative();
    nativeDispatchSemanticsAction(nativePlatformViewId, id, action, args, argsPosition);
  }

  private native void nativeDispatchSemanticsAction(long nativePlatformViewId,
                                                    int id,
                                                    int action,
                                                    ByteBuffer args,
                                                    int argsPosition);

  public void setSemanticsEnabled(boolean enabled) {
    ensureAttachedToNative();
    nativeSetSemanticsEnabled(nativePlatformViewId, enabled);
  }

  private native void nativeSetSemanticsEnabled(long nativePlatformViewId, boolean enabled);

  public void setAccessibilityFeatures(int flags) {
    ensureAttachedToNative();
    nativeSetAccessibilityFeatures(nativePlatformViewId, flags);
  }

  private native void nativeSetAccessibilityFeatures(long nativePlatformViewId, int flags);

  public void registerTexture(long textureId, SurfaceTexture surfaceTexture) {
    ensureAttachedToNative();
    nativeRegisterTexture(nativePlatformViewId, textureId, surfaceTexture);
  }

  private native void nativeRegisterTexture(long nativePlatformViewId, long textureId, SurfaceTexture surfaceTexture);

  public void markTextureFrameAvailable(long textureId) {
    ensureAttachedToNative();
    nativeMarkTextureFrameAvailable(nativePlatformViewId, textureId);
  }

  private native void nativeMarkTextureFrameAvailable(long nativePlatformViewId, long textureId);

  public void unregisterTexture(long textureId) {
    ensureAttachedToNative();
    nativeUnregisterTexture(nativePlatformViewId, textureId);
  }

  private native void nativeUnregisterTexture(long nativePlatformViewId, long textureId);
  //------- End from FlutterView -----

  // TODO(mattcarroll): rename comments after refactor is done and their origin no longer matters
  //------ Start from FlutterNativeView ----
  public boolean isAttached() {
    return nativePlatformViewId != null;
  }

  public void attachToNative(boolean isBackgroundView) {
    ensureNotAttachedToNative();
    nativePlatformViewId = nativeAttach(this, isBackgroundView);
  }

  private native long nativeAttach(FlutterJNI flutterJNI, boolean isBackgroundView);

  public void detachFromNativeButKeepNativeResources() {
    ensureAttachedToNative();
    nativeDetach(nativePlatformViewId);
    nativePlatformViewId = null;
  }

  private native void nativeDetach(long nativePlatformViewId);

  public void detachFromNativeAndReleaseResources() {
    ensureAttachedToNative();
    nativeDestroy(nativePlatformViewId);
    nativePlatformViewId = null;
  }

  private native void nativeDestroy(long nativePlatformViewId);

  public void runBundleAndSnapshotFromLibrary(@NonNull String pathToBundleWithEntrypoint,
                                              @Nullable String pathToFallbackBundle,
                                              @Nullable String entrypointFunctionName,
                                              @Nullable String pathToEntrypointFunction,
                                              @NonNull AssetManager assetManager) {
    ensureAttachedToNative();
    nativeRunBundleAndSnapshotFromLibrary(
        nativePlatformViewId,
        pathToBundleWithEntrypoint,
        pathToFallbackBundle,
        entrypointFunctionName,
        pathToEntrypointFunction,
        assetManager
    );
  }

  private native void nativeRunBundleAndSnapshotFromLibrary(
      long nativePlatformViewId,
      @NonNull String pathToBundleWithEntrypoint,
      @Nullable String pathToFallbackBundle,
      @Nullable String entrypointFunctionName,
      @Nullable String pathToEntrypointFunction,
      @NonNull AssetManager manager
  );

  public void dispatchEmptyPlatformMessage(String channel, int responseId) {
    ensureAttachedToNative();
    nativeDispatchEmptyPlatformMessage(nativePlatformViewId, channel, responseId);
  }

  // Send an empty platform message to Dart.
  private native void nativeDispatchEmptyPlatformMessage(
      long nativePlatformViewId,
      String channel,
      int responseId
  );

  public void dispatchPlatformMessage(String channel, ByteBuffer message, int position, int responseId) {
    ensureAttachedToNative();
    nativeDispatchPlatformMessage(
        nativePlatformViewId,
        channel,
        message,
        position,
        responseId
    );
  }

  // Send a data-carrying platform message to Dart.
  private native void nativeDispatchPlatformMessage(
      long nativePlatformViewId,
      String channel,
      ByteBuffer message,
      int position,
      int responseId
  );

  public void invokePlatformMessageEmptyResponseCallback(int responseId) {
    ensureAttachedToNative();
    nativeInvokePlatformMessageEmptyResponseCallback(nativePlatformViewId, responseId);
  }

  // Send an empty response to a platform message received from Dart.
  private native void nativeInvokePlatformMessageEmptyResponseCallback(
      long nativePlatformViewId,
      int responseId
  );

  public void invokePlatformMessageResponseCallback(int responseId, ByteBuffer message, int position) {
    ensureAttachedToNative();
    nativeInvokePlatformMessageResponseCallback(
        nativePlatformViewId,
        responseId,
        message,
        position
    );
  }

  // Send a data-carrying response to a platform message received from Dart.
  private native void nativeInvokePlatformMessageResponseCallback(
      long nativePlatformViewId,
      int responseId,
      ByteBuffer message,
      int position
  );
  //------ End from FlutterNativeView ----

  // TODO(mattcarroll): rename comments after refactor is done and their origin no longer matters
  //------ Start from Engine ---
  // Called by native.
  @SuppressWarnings("unused")
  private void onPreEngineRestart() {
    for (EngineLifecycleListener listener : engineLifecycleListeners) {
      listener.onPreEngineRestart();
    }
  }
  //------ End from Engine ---

  private void ensureNotAttachedToNative() {
    if (nativePlatformViewId != null) {
      throw new RuntimeException("Cannot execute operation because FlutterJNI is attached to native.");
    }
  }

  private void ensureAttachedToNative() {
    if (nativePlatformViewId == null) {
      throw new RuntimeException("Cannot execute operation because FlutterJNI is not attached to native.");
    }
  }
}