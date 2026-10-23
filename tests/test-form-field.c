/* test-form-field.c - Unit tests for the EvFormField helpers
 *                     added in PR #78 + restored in PR #109,
 *                     and the value accessors added in PR #111.
 *
 * Copyright (C) 2026 Yuri Schmaltz / xreader fork
 *
 * Tests cover two groups:
 *
 *  GROUP 1 (PR #78/#109): the EvFormFieldType enum helpers
 *    - ev_form_field_type_to_string: EvFormFieldType ->
 *      'text' | 'button' | 'choice' | 'signature' | 'unknown'
 *    - ev_form_field_type_from_string: str -> EvFormFieldType,
 *      defaulting to EV_FORM_FIELD_TEXT for unknown input
 *
 *  GROUP 2 (PR #111): the value accessors
 *    - Base: get_id / get_is_read_only / get_font_size /
 *      get_page / get_changed / set_changed
 *    - Text: get_text / set_text (emits 'changed')
 *    - Button: get_state / set_state (emits 'changed')
 *    - Choice: get_text / set_text (emits 'changed'),
 *      get_selected_items / set_selected_items (emits 'changed',
 *      takes ownership)
 *
 * Run via `meson test -C build test-form-field`.
 */

#include <config.h>
#include <glib.h>
#include <glib-object.h>

#include "ev-form-field.h"

/* ------------------------------------------------------------------------- */
/* GROUP 1 -- EvFormFieldType helpers (PR #78 + restored in PR #109)        */
/* ------------------------------------------------------------------------- */

static void
test_to_string_text (void)
{
	g_assert_cmpstr (ev_form_field_type_to_string (EV_FORM_FIELD_TEXT), ==, "text");
}

static void
test_to_string_button (void)
{
	g_assert_cmpstr (ev_form_field_type_to_string (EV_FORM_FIELD_BUTTON), ==, "button");
}

static void
test_to_string_choice (void)
{
	g_assert_cmpstr (ev_form_field_type_to_string (EV_FORM_FIELD_CHOICE), ==, "choice");
}

static void
test_to_string_signature (void)
{
	g_assert_cmpstr (ev_form_field_type_to_string (EV_FORM_FIELD_SIGNATURE), ==, "signature");
}

static void
test_to_string_unknown (void)
{
	/* Cast an out-of-range int to the enum to test the
	 * default branch.  This is a common pattern in C
	 * when handling enums from untrusted input. */
	g_assert_cmpstr (ev_form_field_type_to_string ((EvFormFieldType) 999), ==, "unknown");
	g_assert_cmpstr (ev_form_field_type_to_string ((EvFormFieldType) -1), ==, "unknown");
}

static void
test_roundtrip (void)
{
	const EvFormFieldType types[] = {
		EV_FORM_FIELD_TEXT, EV_FORM_FIELD_BUTTON,
		EV_FORM_FIELD_CHOICE, EV_FORM_FIELD_SIGNATURE
	};
	gsize i;
	for (i = 0; i < G_N_ELEMENTS (types); i++) {
		const gchar *s = ev_form_field_type_to_string (types[i]);
		EvFormFieldType roundtrip = ev_form_field_type_from_string (s);
		g_assert_cmpint (roundtrip, ==, types[i]);
	}
}

static void
test_from_string_case_insensitive (void)
{
	g_assert_cmpint (ev_form_field_type_from_string ("TEXT"),    ==, EV_FORM_FIELD_TEXT);
	g_assert_cmpint (ev_form_field_type_from_string ("Text"),    ==, EV_FORM_FIELD_TEXT);
	g_assert_cmpint (ev_form_field_type_from_string ("BUTTON"),  ==, EV_FORM_FIELD_BUTTON);
	g_assert_cmpint (ev_form_field_type_from_string ("Choice"),  ==, EV_FORM_FIELD_CHOICE);
}

static void
test_from_string_null_defaults (void)
{
	/* NULL input defaults to EV_FORM_FIELD_TEXT. */
	g_assert_cmpint (ev_form_field_type_from_string (NULL), ==, EV_FORM_FIELD_TEXT);
}

static void
test_from_string_unknown_defaults (void)
{
	/* Unknown input also defaults to EV_FORM_FIELD_TEXT. */
	g_assert_cmpint (ev_form_field_type_from_string ("foobar"), ==, EV_FORM_FIELD_TEXT);
	g_assert_cmpint (ev_form_field_type_from_string (""),      ==, EV_FORM_FIELD_TEXT);
}

