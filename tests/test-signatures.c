/* test-signatures.c - GLib Testing unit test for EvDocumentSignatures and EvSignature */

#include "config.h"
#include <glib.h>
#include "ev-document-signatures.h"

static void
test_signature_lifecycle (void)
{
	EvSignature *sig = ev_signature_new ();
	g_assert_nonnull (sig);
	g_assert_cmpint (sig->status, ==, EV_SIGNATURE_STATUS_UNKNOWN);
	g_assert_cmpint (sig->page, ==, -1);

	sig->signer_name = g_strdup ("Test Signer");
	sig->reason = g_strdup ("Approval");
	sig->location = g_strdup ("Brasilia");
	sig->status = EV_SIGNATURE_STATUS_VALID;
	sig->page = 1;

	EvSignature *copy = ev_signature_copy (sig);
	g_assert_nonnull (copy);
	g_assert_cmpstr (copy->signer_name, ==, "Test Signer");
	g_assert_cmpstr (copy->reason, ==, "Approval");
	g_assert_cmpstr (copy->location, ==, "Brasilia");
	g_assert_cmpint (copy->status, ==, EV_SIGNATURE_STATUS_VALID);
	g_assert_cmpint (copy->page, ==, 1);

	ev_signature_free (copy);
	ev_signature_free (sig);
}

static void
test_signature_status_strings (void)
{
	g_assert_nonnull (ev_signature_status_to_string (EV_SIGNATURE_STATUS_VALID));
	g_assert_nonnull (ev_signature_status_to_string (EV_SIGNATURE_STATUS_INVALID));
	g_assert_nonnull (ev_signature_status_to_string (EV_SIGNATURE_STATUS_DIGEST_MISMATCH));
	g_assert_nonnull (ev_signature_status_to_string (EV_SIGNATURE_STATUS_UNTRUSTED));
	g_assert_nonnull (ev_signature_status_to_string (EV_SIGNATURE_STATUS_UNKNOWN));
}

int
main (int argc, char *argv[])
{
	g_test_init (&argc, &argv, NULL);

	g_test_add_func ("/signatures/lifecycle", test_signature_lifecycle);
	g_test_add_func ("/signatures/status-strings", test_signature_status_strings);

	return g_test_run ();
}
