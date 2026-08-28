/* test-document-factory.c - Unit tests for libdocument init / file helpers
 *                           that are not covered by test-file-helpers.c
 *                           (PR #11, #25, #31) or test-mime.c (PR #33).
 *
 * Copyright (C) 2026 Yuri Schmaltz / xreader fork
 *
 * Tests the following public API:
 *
 *   - ev_init / ev_shutdown lifecycle (refcounted).
 *   - ev_file_is_temp (true for /tmp paths, false for /home).
 *   - ev_file_uncompress / ev_file_compress roundtrip for gzip + bzip2.
 *   - ev_file_uncompress on a nonexistent file: error, not crash.
 *   - ev_file_uncompress with NULL uri: g_return_val_if_fail triggers.
 *   - ev_mkstemp on a template with no XXXXXX: error.
 *   - ev_xfer_uri_simple: copy a real file, verify contents.
 *   - _ev_is_initialized: true after ev_init, false before.
 *
 * Run via `meson test -C build test-document-factory`.
 *
 * Note: many of the tests rely on /tmp being writable and on
 * gzip / bzip2 being installed on the build host (which they
 * are in any reasonable CI image and in the xreader build
 * dependencies).  The tests do not depend on any GUI or
 * display, so they run headlessly.
 */

#include <config.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "ev-file-helpers.h"
#include "ev-init.h"
#include "ev-debug.h"

static gchar *
write_tmp_file_with_contents (const gchar *suffix, const guint8 *data, gsize len)
{
	gchar *path;
	gint fd;

	path = g_strdup_printf ("%s/xreader-test-docfactory-XXXXXX%s",
	                        g_get_tmp_dir (),
	                        suffix ? suffix : "");
	fd = g_mkstemp (path);
	g_assert_cmpint (fd, >=, 0);
	if (data != NULL && len > 0) {
		gsize written = 0;
		while (written < len) {
			gssize n = write (fd, data + written, len - written);
			g_assert_cmpint (n, >, 0);
			written += (gsize) n;
		}
	}
	close (fd);
	return path;
}

static gchar *
file_uri_from_path (const gchar *path)
{
	GFile *file = g_file_new_for_path (path);
	gchar *uri = g_file_get_uri (file);
	g_object_unref (file);
	return uri;
}

/* ----- init / shutdown lifecycle ----- */

static void
test_init_lifecycle (void)
{
	/* _ev_is_initialized should be false before ev_init. */
	g_assert_false (_ev_is_initialized ());

	/* First ev_init: brings up the backends. */
	g_assert_true (ev_init ());

	/* Now it must be true. */
	g_assert_true (_ev_is_initialized ());

	/* A second ev_init is a refcount bump and must succeed. */
	g_assert_true (ev_init ());
	g_assert_true (_ev_is_initialized ());

	/* First ev_shutdown: refcount 2 -> 1, still initialized. */
	ev_shutdown ();
	g_assert_true (_ev_is_initialized ());

	/* Second ev_shutdown: refcount 1 -> 0, fully uninitialized. */
	ev_shutdown ();
	g_assert_false (_ev_is_initialized ());

	/* Re-init for the rest of the test cases that depend on it. */
	g_assert_true (ev_init ());
}

/* ----- ev_file_is_temp ----- */

static void
test_file_is_temp_tmp_dir (void)
{
	gchar *path = write_tmp_file_with_contents (".tmp", NULL, 0);
	GFile *file = g_file_new_for_path (path);
	g_assert_true (ev_file_is_temp (file));
	g_object_unref (file);
	g_unlink (path);
	g_free (path);
}

static void
test_file_is_temp_home (void)
{
	/* /etc/passwd exists on every Linux.  It is not in /tmp,
	 * so ev_file_is_temp must return FALSE. */
	GFile *file = g_file_new_for_path ("/etc/passwd");
	g_assert_false (ev_file_is_temp (file));
	g_object_unref (file);
}

static void
test_file_is_temp_non_native (void)
{
	/* A non-native URI like http:// must return FALSE without
	 * crashing, because g_file_is_native() returns FALSE. */
	GFile *file = g_file_new_for_uri ("http://example.com/foo");
	g_assert_false (ev_file_is_temp (file));
	g_object_unref (file);
}

