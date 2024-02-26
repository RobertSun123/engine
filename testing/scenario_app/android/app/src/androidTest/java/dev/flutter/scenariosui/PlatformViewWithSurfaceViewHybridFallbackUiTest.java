// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package dev.flutter.scenariosui;

import android.content.Intent;
import androidx.annotation.NonNull;
import androidx.test.filters.LargeTest;
import androidx.test.rule.ActivityTestRule;
import androidx.test.runner.AndroidJUnit4;
import dev.flutter.scenarios.PlatformViewsActivity;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
@LargeTest
public class PlatformViewWithSurfaceViewHybridFallbackUiTest {
  Intent intent;
  @Rule @NonNull public ArgumentAwareIntent intentRule = new ArgumentAwareIntent();

  @Rule @NonNull
  public ActivityTestRule<PlatformViewsActivity> activityRule =
      new ActivityTestRule<>(
          PlatformViewsActivity.class, /*initialTouchMode=*/ false, /*launchActivity=*/ false);

  private static String goldName(String suffix) {
    return "PlatformViewWithSurfaceViewHybridFallbackUiTest_" + suffix;
  }

  @Before
  public void setUp() {
    intent = intentRule.getIntent();
    // Request TLHC with fallback to HC.
    intent.putExtra("use_android_view", false);
    intent.putExtra("expect_android_view_fallback", true);
    // Use a SurfaceView to trigger fallback.
    intent.putExtra("view_type", PlatformViewsActivity.SURFACE_VIEW_PV);
  }

  @Test
  public void testPlatformView() throws Exception {
    intent.putExtra("scenario_name", "platform_view");
    ScreenshotUtil.capture(activityRule.launchActivity(intent), goldName("testPlatformView"));
  }
}
