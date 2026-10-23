/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8; c-indent-level: 8 -*- */
/* this file is part of xreader, a mate document viewer
 *
 *  Copyright (C) 2006 Julien Rebetez
 *
 * Xreader is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Xreader is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#if !defined (__EV_XREADER_DOCUMENT_H_INSIDE__) && !defined (XREADER_COMPILATION)
#error "Only <xreader-document.h> can be included directly."
#endif

#ifndef EV_FORM_FIELD_H
#define EV_FORM_FIELD_H

#include <glib-object.h>

#include "ev-document.h"

G_BEGIN_DECLS

#define EV_TYPE_FORM_FIELD                        (ev_form_field_get_type())
#define EV_FORM_FIELD(object)                     (G_TYPE_CHECK_INSTANCE_CAST((object), EV_TYPE_FORM_FIELD, EvFormField))
#define EV_FORM_FIELD_CLASS(klass)                (G_TYPE_CHECK_CLASS_CAST((klass), EV_TYPE_FORM_FIELD, EvFormFieldClass))
#define EV_IS_FORM_FIELD(object)                  (G_TYPE_CHECK_INSTANCE_TYPE((object), EV_TYPE_FORM_FIELD))
#define EV_IS_FORM_FIELD_CLASS(klass)             (G_TYPE_CHECK_CLASS_TYPE((klass), EV_TYPE_FORM_FIELD))
#define EV_FORM_FIELD_GET_CLASS(object)           (G_TYPE_INSTANCE_GET_CLASS((object), EV_TYPE_FORM_FIELD, EvFormFieldClass)) 

#define EV_TYPE_FORM_FIELD_TEXT                   (ev_form_field_text_get_type())
#define EV_FORM_FIELD_TEXT(object)                (G_TYPE_CHECK_INSTANCE_CAST((object), EV_TYPE_FORM_FIELD_TEXT, EvFormFieldText))
#define EV_FORM_FIELD_TEXT_CLASS(klass)           (G_TYPE_CHECK_CLASS_CAST((klass), EV_TYPE_FORM_FIELD_TEXT, EvFormFieldTextClass))
#define EV_IS_FORM_FIELD_TEXT(object)             (G_TYPE_CHECK_INSTANCE_TYPE((object), EV_TYPE_FORM_FIELD_TEXT))
#define EV_IS_FORM_FIELD_TEXT_CLASS(klass)        (G_TYPE_CHECK_CLASS_TYPE((klass), EV_TYPE_FORM_FIELD_TEXT))
#define EV_FORM_FIELD_TEXT_GET_CLASS(object)      (G_TYPE_INSTANCE_GET_CLASS((object), EV_TYPE_FORM_FIELD_TEXT, EvFormFieldTextClass))

#define EV_TYPE_FORM_FIELD_BUTTON                 (ev_form_field_button_get_type())
#define EV_FORM_FIELD_BUTTON(object)              (G_TYPE_CHECK_INSTANCE_CAST((object), EV_TYPE_FORM_FIELD_BUTTON, EvFormFieldButton))
#define EV_FORM_FIELD_BUTTON_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST((klass), EV_TYPE_FORM_FIELD_BUTTON, EvFormFieldButtonClass))
#define EV_IS_FORM_FIELD_BUTTON(object)           (G_TYPE_CHECK_INSTANCE_TYPE((object), EV_TYPE_FORM_FIELD_BUTTON))
#define EV_IS_FORM_FIELD_BUTTON_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE((klass), EV_TYPE_FORM_FIELD_BUTTON))
#define EV_FORM_FIELD_BUTTON_GET_CLASS(object)    (G_TYPE_INSTANCE_GET_CLASS((object), EV_TYPE_FORM_FIELD_BUTTON, EvFormFieldButtonClass))

#define EV_TYPE_FORM_FIELD_CHOICE                 (ev_form_field_choice_get_type())
#define EV_FORM_FIELD_CHOICE(object)              (G_TYPE_CHECK_INSTANCE_CAST((object), EV_TYPE_FORM_FIELD_CHOICE, EvFormFieldChoice))
#define EV_FORM_FIELD_CHOICE_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST((klass), EV_TYPE_FORM_FIELD_CHOICE, EvFormFieldChoiceClass))
#define EV_IS_FORM_FIELD_CHOICE(object)           (G_TYPE_CHECK_INSTANCE_TYPE((object), EV_TYPE_FORM_FIELD_CHOICE))
#define EV_IS_FORM_FIELD_CHOICE_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE((klass), EV_TYPE_FORM_FIELD_CHOICE))
#define EV_FORM_FIELD_CHOICE_GET_CLASS(object)    (G_TYPE_INSTANCE_GET_CLASS((object), EV_TYPE_FORM_FIELD_CHOICE, EvFormFieldChoiceClass))

#define EV_TYPE_FORM_FIELD_SIGNATURE              (ev_form_field_signature_get_type())
#define EV_FORM_FIELD_SIGNATURE(object)           (G_TYPE_CHECK_INSTANCE_CAST((object), EV_TYPE_FORM_FIELD_SIGNATURE, EvFormFieldSignature))
#define EV_FORM_FIELD_SIGNATURE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), EV_TYPE_FORM_FIELD_SIGNATURE, EvFormFieldSignatureClass))
#define EV_IS_FORM_FIELD_SIGNATURE(object)        (G_TYPE_CHECK_INSTANCE_TYPE((object), EV_TYPE_FORM_FIELD_SIGNATURE))
#define EV_IS_FORM_FIELD_SIGNATURE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), EV_TYPE_FORM_FIELD_SIGNATURE))
#define EV_FORM_FIELD_SIGNATURE_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS((object), EV_TYPE_FORM_FIELD_SIGNATURE, EvFormFieldSignatureClass))

typedef struct _EvFormField               EvFormField;
typedef struct _EvFormFieldClass          EvFormFieldClass;

typedef struct _EvFormFieldText           EvFormFieldText;
typedef struct _EvFormFieldTextClass      EvFormFieldTextClass;

typedef struct _EvFormFieldButton         EvFormFieldButton;
typedef struct _EvFormFieldButtonClass    EvFormFieldButtonClass;

typedef struct _EvFormFieldChoice         EvFormFieldChoice;
typedef struct _EvFormFieldChoiceClass    EvFormFieldChoiceClass;

typedef struct _EvFormFieldSignature      EvFormFieldSignature;
typedef struct _EvFormFieldSignatureClass EvFormFieldSignatureClass;

typedef enum
{
	EV_FORM_FIELD_TEXT_NORMAL,
	EV_FORM_FIELD_TEXT_MULTILINE,
	EV_FORM_FIELD_TEXT_FILE_SELECT
} EvFormFieldTextType;

typedef enum
{
	EV_FORM_FIELD_BUTTON_PUSH,
	EV_FORM_FIELD_BUTTON_CHECK,
	EV_FORM_FIELD_BUTTON_RADIO
} EvFormFieldButtonType;

typedef enum
{
	EV_FORM_FIELD_CHOICE_COMBO,
	EV_FORM_FIELD_CHOICE_LIST
} EvFormFieldChoiceType;

/**
 * EvFormFieldType:
 * @EV_FORM_FIELD_TEXT: a text field (single-line, multi-line, or
 *   file-select; see #EvFormFieldTextType for the sub-type)
 * @EV_FORM_FIELD_BUTTON: a button field (push, check-box, or
 *   radio; see #EvFormFieldButtonType for the sub-type)
 * @EV_FORM_FIELD_CHOICE: a choice field (combo-box or list;
 *   see #EvFormFieldChoiceType for the sub-type)
 * @EV_FORM_FIELD_SIGNATURE: a signature field (placeholder
 *   widget; the actual signature data is captured out-of-band)
 *
 * The high-level type of an #EvFormField.  This is the type
 * used by the introspection / serialization layer (see
 * ev_form_field_type_to_string() and
 * ev_form_field_type_from_string() for the canonical string
 * mapping).
 *
 * The concrete sub-type is stored on the subclass
 * (e.g. #EvFormFieldTextType on an #EvFormFieldText); this
 * enum is the "category" used by the B6 PDF form-filling UI
 * to pick the right widget.
 *
 * Since: 4.8.0
 */