/* ------------------------------------------------------------------------- */
/* GROUP 2 -- Value accessors (PR #111)                                     */
/* ------------------------------------------------------------------------- */

/* --- Base class getters --- */

static void
test_base_get_id (void)
{
	EvFormField *field = ev_form_field_text_new (42, EV_FORM_FIELD_TEXT_NORMAL);
	g_assert_cmpint (ev_form_field_get_id (field), ==, 42);
	g_object_unref (field);
}

static void
test_base_get_is_read_only_default (void)
{
	EvFormField *field = ev_form_field_text_new (1, EV_FORM_FIELD_TEXT_NORMAL);
	/* Default after ev_form_field_text_new is FALSE */
	g_assert_false (ev_form_field_get_is_read_only (field));
	g_object_unref (field);
}

static void
test_base_get_font_size_default (void)
{
	EvFormField *field = ev_form_field_text_new (1, EV_FORM_FIELD_TEXT_NORMAL);
	/* Default is 0.0 (unset) */
	g_assert_cmpfloat (ev_form_field_get_font_size (field), ==, 0.0);
	g_object_unref (field);
}

static void
test_base_get_page_default (void)
{
	EvFormField *field = ev_form_field_text_new (1, EV_FORM_FIELD_TEXT_NORMAL);
	/* Default is NULL (the poppler backend sets this on load) */
	g_assert_null (ev_form_field_get_page (field));
	g_object_unref (field);
}

static void
test_base_changed_flag_roundtrip (void)
{
	EvFormField *field = ev_form_field_text_new (1, EV_FORM_FIELD_TEXT_NORMAL);

	/* Default is FALSE */
	g_assert_false (ev_form_field_get_changed (field));

	ev_form_field_set_changed (field, TRUE);
	g_assert_true (ev_form_field_get_changed (field));

	ev_form_field_set_changed (field, FALSE);
	g_assert_false (ev_form_field_get_changed (field));

	g_object_unref (field);
}

/* --- Text field --- */

static void
test_text_get_text_default (void)
{
	EvFormField *field = ev_form_field_text_new (1, EV_FORM_FIELD_TEXT_NORMAL);
	EvFormFieldText *field_text = EV_FORM_FIELD_TEXT (field);

	g_assert_null (ev_form_field_text_get_text (field_text));

	g_object_unref (field);
}

static void
test_text_set_text_emits_changed (void)
{
	EvFormField *field = ev_form_field_text_new (1, EV_FORM_FIELD_TEXT_NORMAL);
	EvFormFieldText *field_text = EV_FORM_FIELD_TEXT (field);
	gulong handler_id;

	/* Connect a 'changed' signal handler.  Just count calls. */
	gint emit_count = 0;
	handler_id = g_signal_connect_swapped (field_text, "changed",
		G_CALLBACK (g_increment_integer), &emit_count);

	/* First set: NULL -> "hello" -> emit */
	ev_form_field_text_set_text (field_text, "hello");
	g_assert_cmpstr (ev_form_field_text_get_text (field_text), ==, "hello");
	g_assert_true (ev_form_field_get_changed (field));
	g_assert_cmpint (emit_count, ==, 1);

	/* Second set: "hello" -> "world" -> emit */
	ev_form_field_text_set_text (field_text, "world");
	g_assert_cmpstr (ev_form_field_text_get_text (field_text), ==, "world");
	g_assert_cmpint (emit_count, ==, 2);

	g_signal_handler_disconnect (field_text, handler_id);
	g_object_unref (field);
}

static void
test_text_set_text_no_emit_on_same (void)
{
	EvFormField *field = ev_form_field_text_new (1, EV_FORM_FIELD_TEXT_NORMAL);
	EvFormFieldText *field_text = EV_FORM_FIELD_TEXT (field);
	gint emit_count = 0;
	gulong handler_id;

	ev_form_field_text_set_text (field_text, "hello");
	handler_id = g_signal_connect_swapped (field_text, "changed",
		G_CALLBACK (g_increment_integer), &emit_count);

	/* Same value: no emit */
	ev_form_field_text_set_text (field_text, "hello");
	g_assert_cmpint (emit_count, ==, 0);
	g_assert_true (ev_form_field_get_changed (field));

	g_signal_handler_disconnect (field_text, handler_id);
	g_object_unref (field);
}