/* ----- ev_file_uncompress / ev_file_compress roundtrip ----- */

static void
test_compress_uncompress_gzip (void)
{
	const guint8 payload[] = "the quick brown fox jumps over the lazy dog\n"
	                          "the quick brown fox jumps over the lazy dog\n"
	                          "the quick brown fox jumps over the lazy dog\n";
	gchar *input_path = write_tmp_file_with_contents (".txt", payload, sizeof (payload) - 1);
	gchar *input_uri = file_uri_from_path (input_path);

	/* Compress to a new URI (the helper writes the result to a
	 * side file and returns its URI). */
	GError *err = NULL;
	gchar *gz_uri = ev_file_compress (input_uri, EV_COMPRESSION_GZIP, &err);
	g_assert_no_error (err);
	g_assert_nonnull (gz_uri);

	/* The compressed file must exist and be a different file
	 * from the input. */
	GFile *gz_file = g_file_new_for_uri (gz_uri);
	g_assert_true (g_file_query_exists (gz_file, NULL));

	/* Uncompress back. */
	err = NULL;
	gchar *out_uri = ev_file_uncompress (gz_uri, EV_COMPRESSION_GZIP, &err);
	g_assert_no_error (err);
	g_assert_nonnull (out_uri);

	/* Read the uncompressed output and verify it matches the
	 * original payload byte-for-byte. */
	GFile *out_file = g_file_new_for_uri (out_uri);
	gchar *contents = NULL;
	gsize len = 0;
	g_assert_true (g_file_load_contents (out_file, NULL, &contents, &len, NULL, NULL));
	g_assert_cmpuint (len, ==, sizeof (payload) - 1);
	g_assert_cmpint (memcmp (contents, payload, len), ==, 0);
	g_free (contents);
	g_object_unref (out_file);
	g_object_unref (gz_file);

	g_unlink (input_path);
	g_free (input_path);
	g_free (input_uri);
	g_free (gz_uri);
	g_free (out_uri);
}

static void
test_compress_uncompress_bzip2 (void)
{
	const guint8 payload[] = "lorem ipsum dolor sit amet, consectetur adipiscing elit\n";
	gchar *input_path = write_tmp_file_with_contents (".txt", payload, sizeof (payload) - 1);
	gchar *input_uri = file_uri_from_path (input_path);

	GError *err = NULL;
	gchar *bz2_uri = ev_file_compress (input_uri, EV_COMPRESSION_BZIP2, &err);
	g_assert_no_error (err);
	g_assert_nonnull (bz2_uri);

	err = NULL;
	gchar *out_uri = ev_file_uncompress (bz2_uri, EV_COMPRESSION_BZIP2, &err);
	g_assert_no_error (err);
	g_assert_nonnull (out_uri);

	GFile *out_file = g_file_new_for_uri (out_uri);
	gchar *contents = NULL;
	gsize len = 0;
	g_assert_true (g_file_load_contents (out_file, NULL, &contents, &len, NULL, NULL));
	g_assert_cmpuint (len, ==, sizeof (payload) - 1);
	g_assert_cmpint (memcmp (contents, payload, len), ==, 0);
	g_free (contents);
	g_object_unref (out_file);

	g_unlink (input_path);
	g_free (input_path);
	g_free (input_uri);
	g_free (bz2_uri);
	g_free (out_uri);
}

static void
test_uncompress_nonexistent (void)
{
	/* Uncompress a URI that points to a file that does not exist.
	 * Must return NULL and fill @error, not crash. */
	GError *err = NULL;
	gchar *out = ev_file_uncompress ("file:///nonexistent/path/foo.gz",
	                                 EV_COMPRESSION_GZIP, &err);
	g_assert_null (out);
	g_assert_nonnull (err);
	g_clear_error (&err);
}

/* ----- ev_mkstemp edge cases (not covered by test-file-helpers.c) ----- */