typedef enum
{
	EV_FORM_FIELD_TEXT,
	EV_FORM_FIELD_BUTTON,
	EV_FORM_FIELD_CHOICE,
	EV_FORM_FIELD_SIGNATURE
} EvFormFieldType;

struct _EvFormField
{
	GObject parent;

	gint     id;
	gboolean is_read_only;
	gdouble  font_size;

	EvPage  *page;
	gboolean changed;
};

struct _EvFormFieldClass
{
	GObjectClass parent_class;
};
	
struct _EvFormFieldText
{
	EvFormField partent;
	
	EvFormFieldTextType type;
	
	gboolean do_spell_check : 1;
	gboolean do_scroll : 1;
	gboolean comb : 1;
	gboolean is_rich_text : 1;
	gboolean is_password;

	gint   max_len;
	gchar *text;
};

struct _EvFormFieldTextClass
{
	EvFormFieldClass partent_class;
};

struct _EvFormFieldButton
{
	EvFormField partent;
	
	EvFormFieldButtonType type;

	gboolean state;
};

struct _EvFormFieldButtonClass
{
	EvFormFieldClass partent_class;
};

struct _EvFormFieldChoice
{
	EvFormField partent;

	EvFormFieldChoiceType type;
	
	gboolean multi_select : 1;
	gboolean is_editable : 1;
	gboolean do_spell_check : 1;
	gboolean commit_on_sel_change : 1;

