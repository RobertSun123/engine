// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "GoldenPlatformViewTests.h"

#include <sys/sysctl.h>

#import "PlatformViewGoldenTestManager.h"

static const NSInteger kSecondsToWaitForPlatformView = 30;

@interface GoldenPlatformViewTests ()

@property(nonatomic, copy) NSString* goldenName;

@property(nonatomic, strong) PlatformViewGoldenTestManager* manager;

@end

@implementation GoldenPlatformViewTests

- (instancetype)initWithManager:(PlatformViewGoldenTestManager*)manager
                     invocation:(NSInvocation*)invocation {
  self = [super initWithInvocation:invocation];
  _manager = manager;
  return self;
}

- (void)setUp {
  [super setUp];
  self.continueAfterFailure = NO;

  self.application = [[XCUIApplication alloc] init];
  self.application.launchArguments = @[ self.manager.launchArg ];
  [self.application launch];
}

// Note: don't prefix with "test" or GoldenPlatformViewTests will run instead of the subclasses.
- (void)checkGolden {
  XCUIElement* element = self.application.textViews.firstMatch;
  BOOL exists = [element waitForExistenceWithTimeout:kSecondsToWaitForPlatformView];
  if (!exists) {
    XCTFail(@"It took longer than %@ second to find the platform view."
            @"There might be issues with the platform view's construction,"
            @"or with how the scenario is built.",
            @(kSecondsToWaitForPlatformView));
  }

  GoldenImage* golden = self.manager.goldenImage;

  XCUIScreenshot* screenshot = [[XCUIScreen mainScreen] screenshot];
  if (!golden.image) {
    XCTAttachment* attachment = [XCTAttachment attachmentWithScreenshot:screenshot];
    attachment.name = @"new_golden";
    attachment.lifetime = XCTAttachmentLifetimeKeepAlways;
    [self addAttachment:attachment];
    XCTFail(@"This test will fail - no golden named %@ found. Follow the steps in the "
            @"README to add a new golden.",
            golden.goldenName);
  }

  if (![golden compareGoldenToImage:screenshot.image]) {
    XCTAttachment* screenshotAttachment;
    screenshotAttachment = [XCTAttachment attachmentWithImage:screenshot.image];
    screenshotAttachment.name = golden.goldenName;
    screenshotAttachment.lifetime = XCTAttachmentLifetimeKeepAlways;
    [self addAttachment:screenshotAttachment];

    XCTFail(@"Goldens to not match. Follow the steps in the "
            @"README to update golden named %@ if needed.",
            golden.goldenName);
  }
}
@end
