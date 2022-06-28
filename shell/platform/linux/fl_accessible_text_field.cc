// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/linux/fl_accessible_text_field.h"
#include "flutter/shell/platform/linux/public/flutter_linux/fl_standard_message_codec.h"
#include "flutter/shell/platform/linux/public/flutter_linux/fl_value.h"

struct _FlAccessibleTextField {
  FlAccessibleNode parent_instance;

  gint selection_base;
  gint selection_extent;
  GtkEntryBuffer* buffer;
};

static void fl_accessible_text_iface_init(AtkTextIface* iface);
static void fl_accessible_editable_text_iface_init(AtkEditableTextIface* iface);

G_DEFINE_TYPE_WITH_CODE(
    FlAccessibleTextField,
    fl_accessible_text_field,
    FL_TYPE_ACCESSIBLE_NODE,
    G_IMPLEMENT_INTERFACE(ATK_TYPE_TEXT, fl_accessible_text_iface_init)
        G_IMPLEMENT_INTERFACE(ATK_TYPE_EDITABLE_TEXT,
                              fl_accessible_editable_text_iface_init))

static gchar* get_substring(FlAccessibleTextField* self,
                            glong start,
                            glong end) {
  const gchar* value = gtk_entry_buffer_get_text(self->buffer);
  if (end == -1) {
    // g_utf8_substring() accepts -1 since 2.72
    end = g_utf8_strlen(value, -1);
  }
  return g_utf8_substring(value, start, end);
}

static void perform_set_text_action(FlAccessibleTextField* self,
                                    const char* text) {
  g_autoptr(FlValue) value = fl_value_new_string(text);
  g_autoptr(FlStandardMessageCodec) codec = fl_standard_message_codec_new();
  g_autoptr(GBytes) message =
      fl_message_codec_encode_message(FL_MESSAGE_CODEC(codec), value, nullptr);

  fl_accessible_node_perform_action(FL_ACCESSIBLE_NODE(self),
                                    kFlutterSemanticsActionSetText, message);
}

static void perform_set_selection_action(FlAccessibleTextField* self,
                                         gint base,
                                         gint extent) {
  g_autoptr(FlValue) value = fl_value_new_map();
  fl_value_set_string_take(value, "base", fl_value_new_int(base));
  fl_value_set_string_take(value, "extent", fl_value_new_int(extent));

  g_autoptr(FlStandardMessageCodec) codec = fl_standard_message_codec_new();
  g_autoptr(GBytes) message =
      fl_message_codec_encode_message(FL_MESSAGE_CODEC(codec), value, nullptr);

  fl_accessible_node_perform_action(
      FL_ACCESSIBLE_NODE(self), kFlutterSemanticsActionSetSelection, message);
}

// Implements GObject::dispose.
static void fl_accessible_text_field_dispose(GObject* object) {
  FlAccessibleTextField* self = FL_ACCESSIBLE_TEXT_FIELD(object);

  g_clear_object(&self->buffer);

  G_OBJECT_CLASS(fl_accessible_text_field_parent_class)->dispose(object);
}

// Implements FlAccessibleNode::set_value.
static void fl_accessible_text_field_set_value(FlAccessibleNode* node,
                                               const gchar* value) {
  g_return_if_fail(FL_IS_ACCESSIBLE_TEXT_FIELD(node));
  FlAccessibleTextField* self = FL_ACCESSIBLE_TEXT_FIELD(node);

  if (g_strcmp0(gtk_entry_buffer_get_text(self->buffer), value) == 0) {
    return;
  }

  gtk_entry_buffer_set_text(self->buffer, value, -1);
}

// Implements FlAccessibleNode::set_text_selection.
static void fl_accessible_text_field_set_text_selection(FlAccessibleNode* node,
                                                        gint base,
                                                        gint extent) {
  g_return_if_fail(FL_IS_ACCESSIBLE_TEXT_FIELD(node));
  FlAccessibleTextField* self = FL_ACCESSIBLE_TEXT_FIELD(node);

  gboolean caret_moved = extent != self->selection_extent;
  gboolean has_selection = base != extent;
  gboolean had_selection = self->selection_base != self->selection_extent;
  gboolean selection_changed = (has_selection || had_selection) &&
                               (caret_moved || base != self->selection_base);

  self->selection_base = base;
  self->selection_extent = extent;

  if (selection_changed) {
    g_signal_emit_by_name(self, "text-selection-changed", nullptr);
  }

  if (caret_moved) {
    g_signal_emit_by_name(self, "text-caret-moved", extent, nullptr);
  }
}