	GList *selected_items;
	gchar *text;
};

struct _EvFormFieldChoiceClass
{
	EvFormFieldClass partent_class;
};

struct _EvFormFieldSignature
{
	EvFormField partent;
	
	/* TODO */
};

struct _EvFormFieldSignatureClass
{
	EvFormFieldClass partent_class;
};

/* EvFormField base class */
GType        ev_form_field_get_type           (void) G_GNUC_CONST;

/**
 * ev_form_field_get_id:
 * @field: an #EvFormField
 *
 * Returns the per-document id of the field.  The id is unique
 * within a single document (two fields from different documents
 * may share an id).
 *
 * Returns: a non-negative integer
 *
 * Since: 4.9.0
 */
gint         ev_form_field_get_id             (EvFormField *field);

/**
 * ev_form_field_get_is_read_only:
 * @field: an #EvFormField
 *
 * Returns whether the field is read-only (the user cannot
 * change its value).  The poppler backend sets this based on
 * the field's annotation flags.
 *
 * Returns: %TRUE if the field is read-only
 *
 * Since: 4.9.0
 */
gboolean     ev_form_field_get_is_read_only   (EvFormField *field);

/**
 * ev_form_field_get_font_size:
 * @field: an #EvFormField
 *
 * Returns the default font size of the field's value, in
 * points.  A value of 0.0 means "use the document default".
 *
 * Returns: the font size in points
 *
 * Since: 4.9.0
 */
gdouble      ev_form_field_get_font_size      (EvFormField *field);

/**
 * ev_form_field_get_page:
 * @field: an #EvFormField
 *
 * Returns the page on which the field is rendered.  The
 * returned reference is owned by @field; do not unref.
 *
 * Returns: (transfer none) (nullable): the #EvPage, or %NULL
 *
 * Since: 4.9.0
 */
EvPage      *ev_form_field_get_page           (EvFormField *field);

/**
 * ev_form_field_get_changed:
 * @field: an #EvFormField
 *
 * Returns whether the field has been modified since the
 * document was loaded.  This is the same flag the
 * "save" / "save-as" code checks to decide whether to
 * write the document back out.
 *
 * Returns: %TRUE if the field has unsaved changes
 *
 * Since: 4.9.0
 */
gboolean     ev_form_field_get_changed        (EvFormField *field);

/**
 * ev_form_field_set_changed:
 * @field: an #EvFormField
 * @changed: the new value of the changed flag
 *
 * Sets the changed flag.  The "changed" signal is NOT
 * emitted by this function (it is emitted by the
 * value-set accessors such as
 * ev_form_field_text_set_text()).
 *
 * This function is mainly useful for the save code to
 * reset the flag after a successful save.
 *
 * Since: 4.9.0
 */
void         ev_form_field_set_changed        (EvFormField *field,
					       gboolean     changed);

/* EvFormFieldText */
GType        ev_form_field_text_get_type      (void) G_GNUC_CONST;
EvFormField *ev_form_field_text_new           (gint                  id,
					       EvFormFieldTextType   type);

/**
 * ev_form_field_text_get_text:
 * @field_text: an #EvFormFieldText
 *
 * Returns the current value of the text field.  The returned
 * string is owned by @field_text; do not free.
 *
 * Returns: (transfer none) (nullable): the text value, or %NULL
 *
 * Since: 4.9.0
 */
const gchar *ev_form_field_text_get_text      (EvFormFieldText *field_text);

/**
 * ev_form_field_text_set_text:
 * @field_text: an #EvFormFieldText
 * @text: (nullable): the new text value
 *
 * Sets the value of the text field.  Sets the
 * EvFormField::changed flag and emits the "changed" signal
 * if the new value differs from the current value (string
 * comparison via strcmp).
 *
 * A %NULL @text clears the field (sets the text to NULL,
 * which the save code serializes as an empty string).
 *
 * Since: 4.9.0
 */
void         ev_form_field_text_set_text      (EvFormFieldText  *field_text,
					       const gchar      *text);

/* EvFormFieldButton */
GType        ev_form_field_button_get_type    (void) G_GNUC_CONST;
EvFormField *ev_form_field_button_new         (gint                  id,
					       EvFormFieldButtonType type);

/**
 * ev_form_field_button_get_state:
 * @field_button: an #EvFormFieldButton
 *
 * Returns the current state of a check-box or radio button
 * field.  For a push button, the state is always %FALSE.
 *
 * Returns: %TRUE if the button is "on" / "selected"
 *
 * Since: 4.9.0
 */
gboolean     ev_form_field_button_get_state   (EvFormFieldButton *field_button);

