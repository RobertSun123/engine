// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/linux/public/flutter_linux/fl_view.h"

#include <gdk/gdkwayland.h>
#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif
#include <cstring>

#include "flutter/shell/platform/linux/fl_engine_private.h"
#include "flutter/shell/platform/linux/fl_key_event_plugin.h"
#include "flutter/shell/platform/linux/fl_keyboard_manager.h"
#include "flutter/shell/platform/linux/fl_mouse_cursor_plugin.h"
#include "flutter/shell/platform/linux/fl_platform_plugin.h"
#include "flutter/shell/platform/linux/fl_plugin_registrar_private.h"
#include "flutter/shell/platform/linux/fl_renderer_wayland.h"
#include "flutter/shell/platform/linux/fl_renderer_x11.h"
#include "flutter/shell/platform/linux/fl_text_input_plugin.h"
#include "flutter/shell/platform/linux/public/flutter_linux/fl_engine.h"
#include "flutter/shell/platform/linux/public/flutter_linux/fl_plugin_registry.h"

static constexpr int kMicrosecondsPerMillisecond = 1000;

struct _FlView {
  GtkWidget parent_instance;

  // Project being run.
  FlDartProject* project;

  // Rendering output.
  FlRenderer* renderer;

  // Engine running @project.
  FlEngine* engine;

  // Pointer button state recorded for sending status updates.
  int64_t button_state;

  FlKeyboardManager* keyboard_manager;

  // Flutter system channel handlers.
  FlKeyEventPlugin* key_event_plugin;
  FlMouseCursorPlugin* mouse_cursor_plugin;
  FlPlatformPlugin* platform_plugin;
  FlTextInputPlugin* text_input_plugin;
};

enum { PROP_FLUTTER_PROJECT = 1, PROP_LAST };

static void fl_view_plugin_registry_iface_init(
    FlPluginRegistryInterface* iface);

G_DEFINE_TYPE_WITH_CODE(
    FlView,
    fl_view,
    GTK_TYPE_WIDGET,
    G_IMPLEMENT_INTERFACE(fl_plugin_registry_get_type(),
                          fl_view_plugin_registry_iface_init))

// Converts a GDK button event into a Flutter event and sends it to the engine.
static gboolean fl_view_send_pointer_button_event(FlView* self,
                                                  GdkEventButton* event) {
  int64_t button;
  switch (event->button) {
    case 1:
      button = kFlutterPointerButtonMousePrimary;
      break;
    case 2:
      button = kFlutterPointerButtonMouseMiddle;
      break;
    case 3:
      button = kFlutterPointerButtonMouseSecondary;
      break;
    default:
      return FALSE;
  }
  int old_button_state = self->button_state;
  FlutterPointerPhase phase = kMove;
  if (event->type == GDK_BUTTON_PRESS) {
    // Drop the event if Flutter already thinks the button is down.
    if ((self->button_state & button) != 0) {
      return FALSE;
    }
    self->button_state ^= button;

    phase = old_button_state == 0 ? kDown : kMove;
  } else if (event->type == GDK_BUTTON_RELEASE) {
    // Drop the event if Flutter already thinks the button is up.
    if ((self->button_state & button) == 0) {
      return FALSE;
    }
    self->button_state ^= button;

    phase = self->button_state == 0 ? kUp : kMove;
  }

  if (self->engine == nullptr) {
    return FALSE;
  }

  gint scale_factor = gtk_widget_get_scale_factor(GTK_WIDGET(self));
  fl_engine_send_mouse_pointer_event(
      self->engine, phase, event->time * kMicrosecondsPerMillisecond,
      event->x * scale_factor, event->y * scale_factor, 0, 0,
      self->button_state);

  return TRUE;
}

static FlutterKeyEventKind convertKeyEventKind(FlKeyEventKind kind) {
  switch (kind) {
    case kFlKeyDataKindUp:
      return kFlutterKeyEventKindUp;
    case kFlKeyDataKindDown:
      return kFlutterKeyEventKindDown;
    case kFlKeyDataKindSync:
      return kFlutterKeyEventKindSync;
    case kFlKeyDataKindCancel:
      return kFlutterKeyEventKindCancel;
  }
}

static size_t nullable_strlen(const char* str) {
  return str == nullptr ? 0 : strlen(str);
}

static char *nullable_strcpy(char* dest, const char* src) {
  printf("dest %lx src %lx str %s\n", (int64_t)dest, (int64_t)src, src == nullptr ? "" : src);
  if (src == nullptr)
    return dest;
  return strcpy(dest, src);
}