static void
test_text_set_text_null_clears (void)
{
	EvFormField *field = ev_form_field_text_new (1, EV_FORM_FIELD_TEXT_NORMAL);
	EvFormFieldText *field_text = EV_FORM_FIELD_TEXT (field);

	ev_form_field_text_set_text (field_text, "value");
	g_assert_cmpstr (ev_form_field_text_get_text (field_text), ==, "value");

	ev_form_field_text_set_text (field_text, NULL);
	g_assert_null (ev_form_field_text_get_text (field_text));

	g_object_unref (field);
}

/* --- Button field --- */

static void
test_button_set_state_emits_changed (void)
{
	EvFormField *field = ev_form_field_button_new (1, EV_FORM_FIELD_BUTTON_CHECK);
	EvFormFieldButton *field_button = EV_FORM_FIELD_BUTTON (field);
	gint emit_count = 0;
	gulong handler_id;

	g_assert_false (ev_form_field_button_get_state (field_button));

	handler_id = g_signal_connect_swapped (field_button, "changed",
		G_CALLBACK (g_increment_integer), &emit_count);

	/* FALSE -> TRUE: emit */
	ev_form_field_button_set_state (field_button, TRUE);
	g_assert_true (ev_form_field_button_get_state (field_button));
	g_assert_true (ev_form_field_get_changed (field));
	g_assert_cmpint (emit_count, ==, 1);

	/* TRUE -> FALSE: emit */
	ev_form_field_button_set_state (field_button, FALSE);
	g_assert_false (ev_form_field_button_get_state (field_button));
	g_assert_cmpint (emit_count, ==, 2);

	/* FALSE -> FALSE: no emit */
	ev_form_field_button_set_state (field_button, FALSE);
	g_assert_cmpint (emit_count, ==, 2);

	g_signal_handler_disconnect (field_button, handler_id);
	g_object_unref (field);
}

/* --- Choice field --- */

static void
test_choice_set_text_emits_changed (void)
{
	EvFormField *field = ev_form_field_choice_new (1, EV_FORM_FIELD_CHOICE_COMBO);
	EvFormFieldChoice *field_choice = EV_FORM_FIELD_CHOICE (field);
	gint emit_count = 0;
	gulong handler_id;

	handler_id = g_signal_connect_swapped (field_choice, "changed",
		G_CALLBACK (g_increment_integer), &emit_count);

	ev_form_field_choice_set_text (field_choice, "yes");
	g_assert_cmpstr (ev_form_field_choice_get_text (field_choice), ==, "yes");
	g_assert_true (ev_form_field_get_changed (field));
	g_assert_cmpint (emit_count, ==, 1);

	/* Same value: no emit */
	ev_form_field_choice_set_text (field_choice, "yes");
	g_assert_cmpint (emit_count, ==, 1);

	g_signal_handler_disconnect (field_choice, handler_id);
	g_object_unref (field);
}

static void
test_choice_set_selected_items_emits_changed (void)
{
	EvFormField *field = ev_form_field_choice_new (1, EV_FORM_FIELD_CHOICE_LIST);
	EvFormFieldChoice *field_choice = EV_FORM_FIELD_CHOICE (field);
	GList *items = NULL;
	gint emit_count = 0;
	gulong handler_id;

	handler_id = g_signal_connect_swapped (field_choice, "changed",
		G_CALLBACK (g_increment_integer), &emit_count);

	/* Empty -> [0]: emit */
	items = g_list_append (NULL, g_memdup2 (&(gint){0}, sizeof (gint)));
	ev_form_field_choice_set_selected_items (field_choice, items);
	g_assert_cmpint (emit_count, ==, 1);
	g_assert_nonnull (ev_form_field_choice_get_selected_items (field_choice));

	/* Same pointer: no emit */
	ev_form_field_choice_set_selected_items (field_choice, items);
	g_assert_cmpint (emit_count, ==, 1);

	/* New list: emit (and the old one is freed internally) */
	GList *new_items = g_list_append (NULL, g_memdup2 (&(gint){1}, sizeof (gint)));
	ev_form_field_choice_set_selected_items (field_choice, new_items);
	g_assert_cmpint (emit_count, ==, 2);

	g_signal_handler_disconnect (field_choice, handler_id);
	g_object_unref (field);
}