// Overrides FlAccessibleNode::perform_action.
void fl_accessible_text_field_perform_action(FlAccessibleNode* self,
                                             FlutterSemanticsAction action,
                                             GBytes* data) {
  FlAccessibleNodeClass* parent_class =
      FL_ACCESSIBLE_NODE_CLASS(fl_accessible_text_field_parent_class);

  switch (action) {
    case kFlutterSemanticsActionMoveCursorForwardByCharacter:
    case kFlutterSemanticsActionMoveCursorBackwardByCharacter:
    case kFlutterSemanticsActionMoveCursorForwardByWord:
    case kFlutterSemanticsActionMoveCursorBackwardByWord: {
      // These actions require a boolean argument that indicates whether the
      // selection should be extended or collapsed when moving the cursor.
      g_autoptr(FlValue) extend_selection = fl_value_new_bool(false);
      g_autoptr(FlStandardMessageCodec) codec = fl_standard_message_codec_new();
      g_autoptr(GBytes) message = fl_message_codec_encode_message(
          FL_MESSAGE_CODEC(codec), extend_selection, nullptr);
      parent_class->perform_action(self, action, message);
      break;
    }
    default:
      parent_class->perform_action(self, action, data);
      break;
  }
}

// Implements AtkText::get_character_count.
static gint fl_accessible_text_field_get_character_count(AtkText* text) {
  g_return_val_if_fail(FL_IS_ACCESSIBLE_TEXT_FIELD(text), 0);
  FlAccessibleTextField* self = FL_ACCESSIBLE_TEXT_FIELD(text);

  return gtk_entry_buffer_get_length(self->buffer);
}

// Implements AtkText::get_text.
static gchar* fl_accessible_text_field_get_text(AtkText* text,
                                                gint start_offset,
                                                gint end_offset) {
  g_return_val_if_fail(FL_IS_ACCESSIBLE_TEXT_FIELD(text), nullptr);
  FlAccessibleTextField* self = FL_ACCESSIBLE_TEXT_FIELD(text);

  return get_substring(self, start_offset, end_offset);
}

// Implements AtkText::get_caret_offset.
static gint fl_accessible_text_field_get_caret_offset(AtkText* text) {
  g_return_val_if_fail(FL_IS_ACCESSIBLE_TEXT_FIELD(text), -1);
  FlAccessibleTextField* self = FL_ACCESSIBLE_TEXT_FIELD(text);

  return self->selection_extent;
}

// Implements AtkText::set_caret_offset.
static gboolean fl_accessible_text_field_set_caret_offset(AtkText* text,
                                                          gint offset) {
  g_return_val_if_fail(FL_IS_ACCESSIBLE_TEXT_FIELD(text), false);
  FlAccessibleTextField* self = FL_ACCESSIBLE_TEXT_FIELD(text);

  perform_set_selection_action(self, offset, offset);
  return true;
}

// Implements AtkText::get_n_selections.
static gint fl_accessible_text_field_get_n_selections(AtkText* text) {
  g_return_val_if_fail(FL_IS_ACCESSIBLE_TEXT_FIELD(text), 0);
  FlAccessibleTextField* self = FL_ACCESSIBLE_TEXT_FIELD(text);

  if (self->selection_base == self->selection_extent) {
    return 0;
  }

  return 1;
}

// Implements AtkText::get_selection.
static gchar* fl_accessible_text_field_get_selection(AtkText* text,
                                                     gint selection_num,
                                                     gint* start_offset,
                                                     gint* end_offset) {
  g_return_val_if_fail(FL_IS_ACCESSIBLE_TEXT_FIELD(text), nullptr);
  FlAccessibleTextField* self = FL_ACCESSIBLE_TEXT_FIELD(text);

  if (selection_num != 0 || self->selection_base == self->selection_extent) {
    return nullptr;
  }

  gint start = MIN(self->selection_base, self->selection_extent);
  gint end = MAX(self->selection_base, self->selection_extent);

  if (start_offset != nullptr) {
    *start_offset = start;
  }
  if (end_offset != nullptr) {
    *end_offset = end;
  }

  return get_substring(self, start, end);
}

