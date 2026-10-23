/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8; c-indent-level: 8 -*- */
/* this file is part of xreader, a mate document viewer
 *
 *  Copyright (C) 2007 Carlos Garcia Campos <carlosgc@gnome.org>
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

#include <config.h>
#include "ev-form-field.h"

static void ev_form_field_init                 (EvFormField               *field);
static void ev_form_field_class_init           (EvFormFieldClass          *klass);
static void ev_form_field_text_init            (EvFormFieldText           *field_text);
static void ev_form_field_text_class_init      (EvFormFieldTextClass      *klass);
static void ev_form_field_button_init          (EvFormFieldButton         *field_button);
static void ev_form_field_button_class_init    (EvFormFieldButtonClass    *klass);
static void ev_form_field_choice_init          (EvFormFieldChoice         *field_choice);
static void ev_form_field_choice_class_init    (EvFormFieldChoiceClass    *klass);
static void ev_form_field_signature_init       (EvFormFieldSignature      *field_choice);
static void ev_form_field_signature_class_init (EvFormFieldSignatureClass *klass);

G_DEFINE_ABSTRACT_TYPE (EvFormField, ev_form_field, G_TYPE_OBJECT)
G_DEFINE_TYPE (EvFormFieldText, ev_form_field_text, EV_TYPE_FORM_FIELD)
G_DEFINE_TYPE (EvFormFieldButton, ev_form_field_button, EV_TYPE_FORM_FIELD)
G_DEFINE_TYPE (EvFormFieldChoice, ev_form_field_choice, EV_TYPE_FORM_FIELD)
G_DEFINE_TYPE (EvFormFieldSignature, ev_form_field_signature, EV_TYPE_FORM_FIELD)

static void
ev_form_field_init (EvFormField *field)
{
	field->page = NULL;
	field->changed = FALSE;
	field->is_read_only = FALSE;
}

static void
ev_form_field_finalize (GObject *object)
{
	EvFormField *field = EV_FORM_FIELD (object);

	g_object_unref (field->page);
	field->page = NULL;

	(* G_OBJECT_CLASS (ev_form_field_parent_class)->finalize) (object);
}

static void
ev_form_field_class_init (EvFormFieldClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = ev_form_field_finalize;
}

static void
ev_form_field_text_finalize (GObject *object)
{
	EvFormFieldText *field_text = EV_FORM_FIELD_TEXT (object);

	if (field_text->text) {
		g_free (field_text->text);
		field_text->text = NULL;
	}

	(* G_OBJECT_CLASS (ev_form_field_text_parent_class)->finalize) (object);
}

static void
ev_form_field_text_init (EvFormFieldText *field_text)
{
}

static void
ev_form_field_text_class_init (EvFormFieldTextClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = ev_form_field_text_finalize;
}

static void
ev_form_field_button_init (EvFormFieldButton *field_button)
{
}

static void
ev_form_field_button_class_init (EvFormFieldButtonClass *klass)
{
}

static void
ev_form_field_choice_finalize (GObject *object)
{
	EvFormFieldChoice *field_choice = EV_FORM_FIELD_CHOICE (object);

	if (field_choice->selected_items) {
		g_list_free (field_choice->selected_items);
		field_choice->selected_items = NULL;
	}

	if (field_choice->text) {
		g_free (field_choice->text);
		field_choice->text = NULL;
	}

	(* G_OBJECT_CLASS (ev_form_field_choice_parent_class)->finalize) (object);
}

static void
ev_form_field_choice_init (EvFormFieldChoice *field_choice)
{
}

static void
ev_form_field_choice_class_init (EvFormFieldChoiceClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = ev_form_field_choice_finalize;
}

static void
ev_form_field_signature_init (EvFormFieldSignature *field_signature)
{
}

static void
ev_form_field_signature_class_init (EvFormFieldSignatureClass *klass)
{
}

/**
 * ev_form_field_type_to_string:
 * @type: an #EvFormFieldType
 *
 * Returns the canonical string name for @type.  See
 * ev_form_field_type_to_string() in ev-form-field.h for
 * the full contract.
 *
 * Since: 4.8.0
 */
const gchar *
ev_form_field_type_to_string (EvFormFieldType type)
{
	switch (type) {
	case EV_FORM_FIELD_TEXT:       return "text";
	case EV_FORM_FIELD_BUTTON:     return "button";
	case EV_FORM_FIELD_CHOICE:     return "choice";
	case EV_FORM_FIELD_SIGNATURE:  return "signature";
	default:                       return "unknown";
	}
}

/**
 * ev_form_field_type_from_string:
 * @str: a string (or %NULL)
 *
 * Returns the #EvFormFieldType for @str.  See
 * ev_form_field_type_from_string() in ev-form-field.h for
 * the full contract.
 *
 * Since: 4.8.0
 */
EvFormFieldType
ev_form_field_type_from_string (const gchar *str)
{
	if (str == NULL)
		return EV_FORM_FIELD_TEXT;
	if (g_ascii_strcasecmp (str, "text") == 0)
		return EV_FORM_FIELD_TEXT;
	if (g_ascii_strcasecmp (str, "button") == 0)
		return EV_FORM_FIELD_BUTTON;
	if (g_ascii_strcasecmp (str, "choice") == 0)
		return EV_FORM_FIELD_CHOICE;
	if (g_ascii_strcasecmp (str, "signature") == 0)
		return EV_FORM_FIELD_SIGNATURE;
	return EV_FORM_FIELD_TEXT;
}

EvFormField *
ev_form_field_text_new (gint                id,
			EvFormFieldTextType type)
{
	EvFormField *field;
	
	g_return_val_if_fail (id >= 0, NULL);
	g_return_val_if_fail (type >= EV_FORM_FIELD_TEXT_NORMAL &&
			      type <= EV_FORM_FIELD_TEXT_FILE_SELECT, NULL);

	field = EV_FORM_FIELD (g_object_new (EV_TYPE_FORM_FIELD_TEXT, NULL));
	field->id = id;
	EV_FORM_FIELD_TEXT (field)->type = type;

	return field;
}

EvFormField *
ev_form_field_button_new (gint                  id,
			  EvFormFieldButtonType type)
{
	EvFormField *field;

	g_return_val_if_fail (id >= 0, NULL);
	g_return_val_if_fail (type >= EV_FORM_FIELD_BUTTON_PUSH &&
			      type <= EV_FORM_FIELD_BUTTON_RADIO, NULL);

	field = EV_FORM_FIELD (g_object_new (EV_TYPE_FORM_FIELD_BUTTON, NULL));
	field->id = id;
	EV_FORM_FIELD_BUTTON (field)->type = type;

	return field;
}

EvFormField *
ev_form_field_choice_new (gint                  id,
			  EvFormFieldChoiceType type)
{
	EvFormField *field;

	g_return_val_if_fail (id >= 0, NULL);
	g_return_val_if_fail (type >= EV_FORM_FIELD_CHOICE_COMBO &&
			      type <= EV_FORM_FIELD_CHOICE_LIST, NULL);
	
	field = EV_FORM_FIELD (g_object_new (EV_TYPE_FORM_FIELD_CHOICE, NULL));
	field->id = id;
	EV_FORM_FIELD_CHOICE (field)->type = type;

	return field;
}

EvFormField *
ev_form_field_signature_new (gint id)
{
	EvFormField *field;

	g_return_val_if_fail (id >= 0, NULL);

	field = EV_FORM_FIELD (g_object_new (EV_TYPE_FORM_FIELD_SIGNATURE, NULL));
	field->id = id;

	return field;
}