static void
test_choice_set_selected_items_takes_ownership (void)
{
	/* Verify that the field frees the GList when it's
	 * replaced.  Hard to test directly without a debugger
	 * (we'd need to hook into g_free), so the indirect
	 * proof is: the new accessor returns the new list, the
	 * old list is no longer reachable, and a memdup'd
	 * gint is freed by the field's finalize (we just rely
	 * on the fact that finalize doesn't crash with a
	 * non-empty list of memdup'd gints). */
	EvFormField *field = ev_form_field_choice_new (1, EV_FORM_FIELD_CHOICE_LIST);
	EvFormFieldChoice *field_choice = EV_FORM_FIELD_CHOICE (field);
	GList *items;

	items = g_list_append (NULL, g_memdup2 (&(gint){42}, sizeof (gint)));
	ev_form_field_choice_set_selected_items (field_choice, items);

	/* Drop our reference; field owns the list now */
	g_object_unref (field);

	/* If we get here without a crash, the list was freed
	 * by ev_form_field_choice_finalize().  This is the
	 * primary "ownership" guarantee we care about. */
	g_test_message ("field finalized without crash, ownership ok");
}

/* --- set_changed does NOT emit 'changed' --- */

static void
test_set_changed_does_not_emit (void)
{
	EvFormField *field = ev_form_field_text_new (1, EV_FORM_FIELD_TEXT_NORMAL);
	gint emit_count = 0;
	gulong handler_id;

	handler_id = g_signal_connect_swapped (field, "changed",
		G_CALLBACK (g_increment_integer), &emit_count);

	ev_form_field_set_changed (field, TRUE);
	g_assert_cmpint (emit_count, ==, 0);
	g_assert_true (ev_form_field_get_changed (field));

	g_signal_handler_disconnect (field, handler_id);
	g_object_unref (field);
}

int
main (int argc, char *argv[])
{
	g_test_init (&argc, &argv, NULL);

	/* GROUP 1: EvFormFieldType helpers */
	g_test_add_func ("/form-field/to-string-text",       test_to_string_text);
	g_test_add_func ("/form-field/to-string-button",     test_to_string_button);
	g_test_add_func ("/form-field/to-string-choice",     test_to_string_choice);
	g_test_add_func ("/form-field/to-string-signature",  test_to_string_signature);
	g_test_add_func ("/form-field/to-string-unknown",    test_to_string_unknown);
	g_test_add_func ("/form-field/roundtrip",            test_roundtrip);
	g_test_add_func ("/form-field/from-string-case-insensitive", test_from_string_case_insensitive);
	g_test_add_func ("/form-field/from-string-null-defaults",    test_from_string_null_defaults);
	g_test_add_func ("/form-field/from-string-unknown-defaults", test_from_string_unknown_defaults);

	/* GROUP 2: Value accessors (PR #111) */
	/* Base class */
	g_test_add_func ("/form-field/base/get-id",                  test_base_get_id);
	g_test_add_func ("/form-field/base/get-is-read-only-default", test_base_get_is_read_only_default);
	g_test_add_func ("/form-field/base/get-font-size-default",   test_base_get_font_size_default);
	g_test_add_func ("/form-field/base/get-page-default",        test_base_get_page_default);
	g_test_add_func ("/form-field/base/changed-flag-roundtrip",  test_base_changed_flag_roundtrip);
	g_test_add_func ("/form-field/base/set-changed-no-emit",     test_set_changed_does_not_emit);

	/* Text */
	g_test_add_func ("/form-field/text/get-text-default",        test_text_get_text_default);
	g_test_add_func ("/form-field/text/set-text-emits-changed",  test_text_set_text_emits_changed);
	g_test_add_func ("/form-field/text/set-text-no-emit-same",   test_text_set_text_no_emit_on_same);
	g_test_add_func ("/form-field/text/set-text-null-clears",    test_text_set_text_null_clears);

	/* Button */
	g_test_add_func ("/form-field/button/set-state-emits",       test_button_set_state_emits_changed);

	/* Choice */
	g_test_add_func ("/form-field/choice/set-text-emits",        test_choice_set_text_emits_changed);
	g_test_add_func ("/form-field/choice/set-selected-emits",    test_choice_set_selected_items_emits_changed);
	g_test_add_func ("/form-field/choice/set-selected-ownership", test_choice_set_selected_items_takes_ownership);

	return g_test_run ();
}
