// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package io.flutter.embedding.engine.dart;

import android.os.Handler;
import android.os.Looper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.UiThread;
import io.flutter.Log;
import io.flutter.embedding.engine.FlutterJNI;
import io.flutter.plugin.common.BinaryMessenger;
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * Message conduit for 2-way communication between Android and Dart.
 *
 * <p>See {@link BinaryMessenger}, which sends messages from Android to Dart
 *
 * <p>See {@link PlatformMessageHandler}, which handles messages to Android from Dart
 */
class DartMessenger implements BinaryMessenger, PlatformMessageHandler {
  private static final String TAG = "DartMessenger";

  @NonNull private final FlutterJNI flutterJNI;

  @NonNull private final ConcurrentHashMap<String, HandlerInfo> messageHandlers;

  @NonNull private final Map<Integer, BinaryMessenger.BinaryReply> pendingReplies;
  private int nextReplyId = 1;

  @NonNull private final TaskQueue platformTaskQueue = new PlatformTaskQueue();

  DartMessenger(@NonNull FlutterJNI flutterJNI) {
    this.flutterJNI = flutterJNI;
    this.messageHandlers = new ConcurrentHashMap<>();
    this.pendingReplies = new HashMap<>();
  }

  private static class HandlerInfo {
    @NonNull public final BinaryMessenger.BinaryMessageHandler handler;
    @Nullable public final TaskQueue taskQueue;

    HandlerInfo(
        @NonNull BinaryMessenger.BinaryMessageHandler handler, @Nullable TaskQueue taskQueue) {
      this.handler = handler;
      this.taskQueue = taskQueue;
    }
  }

  private static class DefaultTaskQueue implements TaskQueue {
    @NonNull private final ExecutorService executor;

    DefaultTaskQueue() {
      // TODO(gaaclarke): Use a shared thread pool with serial queues instead of
      // making a thread for each TaskQueue.
      this.executor = Executors.newSingleThreadExecutor();
    }

    @Override
    public void dispatch(@NonNull Runnable runnable) {
      executor.submit(runnable);
    }
  }

  private static class PlatformTaskQueue implements TaskQueue {
    @NonNull private final Handler handler = new Handler(Looper.getMainLooper());

    @Override
    public void dispatch(@NonNull Runnable runnable) {
      handler.post(runnable);
    }
  }

  @Override
  public TaskQueue makeBackgroundTaskQueue() {
    return new DefaultTaskQueue();
  }

  @Override
  public void setMessageHandler(
      @NonNull String channel,
      @Nullable BinaryMessenger.BinaryMessageHandler handler,
      @Nullable TaskQueue taskQueue) {
    if (handler == null) {
      Log.v(TAG, "Removing handler for channel '" + channel + "'");
      messageHandlers.remove(channel);
    } else {
      Log.v(TAG, "Setting handler for channel '" + channel + "'");
      messageHandlers.put(channel, new HandlerInfo(handler, taskQueue));
    }
  }

  @Override
  @UiThread
  public void send(@NonNull String channel, @NonNull ByteBuffer message) {
    Log.v(TAG, "Sending message over channel '" + channel + "'");
    send(channel, message, null);
  }

  @Override
  public void send(
      @NonNull String channel,
      @Nullable ByteBuffer message,
      @Nullable BinaryMessenger.BinaryReply callback) {
    Log.v(TAG, "Sending message with callback over channel '" + channel + "'");
    int replyId = nextReplyId++;
    if (callback != null) {
      pendingReplies.put(replyId, callback);
    }
    if (message == null) {
      flutterJNI.dispatchEmptyPlatformMessage(channel, replyId);
    } else {
      flutterJNI.dispatchPlatformMessage(channel, message, message.position(), replyId);
    }
  }

