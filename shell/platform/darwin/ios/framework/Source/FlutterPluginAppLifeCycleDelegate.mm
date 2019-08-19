// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/darwin/ios/framework/Headers/FlutterPluginAppLifeCycleDelegate.h"
#include "flutter/fml/logging.h"
#include "flutter/fml/paths.h"
#include "flutter/lib/ui/plugins/callback_cache.h"
#include "flutter/shell/platform/darwin/ios/framework/Headers/FlutterViewController.h"
#include "flutter/shell/platform/darwin/ios/framework/Source/FlutterCallbackCache_Internal.h"

static const char* kCallbackCacheSubDir = "Library/Caches/";

static const SEL selectorsHandledByPlugins[] = {
    @selector(application:didReceiveRemoteNotification:fetchCompletionHandler:),
    @selector(application:performFetchWithCompletionHandler:)};

@interface FlutterPluginAppLifeCycleDelegate ()
- (void)handleDidEnterBackground:(NSNotification*)notification;
- (void)handleWillEnterForeground:(NSNotification*)notification;
- (void)handleWillResignActive:(NSNotification*)notification;
- (void)handleDidBecomeActive:(NSNotification*)notification;
- (void)handleWillTerminate:(NSNotification*)notification;
@end

@implementation FlutterPluginAppLifeCycleDelegate {
  NSMutableArray* _notificationUnsubscribers;
  UIBackgroundTaskIdentifier _debugBackgroundTask;

  // Weak references to registered plugins.
  NSPointerArray* _delegates;
}

- (void)addObserverFor:(NSString*)name selector:(SEL)selector {
  [[NSNotificationCenter defaultCenter] addObserver:self selector:selector name:name object:nil];
  __block NSObject* blockSelf = self;
  dispatch_block_t unsubscribe = ^{
    [[NSNotificationCenter defaultCenter] removeObserver:blockSelf name:name object:nil];
  };
  [_notificationUnsubscribers addObject:[unsubscribe copy]];
}

- (instancetype)init {
  if (self = [super init]) {
    _notificationUnsubscribers = [[NSMutableArray alloc] init];
    std::string cachePath = fml::paths::JoinPaths({getenv("HOME"), kCallbackCacheSubDir});
    [FlutterCallbackCache setCachePath:[NSString stringWithUTF8String:cachePath.c_str()]];
    [self addObserverFor:UIApplicationDidEnterBackgroundNotification
                selector:@selector(handleDidEnterBackground:)];
    [self addObserverFor:UIApplicationWillEnterForegroundNotification
                selector:@selector(handleWillEnterForeground:)];
    [self addObserverFor:UIApplicationWillResignActiveNotification
                selector:@selector(handleWillResignActive:)];
    [self addObserverFor:UIApplicationDidBecomeActiveNotification
                selector:@selector(handleDidBecomeActive:)];
    [self addObserverFor:UIApplicationWillTerminateNotification
                selector:@selector(handleWillTerminate:)];
    _delegates = [NSPointerArray weakObjectsPointerArray];
  }
  return self;
}

- (void)dealloc {
  for (dispatch_block_t unsubscribe in _notificationUnsubscribers) {
    unsubscribe();
  }
}

static BOOL isPowerOfTwo(NSUInteger x) {
  return x != 0 && (x & (x - 1)) == 0;
}

- (BOOL)isSelectorAddedDynamically:(SEL)selector {
  for (const SEL& aSelector : selectorsHandledByPlugins) {
    if (selector == aSelector) {
      return YES;
    }
  }
  return NO;
}

- (BOOL)hasPluginThatRespondsToSelector:(SEL)selector {
  for (NSObject<FlutterApplicationLifeCycleDelegate>* delegate in [_delegates allObjects]) {
    if (!delegate) {
      continue;
    }
    if ([delegate respondsToSelector:selector]) {
      return YES;
    }
  }
  return NO;
}

- (void)addDelegate:(NSObject<FlutterApplicationLifeCycleDelegate>*)delegate {
  [_delegates addPointer:(__bridge void*)delegate];
  if (isPowerOfTwo([_delegates count])) {
    [_delegates compact];
  }
}

- (BOOL)application:(UIApplication*)application
    didFinishLaunchingWithOptions:(NSDictionary*)launchOptions {
  for (NSObject<FlutterApplicationLifeCycleDelegate>* delegate in [_delegates allObjects]) {
    if (!delegate) {
      continue;
    }
    if ([delegate respondsToSelector:_cmd]) {
      if (![delegate application:application didFinishLaunchingWithOptions:launchOptions]) {
        return NO;
      }
    }
  }
  return YES;
}

- (BOOL)application:(UIApplication*)application
    willFinishLaunchingWithOptions:(NSDictionary*)launchOptions {
  flutter::DartCallbackCache::LoadCacheFromDisk();
  for (NSObject<FlutterApplicationLifeCycleDelegate>* delegate in [_delegates allObjects]) {
    if (!delegate) {
      continue;
    }
    if ([delegate respondsToSelector:_cmd]) {
      if (![delegate application:application willFinishLaunchingWithOptions:launchOptions]) {
        return NO;
      }
    }
  }
  return YES;
}