// Implements AtkText::add_selection.
static gboolean fl_accessible_text_field_add_selection(AtkText* text,
                                                       gint start_offset,
                                                       gint end_offset) {
  g_return_val_if_fail(FL_IS_ACCESSIBLE_TEXT_FIELD(text), false);
  FlAccessibleTextField* self = FL_ACCESSIBLE_TEXT_FIELD(text);

  if (self->selection_base != self->selection_extent) {
    return false;
  }

  perform_set_selection_action(self, start_offset, end_offset);
  return true;
}

// Implements AtkText::remove_selection.
static gboolean fl_accessible_text_field_remove_selection(AtkText* text,
                                                          gint selection_num) {
  g_return_val_if_fail(FL_IS_ACCESSIBLE_TEXT_FIELD(text), false);
  FlAccessibleTextField* self = FL_ACCESSIBLE_TEXT_FIELD(text);

  if (selection_num != 0 || self->selection_base == self->selection_extent) {
    return false;
  }

  perform_set_selection_action(self, self->selection_extent,
                               self->selection_extent);
  return true;
}

// Implements AtkText::set_selection.
static gboolean fl_accessible_text_field_set_selection(AtkText* text,
                                                       gint selection_num,
                                                       gint start_offset,
                                                       gint end_offset) {
  g_return_val_if_fail(FL_IS_ACCESSIBLE_TEXT_FIELD(text), false);
  FlAccessibleTextField* self = FL_ACCESSIBLE_TEXT_FIELD(text);

  if (selection_num != 0) {
    return false;
  }

  perform_set_selection_action(self, start_offset, end_offset);
  return true;
}

// Implements AtkEditableText::set_text_contents.
static void fl_accessible_text_field_set_text_contents(
    AtkEditableText* editable_text,
    const gchar* string) {
  g_return_if_fail(FL_IS_ACCESSIBLE_TEXT_FIELD(editable_text));
  FlAccessibleTextField* self = FL_ACCESSIBLE_TEXT_FIELD(editable_text);

  perform_set_text_action(self, string);
}

// Implements AtkEditableText::insert_text.
static void fl_accessible_text_field_insert_text(AtkEditableText* editable_text,
                                                 const gchar* string,
                                                 gint length,
                                                 gint* position) {
  g_return_if_fail(FL_IS_ACCESSIBLE_TEXT_FIELD(editable_text));
  FlAccessibleTextField* self = FL_ACCESSIBLE_TEXT_FIELD(editable_text);

  *position +=
      gtk_entry_buffer_insert_text(self->buffer, *position, string, length);

  perform_set_text_action(self, gtk_entry_buffer_get_text(self->buffer));
  perform_set_selection_action(self, *position, *position);
}

// Implements AtkEditableText::delete_text.
static void fl_accessible_node_delete_text(AtkEditableText* editable_text,
                                           gint start_pos,
                                           gint end_pos) {
  g_return_if_fail(FL_IS_ACCESSIBLE_TEXT_FIELD(editable_text));
  FlAccessibleTextField* self = FL_ACCESSIBLE_TEXT_FIELD(editable_text);

  gtk_entry_buffer_delete_text(self->buffer, start_pos, end_pos - start_pos);

  perform_set_text_action(self, gtk_entry_buffer_get_text(self->buffer));
  perform_set_selection_action(self, start_pos, start_pos);
}

// Implement AtkEditableText::copy_text.
static void fl_accessible_text_field_copy_text(AtkEditableText* editable_text,
                                               gint start_pos,
                                               gint end_pos) {
  g_return_if_fail(FL_IS_ACCESSIBLE_TEXT_FIELD(editable_text));
  FlAccessibleTextField* self = FL_ACCESSIBLE_TEXT_FIELD(editable_text);

  perform_set_selection_action(self, start_pos, end_pos);

  fl_accessible_node_perform_action(FL_ACCESSIBLE_NODE(editable_text),
                                    kFlutterSemanticsActionCopy, nullptr);
}

