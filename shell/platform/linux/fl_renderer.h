// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_LINUX_FL_RENDERER_H_
#define FLUTTER_SHELL_PLATFORM_LINUX_FL_RENDERER_H_

#include <EGL/egl.h>

#include <gtk/gtk.h>

#include "flutter/shell/platform/linux/public/flutter_linux/fl_dart_project.h"
#include "flutter/shell/platform/linux/public/flutter_linux/fl_view.h"

G_BEGIN_DECLS

/**
 * FlRendererError:
 * Errors for #FlRenderer objects to set on failures.
 */

typedef enum {
  FL_RENDERER_ERROR_FAILED,
} FlRendererError;

GQuark fl_renderer_error_quark(void) G_GNUC_CONST;

G_DECLARE_DERIVABLE_TYPE(FlRenderer, fl_renderer, FL, RENDERER, GObject)

/**
 * FlRenderer:
 *
 * #FlRenderer is an abstract class that allows Flutter to draw pixels.
 */

struct _FlRendererClass {
  GObjectClass parent_class;

  /**
   * Virtual method called before creating a GdkWindow for the widget.
   * Does not need to be implemented.
   */
  gboolean (*setup_window_attr)(FlRenderer* renderer,
                                EGLDisplay egl_display,
                                EGLConfig egl_config,
                                GdkWindowAttr* window_attributes,
                                gint* mask,
                                GError** error);

  /**
   * Virtual method to create a new EGL display.
   */
  EGLDisplay (*create_display)(FlRenderer* renderer);

  /**
   * Virtual method called when Flutter needs surfaces to render to.
   * @renderer: an #FlRenderer.
   * @display: display to create surfaces on.
   * @visible: (out): the visible surface that is created.
   * @resource: (out): the resource surface that is created.
   * @error: (allow-none): #GError location to store the error occurring, or
   * %NULL to ignore.
   *
   * Returns: %TRUE if both surfaces were created, %FALSE if there was an error.
   */
  gboolean (*create_surfaces)(FlRenderer* renderer,
                              EGLDisplay display,
                              EGLConfig config,
                              EGLSurface* visible,
                              EGLSurface* resource,
                              GError** error);

  /**
   * Virtual method called when the EGL window needs to be resized.
   * Does not need to be implemented.
   */
  void (*set_geometry)(FlRenderer* renderer,
                       GdkRectangle* geometry,
                       gint scale);
};

/**
 * fl_renderer_start:
 * @renderer: an #FlRenderer.
 * @view: the #FlView widget this renderer is using.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL
 * to ignore.
 *
 * Start the renderer.
 *
 * Returns: %TRUE if successfully started.
 */
gboolean fl_renderer_start(FlRenderer* renderer, FlView* view, GError** error);

/**
 * fl_renderer_get_view:
 * @renderer: an #FlRenderer.
 *
 * Returns the #FlView that is using the renderer.
 */
FlView* fl_renderer_get_view(FlRenderer* renderer);

/**
 * fl_renderer_set_geometry:
 * @renderer: an #FlRenderer.
 * @geometry: New size and position (unscaled) of the EGL window.
 * @scale: Scale of the window.
 */
void fl_renderer_set_geometry(FlRenderer* renderer,
                              GdkRectangle* geometry,
                              gint scale);

/**
 * fl_renderer_get_proc_address:
 * @renderer: an #FlRenderer.
 * @name: a function name.
 *
 * Gets the rendering API function that matches the given name.
 *
 * Returns: a function pointer.
 */
void* fl_renderer_get_proc_address(FlRenderer* renderer, const char* name);

/**
 * fl_renderer_make_current:
 * @renderer: an #FlRenderer.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL
 * to ignore.
 *
 * Makes the rendering context current.
 *
 * Returns %TRUE if successful.
 */
gboolean fl_renderer_make_current(FlRenderer* renderer, GError** error);

/**
 * fl_renderer_make_resource_current:
 * @renderer: an #FlRenderer.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL
 * to ignore.
 *
 * Makes the resource rendering context current.
 *
 * Returns %TRUE if successful.
 */
gboolean fl_renderer_make_resource_current(FlRenderer* renderer,
                                           GError** error);

/**
 * fl_renderer_clear_current:
 * @renderer: an #FlRenderer.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL
 * to ignore.
 *
 * Clears the current rendering context.
 *
 * Returns %TRUE if successful.
 */
gboolean fl_renderer_clear_current(FlRenderer* renderer, GError** error);

/**
 * fl_renderer_get_fbo:
 * @renderer: an #FlRenderer.
 *
 * Gets the frame buffer object to render to.
 *
 * Returns: a frame buffer object index.
 */
guint32 fl_renderer_get_fbo(FlRenderer* renderer);

/**
 * fl_renderer_present:
 * @renderer: an #FlRenderer.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL
 * to ignore.
 *
 * Presents the current frame.
 *
 * Returns %TRUE if successful.
 */
gboolean fl_renderer_present(FlRenderer* renderer, GError** error);

G_END_DECLS

#endif  // FLUTTER_SHELL_PLATFORM_LINUX_FL_RENDERER_H_