- (void)handleDidEnterBackground:(NSNotification*)notification {
  UIApplication* application = [UIApplication sharedApplication];
#if FLUTTER_RUNTIME_MODE == FLUTTER_RUNTIME_MODE_DEBUG
  // The following keeps the Flutter session alive when the device screen locks
  // in debug mode. It allows continued use of features like hot reload and
  // taking screenshots once the device unlocks again.
  //
  // Note the name is not an identifier and multiple instances can exist.
  _debugBackgroundTask = [application
      beginBackgroundTaskWithName:@"Flutter debug task"
                expirationHandler:^{
                  [application endBackgroundTask:_debugBackgroundTask];
                  FML_LOG(WARNING)
                      << "\nThe OS has terminated the Flutter debug connection for being "
                         "inactive in the background for too long.\n\n"
                         "There are no errors with your Flutter application.\n\n"
                         "To reconnect, launch your application again via 'flutter run'";
                }];
#endif  // FLUTTER_RUNTIME_MODE == FLUTTER_RUNTIME_MODE_DEBUG
  for (NSObject<FlutterApplicationLifeCycleDelegate>* delegate in _delegates) {
    if (!delegate) {
      continue;
    }
    if ([delegate respondsToSelector:@selector(applicationDidEnterBackground:)]) {
      [delegate applicationDidEnterBackground:application];
    }
  }
}

- (void)handleWillEnterForeground:(NSNotification*)notification {
  UIApplication* application = [UIApplication sharedApplication];
#if FLUTTER_RUNTIME_MODE == FLUTTER_RUNTIME_MODE_DEBUG
  [application endBackgroundTask:_debugBackgroundTask];
#endif  // FLUTTER_RUNTIME_MODE == FLUTTER_RUNTIME_MODE_DEBUG
  for (NSObject<FlutterApplicationLifeCycleDelegate>* delegate in _delegates) {
    if (!delegate) {
      continue;
    }
    if ([delegate respondsToSelector:@selector(applicationWillEnterForeground:)]) {
      [delegate applicationWillEnterForeground:application];
    }
  }
}

- (void)handleWillResignActive:(NSNotification*)notification {
  UIApplication* application = [UIApplication sharedApplication];
  for (NSObject<FlutterApplicationLifeCycleDelegate>* delegate in _delegates) {
    if (!delegate) {
      continue;
    }
    if ([delegate respondsToSelector:@selector(applicationWillResignActive:)]) {
      [delegate applicationWillResignActive:application];
    }
  }
}

- (void)handleDidBecomeActive:(NSNotification*)notification {
  UIApplication* application = [UIApplication sharedApplication];
  for (NSObject<FlutterApplicationLifeCycleDelegate>* delegate in _delegates) {
    if (!delegate) {
      continue;
    }
    if ([delegate respondsToSelector:@selector(applicationDidBecomeActive:)]) {
      [delegate applicationDidBecomeActive:application];
    }
  }
}

