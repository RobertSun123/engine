// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_LINUX_FL_ACCESSIBLE_APPLICATION_H_
#define FLUTTER_SHELL_PLATFORM_LINUX_FL_ACCESSIBLE_APPLICATION_H_

#include <atk/atk.h>

#include "flutter/shell/platform/linux/fl_atk_fixes.h"

G_BEGIN_DECLS

#define FL_TYPE_ACCESSIBLE_APPLICATION fl_accessible_application_get_type()
G_DECLARE_FINAL_TYPE(FlAccessibleApplication,
                     fl_accessible_application,
                     FL,
                     ACCESSIBLE_APPLICATION,
                     AtkObject);

/**
 * FlAccessibleApplication:
 *
 * #FlAccessibleApplication is an root accessibility node for a Flutter
 * application.
 */

/**
 * fl_accessible_application_new:
 *
 * Creates a the root accessibility node for a Flutter application.
 *
 * Returns: a new #FlAccessibleApplication.
 */
FlAccessibleApplication* fl_accessible_application_new();

G_END_DECLS

#endif  // FLUTTER_SHELL_PLATFORM_LINUX_FL_ACCESSIBLE_APPLICATION_H_
