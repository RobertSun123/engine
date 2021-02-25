// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

typedef void (^FlutterKeyHandlerCallback)(BOOL handled);

@protocol FlutterKeyHandlerBase

- (void)handleEvent:(NSEvent*)event
             ofType:(NSString*)type
           callback:(FlutterKeyHandlerCallback)callback;

@end