- (void)handleWillTerminate:(NSNotification*)notification {
  UIApplication* application = [UIApplication sharedApplication];
  for (NSObject<FlutterApplicationLifeCycleDelegate>* delegate in _delegates) {
    if (!delegate) {
      continue;
    }
    if ([delegate respondsToSelector:@selector(applicationWillTerminate:)]) {
      [delegate applicationWillTerminate:application];
    }
  }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
- (void)application:(UIApplication*)application
    didRegisterUserNotificationSettings:(UIUserNotificationSettings*)notificationSettings {
  for (NSObject<FlutterApplicationLifeCycleDelegate>* delegate in _delegates) {
    if (!delegate) {
      continue;
    }
    if ([delegate respondsToSelector:_cmd]) {
      [delegate application:application didRegisterUserNotificationSettings:notificationSettings];
    }
  }
}
#pragma GCC diagnostic pop

- (void)application:(UIApplication*)application
    didRegisterForRemoteNotificationsWithDeviceToken:(NSData*)deviceToken {
  for (NSObject<FlutterApplicationLifeCycleDelegate>* delegate in _delegates) {
    if (!delegate) {
      continue;
    }
    if ([delegate respondsToSelector:_cmd]) {
      [delegate application:application
          didRegisterForRemoteNotificationsWithDeviceToken:deviceToken];
    }
  }
}

- (void)application:(UIApplication*)application
    didReceiveRemoteNotification:(NSDictionary*)userInfo
          fetchCompletionHandler:(void (^)(UIBackgroundFetchResult result))completionHandler {
  for (NSObject<FlutterApplicationLifeCycleDelegate>* delegate in _delegates) {
    if (!delegate) {
      continue;
    }
    if ([delegate respondsToSelector:_cmd]) {
      if ([delegate application:application
              didReceiveRemoteNotification:userInfo
                    fetchCompletionHandler:completionHandler]) {
        return;
      }
    }
  }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
- (void)application:(UIApplication*)application
    didReceiveLocalNotification:(UILocalNotification*)notification {
  for (NSObject<FlutterApplicationLifeCycleDelegate>* delegate in _delegates) {
    if (!delegate) {
      continue;
    }
    if ([delegate respondsToSelector:_cmd]) {
      [delegate application:application didReceiveLocalNotification:notification];
    }
  }
}
#pragma GCC diagnostic pop

- (void)userNotificationCenter:(UNUserNotificationCenter*)center
       willPresentNotification:(UNNotification*)notification
         withCompletionHandler:
             (void (^)(UNNotificationPresentationOptions options))completionHandler {
  if (@available(iOS 10.0, *)) {
    for (NSObject<FlutterApplicationLifeCycleDelegate>* delegate in _delegates) {
      if (!delegate) {
        continue;
      }
      if ([delegate respondsToSelector:_cmd]) {
        [delegate userNotificationCenter:center
                 willPresentNotification:notification
                   withCompletionHandler:completionHandler];
      }
    }
  }
}

- (BOOL)application:(UIApplication*)application
            openURL:(NSURL*)url
            options:(NSDictionary<UIApplicationOpenURLOptionsKey, id>*)options {
  for (NSObject<FlutterApplicationLifeCycleDelegate>* delegate in _delegates) {
    if (!delegate) {
      continue;
    }
    if ([delegate respondsToSelector:_cmd]) {
      if ([delegate application:application openURL:url options:options]) {
        return YES;
      }
    }
  }
  return NO;
}

- (BOOL)application:(UIApplication*)application handleOpenURL:(NSURL*)url {
  for (NSObject<FlutterApplicationLifeCycleDelegate>* delegate in _delegates) {
    if (!delegate) {
      continue;
    }
    if ([delegate respondsToSelector:_cmd]) {
      if ([delegate application:application handleOpenURL:url]) {
        return YES;
      }
    }
  }
  return NO;
}

- (BOOL)application:(UIApplication*)application
              openURL:(NSURL*)url
    sourceApplication:(NSString*)sourceApplication
           annotation:(id)annotation {
  for (NSObject<FlutterApplicationLifeCycleDelegate>* delegate in _delegates) {
    if (!delegate) {
      continue;
    }
    if ([delegate respondsToSelector:_cmd]) {
      if ([delegate application:application
                        openURL:url
              sourceApplication:sourceApplication
                     annotation:annotation]) {
        return YES;
      }
    }
  }
  return NO;
}

- (void)application:(UIApplication*)application
    performActionForShortcutItem:(UIApplicationShortcutItem*)shortcutItem
               completionHandler:(void (^)(BOOL succeeded))completionHandler NS_AVAILABLE_IOS(9_0) {
  for (NSObject<FlutterApplicationLifeCycleDelegate>* delegate in _delegates) {
    if (!delegate) {
      continue;
    }
    if ([delegate respondsToSelector:_cmd]) {
      if ([delegate application:application
              performActionForShortcutItem:shortcutItem
                         completionHandler:completionHandler]) {
        return;
      }
    }
  }
}

- (BOOL)application:(UIApplication*)application
    handleEventsForBackgroundURLSession:(nonnull NSString*)identifier
                      completionHandler:(nonnull void (^)())completionHandler {
  for (NSObject<FlutterApplicationLifeCycleDelegate>* delegate in _delegates) {
    if (!delegate) {
      continue;
    }
    if ([delegate respondsToSelector:_cmd]) {
      if ([delegate application:application
              handleEventsForBackgroundURLSession:identifier
                                completionHandler:completionHandler]) {
        return YES;
      }
    }
  }
  return NO;
}

- (BOOL)application:(UIApplication*)application
    performFetchWithCompletionHandler:(void (^)(UIBackgroundFetchResult result))completionHandler {
  for (NSObject<FlutterApplicationLifeCycleDelegate>* delegate in _delegates) {
    if (!delegate) {
      continue;
    }
    if ([delegate respondsToSelector:_cmd]) {
      if ([delegate application:application performFetchWithCompletionHandler:completionHandler]) {
        return YES;
      }
    }
  }
  return NO;
}

- (BOOL)application:(UIApplication*)application
    continueUserActivity:(NSUserActivity*)userActivity
      restorationHandler:(void (^)(NSArray*))restorationHandler {
  for (NSObject<FlutterApplicationLifeCycleDelegate>* delegate in _delegates) {
    if (!delegate) {
      continue;
    }
    if ([delegate respondsToSelector:_cmd]) {
      if ([delegate application:application
              continueUserActivity:userActivity
                restorationHandler:restorationHandler]) {
        return YES;
      }
    }
  }
  return NO;
}
@end