  private void invokeHandler(
      @Nullable HandlerInfo handlerInfo, @Nullable ByteBuffer message, final int replyId) {
    // Called from any thread.
    if (handlerInfo != null) {
      try {
        Log.v(TAG, "Deferring to registered handler to process message.");
        handlerInfo.handler.onMessage(message, new Reply(flutterJNI, replyId));
      } catch (Exception ex) {
        Log.e(TAG, "Uncaught exception in binary message listener", ex);
        flutterJNI.invokePlatformMessageEmptyResponseCallback(replyId);
      } catch (Error err) {
        handleError(err);
      }
    } else {
      Log.v(TAG, "No registered handler for message. Responding to Dart with empty reply message.");
      flutterJNI.invokePlatformMessageEmptyResponseCallback(replyId);
    }
  }

  @Override
  public void handleMessageFromDart(
      @NonNull final String channel,
      @Nullable ByteBuffer message,
      final int replyId,
      long messageData) {
    // Called from the ui thread.
    Log.v(TAG, "Received message from Dart over channel '" + channel + "'");
    @Nullable HandlerInfo handlerInfo = messageHandlers.get(channel);
    @Nullable TaskQueue taskQueue = null;
    if (handlerInfo != null) {
      taskQueue = handlerInfo.taskQueue;
    }
    Runnable myRunnable =
        () -> {
          try {
            invokeHandler(handlerInfo, message, replyId);
            if (message != null && message.isDirect()) {
              // This ensures that if a user retains an instance to the ByteBuffer and it happens to
              // be direct they will get a deterministic error.
              message.limit(0);
            }
          } finally {
            // This is deleting the data underneath the message object.
            flutterJNI.cleanupMessageData(messageData);
          }
        };
    if (taskQueue == null) {
      platformTaskQueue.dispatch(myRunnable);
    } else {
      taskQueue.dispatch(myRunnable);
    }
  }

  @Override
  public void handlePlatformMessageResponse(int replyId, @Nullable ByteBuffer reply) {
    Log.v(TAG, "Received message reply from Dart.");
    BinaryMessenger.BinaryReply callback = pendingReplies.remove(replyId);
    if (callback != null) {
      try {
        Log.v(TAG, "Invoking registered callback for reply from Dart.");
        callback.reply(reply);
        if (reply != null && reply.isDirect()) {
          // This ensures that if a user retains an instance to the ByteBuffer and it happens to
          // be direct they will get a deterministic error.
          reply.limit(0);
        }
      } catch (Exception ex) {
        Log.e(TAG, "Uncaught exception in binary message reply handler", ex);
      } catch (Error err) {
        handleError(err);
      }
    }
  }

  /**
   * Returns the number of pending channel callback replies.
   *
   * <p>When sending messages to the Flutter application using {@link BinaryMessenger#send(String,
   * ByteBuffer, io.flutter.plugin.common.BinaryMessenger.BinaryReply)}, developers can optionally
   * specify a reply callback if they expect a reply from the Flutter application.
   *
   * <p>This method tracks all the pending callbacks that are waiting for response, and is supposed
   * to be called from the main thread (as other methods). Calling from a different thread could
   * possibly capture an indeterministic internal state, so don't do it.
   */
  @UiThread
  public int getPendingChannelResponseCount() {
    return pendingReplies.size();
  }

  // Handles `Error` objects which are not supposed to be caught.
  //
  // We forward them to the thread's uncaught exception handler if there is one. If not, they
  // are rethrown.
  private static void handleError(Error err) {
    Thread currentThread = Thread.currentThread();
    if (currentThread.getUncaughtExceptionHandler() == null) {
      throw err;
    }
    currentThread.getUncaughtExceptionHandler().uncaughtException(currentThread, err);
  }

  static class Reply implements BinaryMessenger.BinaryReply {
    @NonNull private final FlutterJNI flutterJNI;
    private final int replyId;
    private final AtomicBoolean done = new AtomicBoolean(false);

    Reply(@NonNull FlutterJNI flutterJNI, int replyId) {
      this.flutterJNI = flutterJNI;
      this.replyId = replyId;
    }

    @Override
    public void reply(@Nullable ByteBuffer reply) {
      if (done.getAndSet(true)) {
        throw new IllegalStateException("Reply already submitted");
      }
      if (reply == null) {
        flutterJNI.invokePlatformMessageEmptyResponseCallback(replyId);
      } else {
        flutterJNI.invokePlatformMessageResponseCallback(replyId, reply, reply.position());
      }
    }
  }
}