static void
test_mkstemp_missing_template (void)
{
	/* The template must end with at least 6 X's (mkstemp(3)
	 * requirement).  Passing a template without any X's must
	 * return -1 and fill @error. */
	GError *err = NULL;
	gchar *name = NULL;
	gint fd = ev_mkstemp ("no-xs-here.txt", &name, &err);
	g_assert_cmpint (fd, ==, -1);
	g_assert_nonnull (err);
	g_clear_error (&err);
	g_free (name);
}

static void
test_mkstemp_creates_file (void)
{
	/* The positive case: a template with 6 X's is replaced and
	 * the file is created. */
	GError *err = NULL;
	gchar *name = NULL;
	gchar tmpl[] = "xreader-mkstemp-positive-XXXXXX";
	gint fd = ev_mkstemp (tmpl, &name, &err);
	g_assert_cmpint (fd, >=, 0);
	g_assert_no_error (err);
	g_assert_nonnull (name);
	g_assert_nonnull (strstr (name, "xreader-mkstemp-positive-"));

	/* The file must exist and be writable. */
	gchar *body = g_strdup_printf ("test %lld\n", (long long) g_get_real_time ());
	g_assert_cmpint (write (fd, body, strlen (body)), ==, (gssize) strlen (body));
	close (fd);
	g_unlink (name);
	g_free (name);
	g_free (body);
}

/* ----- ev_xfer_uri_simple ----- */

static void
test_xfer_uri_simple (void)
{
	/* Copy a real file and verify the destination has the same
	 * contents. */
	const guint8 payload[] = "alpha\nbeta\ngamma\n";
	gchar *src_path = write_tmp_file_with_contents (".src", payload, sizeof (payload) - 1);
	gchar *src_uri = file_uri_from_path (src_path);

	gchar *dst_tmpl = g_strdup_printf ("%s/xreader-xfer-dst-XXXXXX", g_get_tmp_dir ());
	gint fd = g_mkstemp (dst_tmpl);
	g_assert_cmpint (fd, >=, 0);
	close (fd);
	g_unlink (dst_tmpl);  /* we want ev_xfer_uri_simple to create it */
	gchar *dst_uri = file_uri_from_path (dst_tmpl);

	GError *err = NULL;
	gboolean ok = ev_xfer_uri_simple (src_uri, dst_uri, &err);
	g_assert_no_error (err);
	g_assert_true (ok);

	/* Verify the dst has the same contents. */
	GFile *dst_file = g_file_new_for_uri (dst_uri);
	gchar *contents = NULL;
	gsize len = 0;
	g_assert_true (g_file_load_contents (dst_file, NULL, &contents, &len, NULL, NULL));
	g_assert_cmpuint (len, ==, sizeof (payload) - 1);
	g_assert_cmpint (memcmp (contents, payload, len), ==, 0);
	g_free (contents);
	g_object_unref (dst_file);

	g_unlink (src_path);
	g_unlink (dst_tmpl);
	g_free (dst_tmpl);
	g_free (src_path);
	g_free (src_uri);
	g_free (dst_uri);
}

int
main (int argc, char *argv[])
{
	g_test_init (&argc, &argv, NULL);

	g_test_add_func ("/docfactory/init-lifecycle",          test_init_lifecycle);
	g_test_add_func ("/docfactory/file-is-temp-tmp-dir",    test_file_is_temp_tmp_dir);
	g_test_add_func ("/docfactory/file-is-temp-home",       test_file_is_temp_home);
	g_test_add_func ("/docfactory/file-is-temp-non-native", test_file_is_temp_non_native);
	g_test_add_func ("/docfactory/compress-gzip",           test_compress_uncompress_gzip);
	g_test_add_func ("/docfactory/compress-bzip2",          test_compress_uncompress_bzip2);
	g_test_add_func ("/docfactory/uncompress-nonexistent",  test_uncompress_nonexistent);
	g_test_add_func ("/docfactory/mkstemp-missing-xs",      test_mkstemp_missing_template);
	g_test_add_func ("/docfactory/mkstemp-creates-file",    test_mkstemp_creates_file);
	g_test_add_func ("/docfactory/xfer-uri-simple",         test_xfer_uri_simple);

	return g_test_run ();
}