/**
 * ev_form_field_button_set_state:
 * @field_button: an #EvFormFieldButton
 * @state: the new state
 *
 * Sets the state of a check-box or radio button field.  Sets
 * the EvFormField::changed flag and emits the "changed"
 * signal if the new state differs from the current state.
 *
 * Setting the state of a push button is a no-op (the state
 * is reset to %FALSE on the next poppler round-trip).
 *
 * Since: 4.9.0
 */
void         ev_form_field_button_set_state   (EvFormFieldButton *field_button,
					       gboolean           state);

/* EvFormFieldChoice */
GType        ev_form_field_choice_get_type    (void) G_GNUC_CONST;
EvFormField *ev_form_field_choice_new         (gint                  id,
					       EvFormFieldChoiceType type);

/**
 * ev_form_field_choice_get_text:
 * @field_choice: an #EvFormFieldChoice
 *
 * Returns the current value of a combo-box field.  For a
 * list field, returns the text of the first selected item
 * (use ev_form_field_choice_get_selected_items() to get
 * the full list).
 *
 * Returns: (transfer none) (nullable): the text value, or %NULL
 *
 * Since: 4.9.0
 */
const gchar *ev_form_field_choice_get_text    (EvFormFieldChoice *field_choice);

/**
 * ev_form_field_choice_set_text:
 * @field_choice: an #EvFormFieldChoice
 * @text: (nullable): the new text value
 *
 * Sets the value of a combo-box field.  Sets the
 * EvFormField::changed flag and emits the "changed" signal
 * if the new value differs from the current value.
 *
 * For a list field, this is equivalent to clearing the
 * selection and setting the text; callers wanting to set
 * a specific list of items should use
 * ev_form_field_choice_set_selected_items() instead.
 *
 * Since: 4.9.0
 */
void         ev_form_field_choice_set_text    (EvFormFieldChoice  *field_choice,
					       const gchar        *text);

/**
 * ev_form_field_choice_get_selected_items:
 * @field_choice: an #EvFormFieldChoice
 *
 * Returns the list of currently selected item indices.
 * Each element is a #gint pointer (g_list_nth_data() to
 * read).  For a single-select list, the list has 0 or 1
 * elements.
 *
 * The returned list is owned by @field_choice; do not free
 * or modify.
 *
 * Returns: (transfer none) (element-type gint) (nullable): the
 *   list of selected indices, or %NULL if none
 *
 * Since: 4.9.0
 */
const GList *ev_form_field_choice_get_selected_items
                                              (EvFormFieldChoice *field_choice);

/**
 * ev_form_field_choice_set_selected_items:
 * @field_choice: an #EvFormFieldChoice
 * @items: (transfer full) (element-type gint) (nullable): the
 *   new list of selected indices
 *
 * Sets the list of selected item indices.  Takes ownership
 * of @items (it will be freed with g_list_free() and its
 * elements with g_free() when the field is finalized or
 * replaced).
 *
 * Sets the EvFormField::changed flag and emits the
 * "changed" signal.
 *
 * Since: 4.9.0
 */
void         ev_form_field_choice_set_selected_items
                                              (EvFormFieldChoice  *field_choice,
					       GList              *items);

/* EvFormFieldSignature */
GType        ev_form_field_signature_get_type (void) G_GNUC_CONST;
EvFormField *ev_form_field_signature_new      (gint                  id);

/* Type-name helpers (B6 form-filling UI)
 *
 * These map between the EvFormFieldType enum and the
 * canonical string name used by the introspection +
 * serialization layer.  The string form is stable across
 * releases; new values may be added but existing values
 * will not change.
 */

/**
 * ev_form_field_type_to_string:
 * @type: an #EvFormFieldType
 *
 * Returns the canonical string name for @type
 * ("text" / "button" / "choice" / "signature").
 * Out-of-range values (including uninitialized memory) map
 * to "unknown" so the function is safe to call with
 * untrusted input.
 *
 * Returns: a statically-allocated string; do not free
 *
 * Since: 4.8.0
 */
const gchar        *ev_form_field_type_to_string  (EvFormFieldType type);

/**
 * ev_form_field_type_from_string:
 * @str: a string (or %NULL)
 *
 * Returns the #EvFormFieldType for the given canonical
 * string name.  The comparison is case-insensitive
 * (g_ascii_strcasecmp).  Unknown input (including %NULL)
 * defaults to %EV_FORM_FIELD_TEXT, matching the behaviour
 * of the original PR #78 helper.
 *
 * Returns: the matching #EvFormFieldType, or
 *   %EV_FORM_FIELD_TEXT on unknown input
 *
 * Since: 4.8.0
 */
EvFormFieldType     ev_form_field_type_from_string (const gchar    *str);


G_END_DECLS

#endif /* !EV_FORM_FIELD_H */

