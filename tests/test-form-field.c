/* test-form-field.c - Unit tests for the EvFormField helpers
 *                     added in PR #78 + restored in PR #109.
 *
 * Copyright (C) 2026 Yuri Schmaltz / xreader fork
 *
 * Tests the two helpers:
 *
 *  - ev_form_field_type_to_string: EvFormFieldType ->
 *    'text' | 'button' | 'choice' | 'signature' | 'unknown'
 *  - ev_form_field_type_from_string: str -> EvFormFieldType,
 *    defaulting to EV_FORM_FIELD_TEXT for unknown input
 *
 * Round-trips are tested for all 4 known types, plus the
 * case-insensitive parsing and the NULL-input edge case.
 *
 * The EvFormFieldType enum was added in PR #109 because the
 * original PR #78 implementation used a typedef that was
 * never declared; the build fix in commit 1612325 removed
 * the helpers entirely.  PR #109 restores the feature with
 * a properly-declared enum.
 *
 * Run via `meson test -C build test-form-field`.
 */

#include <config.h>
#include <glib.h>

#include "ev-form-field.h"

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

int
main (int argc, char *argv[])
{
	g_test_init (&argc, &argv, NULL);

	g_test_add_func ("/form-field/to-string-text",       test_to_string_text);
	g_test_add_func ("/form-field/to-string-button",     test_to_string_button);
	g_test_add_func ("/form-field/to-string-choice",     test_to_string_choice);
	g_test_add_func ("/form-field/to-string-signature",  test_to_string_signature);
	g_test_add_func ("/form-field/to-string-unknown",    test_to_string_unknown);
	g_test_add_func ("/form-field/roundtrip",            test_roundtrip);
	g_test_add_func ("/form-field/from-string-case-insensitive", test_from_string_case_insensitive);
	g_test_add_func ("/form-field/from-string-null-defaults",    test_from_string_null_defaults);
	g_test_add_func ("/form-field/from-string-unknown-defaults", test_from_string_unknown_defaults);

	return g_test_run ();
}
