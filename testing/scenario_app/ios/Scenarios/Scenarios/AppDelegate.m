// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "AppDelegate.h"

#import "ContinuousTexture.h"
#import "FlutterEngine+ScenariosTest.h"
#import "ScreenBeforeFlutter.h"
#import "TextPlatformView.h"

@interface NoStatusBarFlutterViewController : FlutterViewController

@end

@implementation NoStatusBarFlutterViewController
- (BOOL)prefersStatusBarHidden {
  return YES;
}
@end

@implementation AppDelegate

- (BOOL)application:(UIApplication*)application
    didFinishLaunchingWithOptions:(NSDictionary*)launchOptions {
  self.window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
  if ([[[NSProcessInfo processInfo] arguments] containsObject:@"--maskview-blocking"]) {
    self.window.tintColor = UIColor.systemPinkColor;
  }
  NSDictionary<NSString*, NSString*>* launchArgsMap = @{
    // The golden test args should match `GoldenTestManager`.
    @"--locale-initialization" : @"locale_initialization",
    @"--platform-view" : @"platform_view",
    @"--platform-view-no-overlay-intersection" : @"platform_view_no_overlay_intersection",
    @"--platform-view-two-intersecting-overlays" : @"platform_view_two_intersecting_overlays",
    @"--platform-view-partial-intersection" : @"platform_view_partial_intersection",
    @"--platform-view-one-overlay-two-intersecting-overlays" :
        @"platform_view_one_overlay_two_intersecting_overlays",
    @"--platform-view-multiple-without-overlays" : @"platform_view_multiple_without_overlays",
    @"--platform-view-max-overlays" : @"platform_view_max_overlays",
    @"--platform-view-multiple" : @"platform_view_multiple",
    @"--platform-view-multiple-background-foreground" :
        @"platform_view_multiple_background_foreground",
    @"--platform-view-cliprect" : @"platform_view_cliprect",
    @"--platform-view-cliprrect" : @"platform_view_cliprrect",
    @"--platform-view-clippath" : @"platform_view_clippath",
    @"--platform-view-transform" : @"platform_view_transform",
    @"--platform-view-opacity" : @"platform_view_opacity",
    @"--platform-view-rotate" : @"platform_view_rotate",
    @"--gesture-reject-after-touches-ended" : @"platform_view_gesture_reject_after_touches_ended",
    @"--gesture-reject-eager" : @"platform_view_gesture_reject_eager",
    @"--gesture-accept" : @"platform_view_gesture_accept",
    @"--tap-status-bar" : @"tap_status_bar",
    @"--text-semantics-focus" : @"text_semantics_focus",
    @"--animated-color-square" : @"animated_color_square",
    @"--platform-view-with-continuous-texture" : @"platform_view_with_continuous_texture",
    @"--bogus-font-text" : @"bogus_font_text",
    @"--spawn-engine-works" : @"spawn_engine_works",
  };
  __block NSString* flutterViewControllerTestName = nil;
  [launchArgsMap
      enumerateKeysAndObjectsUsingBlock:^(NSString* argument, NSString* testName, BOOL* stop) {
        if ([[[NSProcessInfo processInfo] arguments] containsObject:argument]) {
          flutterViewControllerTestName = testName;
          *stop = YES;
        }
      }];
  if (flutterViewControllerTestName) {
    [self setupFlutterViewControllerTest:flutterViewControllerTestName];
  } else if ([[[NSProcessInfo processInfo] arguments] containsObject:@"--screen-before-flutter"]) {
    self.window.rootViewController = [[ScreenBeforeFlutter alloc] initWithEngineRunCompletion:nil];
  } else {
    self.window.rootViewController = [[UIViewController alloc] init];
  }

  [self.window makeKeyAndVisible];
  if ([[[NSProcessInfo processInfo] arguments] containsObject:@"--with-continuous-texture"]) {
    [ContinuousTexture
        registerWithRegistrar:[self registrarForPlugin:@"com.constant.firing.texture"]];
  }
  return [super application:application didFinishLaunchingWithOptions:launchOptions];
}