// Sends a Flutter key event to the engine.
static gboolean fl_view_send_key_event(FlView* self, const GdkEventKey* event) {
  FlKeyDatum physical_data[kMaxConvertedKeyData] = {};
  FlLogicalKeyDatum logical_data[kMaxConvertedLogicalKeyData] = {};
  size_t result_count = fl_keyboard_manager_convert_key_event(
      self->keyboard_manager, event, physical_data, logical_data);
  size_t base_logical_index = 0;
  for (size_t physical_index = 0; physical_index < result_count; physical_index++) {
    FlKeyDatum* physical_datum = physical_data + physical_index;
    FlutterKeyEvent fl_event = {};
    fl_event.struct_size = sizeof(fl_event);
    fl_event.timestamp = physical_datum->timestamp;
    fl_event.active_locks = physical_datum->active_locks;
    fl_event.kind = convertKeyEventKind(physical_datum->kind);
    fl_event.key = physical_datum->key;
    fl_event.repeated = physical_datum->repeated;
    fl_event.active_locks = physical_datum->active_locks;
    fl_event.logical_event_count = physical_datum->logical_data_count;

    size_t character_sizes[fl_event.logical_event_count];
    size_t character_data_size = 0;
    for (size_t logical_index = 0; logical_index < fl_event.logical_event_count; logical_index++) {
      character_sizes[logical_index] = nullable_strlen(logical_data[base_logical_index + logical_index].character);
      character_data_size += character_sizes[logical_index];
    }
    uint8_t logical_characters_data[character_data_size];
    fl_event.logical_characters_data = logical_characters_data;

    FlutterLogicalKeyEvent logical_events[fl_event.logical_event_count];
    fl_event.logical_events = logical_events;

    static_assert(sizeof(uint8_t) == sizeof(char));
    char* base_character_head = reinterpret_cast<char*>(logical_characters_data);
    for (size_t logical_index = 0; logical_index < fl_event.logical_event_count; logical_index++) {
      FlLogicalKeyDatum *logical_datum = logical_data + (base_logical_index + logical_index);
      nullable_strcpy(base_character_head, logical_datum->character);
      logical_events[logical_index].struct_size = sizeof(FlutterLogicalKeyEvent);
      logical_events[logical_index].character_size = character_sizes[logical_index];
      logical_events[logical_index].key = logical_datum->key;
      logical_events[logical_index].kind = convertKeyEventKind(logical_datum->kind);
      logical_events[logical_index].repeated = logical_datum->repeated;
      base_character_head += character_sizes[logical_index];
    }

    fl_engine_send_key_event(self->engine, &fl_event);
    base_logical_index += fl_event.logical_event_count;
  }
  return TRUE;
}

// Updates the engine with the current window metrics.
static void fl_view_geometry_changed(FlView* self) {
  GtkAllocation allocation;
  gtk_widget_get_allocation(GTK_WIDGET(self), &allocation);
  gint scale_factor = gtk_widget_get_scale_factor(GTK_WIDGET(self));
  fl_engine_send_window_metrics_event(
      self->engine, allocation.width * scale_factor,
      allocation.height * scale_factor, scale_factor);
  fl_renderer_set_geometry(self->renderer, &allocation, scale_factor);
}

// Implements FlPluginRegistry::get_registrar_for_plugin.
static FlPluginRegistrar* fl_view_get_registrar_for_plugin(
    FlPluginRegistry* registry,
    const gchar* name) {
  FlView* self = FL_VIEW(registry);

  return fl_plugin_registrar_new(self,
                                 fl_engine_get_binary_messenger(self->engine));
}

static void fl_view_plugin_registry_iface_init(
    FlPluginRegistryInterface* iface) {
  iface->get_registrar_for_plugin = fl_view_get_registrar_for_plugin;
}

static FlRenderer* fl_view_get_renderer_for_display(GdkDisplay* display) {
#ifdef GDK_WINDOWING_X11
  if (GDK_IS_X11_DISPLAY(display)) {
    return FL_RENDERER(fl_renderer_x11_new());
  }
#endif

  if (GDK_IS_WAYLAND_DISPLAY(display)) {
    return FL_RENDERER(fl_renderer_wayland_new());
  }

  g_error("Unsupported GDK backend");

  return nullptr;
}