// Implements AtkEditableText::cut_text.
static void fl_accessible_text_field_cut_text(AtkEditableText* editable_text,
                                              gint start_pos,
                                              gint end_pos) {
  g_return_if_fail(FL_IS_ACCESSIBLE_TEXT_FIELD(editable_text));
  FlAccessibleTextField* self = FL_ACCESSIBLE_TEXT_FIELD(editable_text);

  perform_set_selection_action(self, start_pos, end_pos);

  fl_accessible_node_perform_action(FL_ACCESSIBLE_NODE(editable_text),
                                    kFlutterSemanticsActionCut, nullptr);
}

// Implements AtkEditableText::paste_text.
static void fl_accessible_text_field_paste_text(AtkEditableText* editable_text,
                                                gint position) {
  g_return_if_fail(FL_IS_ACCESSIBLE_TEXT_FIELD(editable_text));
  FlAccessibleTextField* self = FL_ACCESSIBLE_TEXT_FIELD(editable_text);

  perform_set_selection_action(self, position, position);

  fl_accessible_node_perform_action(FL_ACCESSIBLE_NODE(editable_text),
                                    kFlutterSemanticsActionPaste, nullptr);
}

static void fl_accessible_text_field_class_init(
    FlAccessibleTextFieldClass* klass) {
  G_OBJECT_CLASS(klass)->dispose = fl_accessible_text_field_dispose;
  FL_ACCESSIBLE_NODE_CLASS(klass)->set_value =
      fl_accessible_text_field_set_value;
  FL_ACCESSIBLE_NODE_CLASS(klass)->set_text_selection =
      fl_accessible_text_field_set_text_selection;
  FL_ACCESSIBLE_NODE_CLASS(klass)->perform_action =
      fl_accessible_text_field_perform_action;
}

static void fl_accessible_text_iface_init(AtkTextIface* iface) {
  iface->get_character_count = fl_accessible_text_field_get_character_count;
  iface->get_text = fl_accessible_text_field_get_text;
  // TODO(jpnurmi): get_text_at/before/after_offset

  iface->get_caret_offset = fl_accessible_text_field_get_caret_offset;
  iface->set_caret_offset = fl_accessible_text_field_set_caret_offset;

  iface->get_n_selections = fl_accessible_text_field_get_n_selections;
  iface->get_selection = fl_accessible_text_field_get_selection;
  iface->add_selection = fl_accessible_text_field_add_selection;
  iface->remove_selection = fl_accessible_text_field_remove_selection;
  iface->set_selection = fl_accessible_text_field_set_selection;
}

static void fl_accessible_editable_text_iface_init(
    AtkEditableTextIface* iface) {
  iface->set_text_contents = fl_accessible_text_field_set_text_contents;
  iface->insert_text = fl_accessible_text_field_insert_text;
  iface->delete_text = fl_accessible_node_delete_text;

  iface->copy_text = fl_accessible_text_field_copy_text;
  iface->cut_text = fl_accessible_text_field_cut_text;
  iface->paste_text = fl_accessible_text_field_paste_text;
}

static void fl_accessible_text_field_init(FlAccessibleTextField* self) {
  self->selection_base = -1;
  self->selection_extent = -1;

  self->buffer = gtk_entry_buffer_new("", 0);

  g_signal_connect_object(
      self->buffer, "inserted-text",
      G_CALLBACK(+[](FlAccessibleTextField* self, guint position, gchar* chars,
                     guint n_chars) {
        g_signal_emit_by_name(self, "text-insert", position, n_chars, chars,
                              nullptr);
      }),
      self, G_CONNECT_SWAPPED);

  g_signal_connect_object(self->buffer, "deleted-text",
                          G_CALLBACK(+[](FlAccessibleTextField* self,
                                         guint position, guint n_chars) {
                            g_autofree gchar* chars = atk_text_get_text(
                                ATK_TEXT(self), position, position + n_chars);
                            g_signal_emit_by_name(self, "text-remove", position,
                                                  n_chars, chars, nullptr);
                          }),
                          self, G_CONNECT_SWAPPED);
}

FlAccessibleNode* fl_accessible_text_field_new(FlEngine* engine, int32_t id) {
  return FL_ACCESSIBLE_NODE(g_object_new(fl_accessible_text_field_get_type(),
                                         "engine", engine, "id", id, nullptr));
}
