// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package dev.flutter.scenarios;

import android.content.Context;
import androidx.annotation.NonNull;
import io.flutter.embedding.engine.FlutterEngine;
import io.flutter.embedding.engine.FlutterEngineGroup;

public class SpawnedEngineActivity extends TestActivity {
  static final String TAG = "Scenarios";

  @Override
  public FlutterEngine provideFlutterEngine(@NonNull Context context) {
    FlutterEngineGroup engineGroup = new FlutterEngineGroup(context);
    engineGroup.createAndRunDefaultEngine(context);

    FlutterEngine secondEngine = engineGroup.createAndRunDefaultEngine(context);

    secondEngine
        .getDartExecutor()
        .setMessageHandler("take_screenshot", (byteBuffer, binaryReply) -> notifyFlutterRendered());

    return secondEngine;
  }
}