static void fl_view_constructed(GObject* object) {
  FlView* self = FL_VIEW(object);

  GdkDisplay* display = gtk_widget_get_display(GTK_WIDGET(self));
  self->renderer = fl_view_get_renderer_for_display(display);
  self->engine = fl_engine_new(self->project, self->renderer);

  // Create system channel handlers.
  FlBinaryMessenger* messenger = fl_engine_get_binary_messenger(self->engine);
  self->key_event_plugin = fl_key_event_plugin_new(messenger);
  self->mouse_cursor_plugin = fl_mouse_cursor_plugin_new(messenger, self);
  self->platform_plugin = fl_platform_plugin_new(messenger);
  self->text_input_plugin = fl_text_input_plugin_new(messenger, self);
  self->keyboard_manager = fl_keyboard_manager_new();
}

static void fl_view_set_property(GObject* object,
                                 guint prop_id,
                                 const GValue* value,
                                 GParamSpec* pspec) {
  FlView* self = FL_VIEW(object);

  switch (prop_id) {
    case PROP_FLUTTER_PROJECT:
      g_set_object(&self->project,
                   static_cast<FlDartProject*>(g_value_get_object(value)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
}

static void fl_view_get_property(GObject* object,
                                 guint prop_id,
                                 GValue* value,
                                 GParamSpec* pspec) {
  FlView* self = FL_VIEW(object);

  switch (prop_id) {
    case PROP_FLUTTER_PROJECT:
      g_value_set_object(value, self->project);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
}

static void fl_view_notify(GObject* object, GParamSpec* pspec) {
  FlView* self = FL_VIEW(object);

  if (strcmp(pspec->name, "scale-factor") == 0) {
    fl_view_geometry_changed(self);
  }

  if (G_OBJECT_CLASS(fl_view_parent_class)->notify != nullptr) {
    G_OBJECT_CLASS(fl_view_parent_class)->notify(object, pspec);
  }
}

static void fl_view_dispose(GObject* object) {
  FlView* self = FL_VIEW(object);

  g_clear_object(&self->project);
  g_clear_object(&self->renderer);
  g_clear_object(&self->engine);
  g_clear_object(&self->key_event_plugin);
  g_clear_object(&self->mouse_cursor_plugin);
  g_clear_object(&self->platform_plugin);
  g_clear_object(&self->text_input_plugin);
  g_clear_object(&self->keyboard_manager);

  G_OBJECT_CLASS(fl_view_parent_class)->dispose(object);
}

// Implements GtkWidget::realize.
static void fl_view_realize(GtkWidget* widget) {
  FlView* self = FL_VIEW(widget);
  g_autoptr(GError) error = nullptr;

  gtk_widget_set_realized(widget, TRUE);

  if (!fl_renderer_start(self->renderer, widget, &error)) {
    g_warning("Failed to start Flutter renderer: %s", error->message);
    return;
  }

  if (!fl_engine_start(self->engine, &error)) {
    g_warning("Failed to start Flutter engine: %s", error->message);
    return;
  }
}

// Implements GtkWidget::size-allocate.
static void fl_view_size_allocate(GtkWidget* widget,
                                  GtkAllocation* allocation) {
  FlView* self = FL_VIEW(widget);

  gtk_widget_set_allocation(widget, allocation);

  if (gtk_widget_get_realized(widget) && gtk_widget_get_has_window(widget)) {
    gdk_window_move_resize(gtk_widget_get_window(widget), allocation->x,
                           allocation->y, allocation->width,
                           allocation->height);
  }

  fl_view_geometry_changed(self);
}

// Implements GtkWidget::button_press_event.
static gboolean fl_view_button_press_event(GtkWidget* widget,
                                           GdkEventButton* event) {
  FlView* self = FL_VIEW(widget);

  // Flutter doesn't handle double and triple click events.
  if (event->type == GDK_DOUBLE_BUTTON_PRESS ||
      event->type == GDK_TRIPLE_BUTTON_PRESS) {
    return FALSE;
  }

  return fl_view_send_pointer_button_event(self, event);
}

// Implements GtkWidget::button_release_event.
static gboolean fl_view_button_release_event(GtkWidget* widget,
                                             GdkEventButton* event) {
  FlView* self = FL_VIEW(widget);

  return fl_view_send_pointer_button_event(self, event);
}

// Implements GtkWidget::scroll_event.
static gboolean fl_view_scroll_event(GtkWidget* widget, GdkEventScroll* event) {
  FlView* self = FL_VIEW(widget);

  // TODO(robert-ancell): Update to use GtkEventControllerScroll when we can
  // depend on GTK 3.24.

  gdouble scroll_delta_x = 0.0, scroll_delta_y = 0.0;
  if (event->direction == GDK_SCROLL_SMOOTH) {
    scroll_delta_x = event->delta_x;
    scroll_delta_y = event->delta_y;
  } else if (event->direction == GDK_SCROLL_UP) {
    scroll_delta_y = -1;
  } else if (event->direction == GDK_SCROLL_DOWN) {
    scroll_delta_y = 1;
  } else if (event->direction == GDK_SCROLL_LEFT) {
    scroll_delta_x = -1;
  } else if (event->direction == GDK_SCROLL_RIGHT) {
    scroll_delta_x = 1;
  }

  // TODO(robert-ancell): See if this can be queried from the OS; this value is
  // chosen arbitrarily to get something that feels reasonable.
  const int kScrollOffsetMultiplier = 20;
  scroll_delta_x *= kScrollOffsetMultiplier;
  scroll_delta_y *= kScrollOffsetMultiplier;

  gint scale_factor = gtk_widget_get_scale_factor(GTK_WIDGET(self));
  fl_engine_send_mouse_pointer_event(
      self->engine, self->button_state != 0 ? kMove : kHover,
      event->time * kMicrosecondsPerMillisecond, event->x * scale_factor,
      event->y * scale_factor, scroll_delta_x, scroll_delta_y,
      self->button_state);

  return TRUE;
}

// Implements GtkWidget::motion_notify_event.
static gboolean fl_view_motion_notify_event(GtkWidget* widget,
                                            GdkEventMotion* event) {
  FlView* self = FL_VIEW(widget);

  if (self->engine == nullptr) {
    return FALSE;
  }

  gint scale_factor = gtk_widget_get_scale_factor(GTK_WIDGET(self));
  fl_engine_send_mouse_pointer_event(
      self->engine, self->button_state != 0 ? kMove : kHover,
      event->time * kMicrosecondsPerMillisecond, event->x * scale_factor,
      event->y * scale_factor, 0, 0, self->button_state);

  return TRUE;
}

// Implements GtkWidget::key_press_event.
static gboolean fl_view_key_press_event(GtkWidget* widget, GdkEventKey* event) {
  FlView* self = FL_VIEW(widget);

  fl_key_event_plugin_send_key_event(self->key_event_plugin, event);
  fl_text_input_plugin_filter_keypress(self->text_input_plugin, event);
  fl_view_send_key_event(self, event);

  return TRUE;
}

// Implements GtkWidget::key_release_event.
static gboolean fl_view_key_release_event(GtkWidget* widget,
                                          GdkEventKey* event) {
  FlView* self = FL_VIEW(widget);

  fl_key_event_plugin_send_key_event(self->key_event_plugin, event);
  fl_text_input_plugin_filter_keypress(self->text_input_plugin, event);
  fl_view_send_key_event(self, event);

  return TRUE;
}

static void fl_view_class_init(FlViewClass* klass) {
  G_OBJECT_CLASS(klass)->constructed = fl_view_constructed;
  G_OBJECT_CLASS(klass)->set_property = fl_view_set_property;
  G_OBJECT_CLASS(klass)->get_property = fl_view_get_property;
  G_OBJECT_CLASS(klass)->notify = fl_view_notify;
  G_OBJECT_CLASS(klass)->dispose = fl_view_dispose;
  GTK_WIDGET_CLASS(klass)->realize = fl_view_realize;
  GTK_WIDGET_CLASS(klass)->size_allocate = fl_view_size_allocate;
  GTK_WIDGET_CLASS(klass)->button_press_event = fl_view_button_press_event;
  GTK_WIDGET_CLASS(klass)->button_release_event = fl_view_button_release_event;
  GTK_WIDGET_CLASS(klass)->scroll_event = fl_view_scroll_event;
  GTK_WIDGET_CLASS(klass)->motion_notify_event = fl_view_motion_notify_event;
  GTK_WIDGET_CLASS(klass)->key_press_event = fl_view_key_press_event;
  GTK_WIDGET_CLASS(klass)->key_release_event = fl_view_key_release_event;

  g_object_class_install_property(
      G_OBJECT_CLASS(klass), PROP_FLUTTER_PROJECT,
      g_param_spec_object(
          "flutter-project", "flutter-project", "Flutter project in use",
          fl_dart_project_get_type(),
          static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                                   G_PARAM_STATIC_STRINGS)));
}

static void fl_view_init(FlView* self) {
  gtk_widget_set_can_focus(GTK_WIDGET(self), TRUE);
}

G_MODULE_EXPORT FlView* fl_view_new(FlDartProject* project) {
  return static_cast<FlView*>(
      g_object_new(fl_view_get_type(), "flutter-project", project, nullptr));
}

G_MODULE_EXPORT FlEngine* fl_view_get_engine(FlView* view) {
  g_return_val_if_fail(FL_IS_VIEW(view), nullptr);
  return view->engine;
}