// TODO(https://github.com/flutter/flutter/issues/72019) Sharing threads doesn't work correctly yet
// so we need to keep the Spawning engine alive.  Once this issue is fixed we can remove s_spawner.
static FlutterEngine* s_spawner;

- (FlutterEngine*)engineForTest:(NSString*)scenarioIdentifier {
  if ([scenarioIdentifier isEqualToString:@"spawn_engine_works"]) {
    if (s_spawner == nil) {
      s_spawner = [[FlutterEngine alloc] initWithName:@"FlutterControllerTest" project:nil];
      [s_spawner run];
    }
    return [s_spawner spawnWithEntrypoint:nil libraryURI:nil];
  } else {
    FlutterEngine* engine = [[FlutterEngine alloc] initWithName:@"FlutterControllerTest"
                                                        project:nil];
    [engine run];
    return engine;
  }
}

- (FlutterViewController*)flutterViewControllerForTest:(NSString*)scenarioIdentifier
                                            withEngine:(FlutterEngine*)engine {
  if ([scenarioIdentifier isEqualToString:@"tap_status_bar"]) {
    return [[FlutterViewController alloc] initWithEngine:engine nibName:nil bundle:nil];
  } else {
    return [[NoStatusBarFlutterViewController alloc] initWithEngine:engine nibName:nil bundle:nil];
  }
}

- (void)setupFlutterViewControllerTest:(NSString*)scenarioIdentifier {
  FlutterEngine* engine = [self engineForTest:scenarioIdentifier];
  FlutterViewController* flutterViewController =
      [self flutterViewControllerForTest:scenarioIdentifier withEngine:engine];

  [engine.binaryMessenger
      setMessageHandlerOnChannel:@"waiting_for_status"
            binaryMessageHandler:^(NSData* _Nullable message, FlutterBinaryReply _Nonnull reply) {
              FlutterMethodChannel* channel = [FlutterMethodChannel
                  methodChannelWithName:@"driver"
                        binaryMessenger:engine.binaryMessenger
                                  codec:[FlutterJSONMethodCodec sharedInstance]];
              [channel invokeMethod:@"set_scenario" arguments:@{@"name" : scenarioIdentifier}];
            }];
  // Can be used to synchronize timing in the test for a signal from Dart.
  [engine.binaryMessenger
      setMessageHandlerOnChannel:@"display_data"
            binaryMessageHandler:^(NSData* _Nullable message, FlutterBinaryReply _Nonnull reply) {
              NSDictionary* dict = [NSJSONSerialization JSONObjectWithData:message
                                                                   options:0
                                                                     error:nil];
              UITextField* text = [[UITextField alloc] initWithFrame:CGRectMake(0, 400, 300, 100)];
              text.text = dict[@"data"];
              [flutterViewController.view addSubview:text];
            }];

  TextPlatformViewFactory* textPlatformViewFactory =
      [[TextPlatformViewFactory alloc] initWithMessenger:engine.binaryMessenger];
  NSObject<FlutterPluginRegistrar>* registrar =
      [engine registrarForPlugin:@"scenarios/TextPlatformViewPlugin"];
  [registrar registerViewFactory:textPlatformViewFactory
                                withId:@"scenarios/textPlatformView"
      gestureRecognizersBlockingPolicy:FlutterPlatformViewGestureRecognizersBlockingPolicyEager];
  [registrar registerViewFactory:textPlatformViewFactory
                                withId:@"scenarios/textPlatformView_blockPolicyUntilTouchesEnded"
      gestureRecognizersBlockingPolicy:
          FlutterPlatformViewGestureRecognizersBlockingPolicyWaitUntilTouchesEnded];
  self.window.rootViewController = flutterViewController;

  if ([[[NSProcessInfo processInfo] arguments] containsObject:@"--assert-ca-layer-type"]) {
    if ([[[NSProcessInfo processInfo] arguments] containsObject:@"--enable-software-rendering"]) {
      NSAssert([flutterViewController.view.layer isKindOfClass:[CALayer class]],
               @"Expected CALayer for software rendering.");
    } else {
      NSAssert([flutterViewController.view.layer isKindOfClass:[CAMetalLayer class]],
               @"Expected CAMetalLayer for non-software rendering.");
    }
  }
}

@end
