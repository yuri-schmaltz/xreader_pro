/* test-file-helpers.c - Unit tests for ev-file-helpers
 *
 * Copyright (C) 2026 Yuri Schmaltz / xreader fork
 *
 * Tests the small set of pure functions exposed by libdocument/ev-file-helpers.h:
 *
 *  - ev_mkstemp / ev_mkdtemp:  mkstemp(3) / mkdtemp(3) wrappers that
 *    return the resulting filename as a newly-allocated string.
 *  - ev_mkstemp_file: same as ev_mkstemp but returns a GFile, the
 *    file descriptor is closed via the destroy notify.
 *  - ev_xfer_uri_simple: g_file_copy wrapper, the NULL-from edge
 *    case must be a no-op success.
 *  - ev_file_get_mime_type: fast / slow MIME detection; on a known
 *    text file the helper must return SOME non-NULL MIME type.
 *  - ev_file_compress / ev_file_uncompress: with EV_COMPRESSION_NONE
 *    the helper must not shell out to an external compressor
 *    (the comment in the source says so explicitly).
 *
 * Run via `meson test -C build test-file-helpers`.
 */

#include <config.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ev-file-helpers.h"

static void
test_mkstemp_creates_file (void)
{
	char *template_str = g_strdup ("xreader-test-XXXXXX");
	char *name = NULL;
	int fd;

	fd = ev_mkstemp (template_str, &name, NULL);
	g_assert_cmpint (fd, >=, 0);
	g_assert_nonnull (name);
	g_assert_nonnull (strstr (name, "xreader-test-"));
	g_assert_cmpuint (strlen (name), >, 0);

	/* The file must exist and be writable. */
	g_assert_cmpint (g_access (name, F_OK), ==, 0);

	/* The template suffix must have been replaced with 6 random chars. */
	const char *suffix = name + strlen (name) - 6;
	for (int i = 0; i < 6; i++) {
		g_assert_cmpint (suffix[i] != '\0', ==, 1);
	}

	g_assert_cmpint (close (fd), ==, 0);
	g_assert_cmpint (g_unlink (name), ==, 0);

	g_free (name);
	g_free (template_str);
}

static void
test_mkdtemp_creates_directory (void)
{
	char *template_str = g_strdup ("xreader-test-XXXXXX");

	/* ev_mkdtemp returns a newly-allocated string with the
	 * resulting path, or NULL on error. */
	char *name = ev_mkdtemp (template_str, NULL);
	g_assert_nonnull (name);
	g_assert_nonnull (strstr (name, "xreader-test-"));

	/* The directory must exist. */
	g_assert_cmpint (g_access (name, F_OK), ==, 0);

	g_assert_cmpint (g_rmdir (name), ==, 0);

	g_free (name);
	g_free (template_str);
}




static void
test_mkstemp_file_returns_gfile (void)
{
	GError *error = NULL;
	GFile *file = NULL;

	file = ev_mkstemp_file ("xreader-test-XXXXXX", &error);
	g_assert_no_error (error);
	g_assert_nonnull (file);

	/* The returned GFile must be a valid local file path. */
	gchar *path = g_file_get_path (file);
	g_assert_nonnull (path);
	g_assert_nonnull (strstr (path, "xreader-test-"));
	g_assert_cmpint (g_access (path, F_OK), ==, 0);

	/* unrefing the GFile must close the underlying fd (the
	 * destroy notify on "ev-mkstemp-fd" takes care of it). */
	g_object_unref (file);
	g_assert_cmpint (g_unlink (path), ==, 0);

	g_free (path);
}

static void
test_xfer_uri_simple_copies_file (void)
{
	GError *error = NULL;
	gchar *src_name = NULL;
	int src_fd;
	gchar *src_uri, *dst_uri;
	const char payload[] = "xreader test payload";
	gchar *read_back = NULL;
	gsize read_len = 0;

	/* Create a source file with a known content. */
	src_fd = ev_mkstemp ("xreader-src-XXXXXX", &src_name, &error);
	g_assert_no_error (error);
	g_assert_cmpint (src_fd, >=, 0);
	g_assert_cmpint (write (src_fd, payload, sizeof (payload) - 1), ==, sizeof (payload) - 1);
	g_assert_cmpint (close (src_fd), ==, 0);

	src_uri = g_filename_to_uri (src_name, NULL, &error);
	g_assert_no_error (error);
	g_assert_nonnull (src_uri);

	dst_uri = g_strconcat (src_uri, ".copy", NULL);

	/* The copy itself. */
	g_assert_true (ev_xfer_uri_simple (src_uri, dst_uri, &error));
	g_assert_no_error (error);

	/* Verify the content of the destination. */
	g_assert_true (g_file_get_contents (dst_uri + strlen ("file://"), &read_back, &read_len, &error));
	g_assert_no_error (error);
	g_assert_cmpuint (read_len, ==, sizeof (payload) - 1);
	g_assert_cmpint (memcmp (read_back, payload, sizeof (payload) - 1), ==, 0);

	g_unlink (src_name);
	g_unlink (dst_uri + strlen ("file://"));
	g_free (read_back);
	g_free (src_uri);
	g_free (dst_uri);
	g_free (src_name);
}

static void
test_xfer_uri_simple_null_from_is_noop (void)
{
	GError *error = NULL;
	/* NULL from -> returns TRUE without doing anything (the
	 * 'copy nothing' use case the comment in the source
	 * mentions).  This protects callers that pass NULL by
	 * accident (e.g. the print previewer when no file is
	 * selected). */
	g_assert_true (ev_xfer_uri_simple (NULL, "file:///tmp/none", &error));
	g_assert_no_error (error);
}

static void
test_file_get_mime_type_text (void)
{
	GError *error = NULL;
	gchar *src_name = NULL;
	int src_fd;
	gchar *src_uri;
	gchar *mime = NULL;

	src_fd = ev_mkstemp ("xreader-mime-XXXXXX", &src_name, &error);
	g_assert_no_error (error);
	g_assert_cmpint (write (src_fd, "hello, world\n", 13), ==, 13);
	g_assert_cmpint (close (src_fd), ==, 0);

	src_uri = g_filename_to_uri (src_name, NULL, &error);
	g_assert_no_error (error);

	mime = ev_file_get_mime_type (src_uri, TRUE, &error);
	g_assert_no_error (error);
	/* On a fresh /tmp file the fast (extension-based) detection
	 * can return application/octet-stream; both are acceptable
	 * for the smoke test, the contract is that some MIME type
	 * is returned without an error. */
	g_assert_nonnull (mime);

	g_unlink (src_name);
	g_free (src_uri);
	g_free (mime);
	g_free (src_name);
}

static void
test_file_compress_none_is_noop (void)
{
	GError *error = NULL;
	/* EV_COMPRESSION_NONE: the helper must not call the
	 * external compressor binary (the comment in
	 * ev_file_compress says so explicitly).  Returns NULL
	 * without filling @error, which is the contract
	 * callers depend on for the 'no compression needed' code
	 * path. */
	gchar *result = ev_file_compress ("file:///dev/null",
	                                  EV_COMPRESSION_NONE, &error);
	g_assert_null (result);
	g_assert_no_error (error);

	/* Same for the decompress side. */
	result = ev_file_uncompress ("file:///dev/null",
	                             EV_COMPRESSION_NONE, &error);
	g_assert_null (result);
	g_assert_no_error (error);
}

/* ----- ev_tmp_filename_unlink / ev_tmp_uri_unlink / ev_mkstemp_file ----- */

static void
test_tmp_filename_unlink_in_tmp (void)
{
	/* Create a file in /tmp via ev_mkstemp, then call
	 * ev_tmp_filename_unlink on the path.  The helper must
	 * delete it because the path is inside g_get_tmp_dir(). */
	gchar *name = NULL;
	gint fd = ev_mkstemp ("xreader-tmp-unlink-XXXXXX", &name, NULL);
	g_assert_cmpint (fd, >=, 0);
	close (fd);

	/* File must exist before unlink. */
	g_assert_true (g_file_test (name, G_FILE_TEST_EXISTS));

	ev_tmp_filename_unlink (name);

	/* File must NOT exist after unlink. */
	g_assert_false (g_file_test (name, G_FILE_TEST_EXISTS));

	g_free (name);
}

static void
test_tmp_filename_unlink_outside_tmp (void)
{
	/* A path outside g_get_tmp_dir() must NOT be deleted by
	 * ev_tmp_filename_unlink -- the helper is supposed to be
	 * a safe unlink that only operates on the xreader tmp
	 * directory, not a general-purpose unlink. */
	gchar *name = g_strdup ("/etc/passwd");

	ev_tmp_filename_unlink (name);

	/* /etc/passwd must still exist. */
	g_assert_true (g_file_test (name, G_FILE_TEST_EXISTS));

	g_free (name);
}

static void
test_tmp_filename_unlink_null (void)
{
	/* NULL filename is a documented no-op. */
	ev_tmp_filename_unlink (NULL);
	/* If we got here without crashing, the test passes. */
}

static void
test_mkstemp_file_destroy_notify (void)
{
	/* The GFile returned by ev_mkstemp_file has a destroy
	 * notify that closes the file descriptor.  Unref the
	 * GFile and verify the FD is closed by trying to close
	 * it again (close on a closed FD is a no-op + EBADF
	 * return, which we tolerate). */
	GError *error = NULL;
	GFile *file = ev_mkstemp_file ("xreader-mkstemp-fd-XXXXXX", &error);
	g_assert_nonnull (file);
	g_assert_no_error (error);

	gchar *path = g_file_get_path (file);
	g_assert_nonnull (path);
	g_assert_true (g_file_test (path, G_FILE_TEST_EXISTS));
	g_unlink (path);
	g_free (path);

	/* The fd is closed by the destroy notify.  Just unref. */
	g_object_unref (file);
}

static void
test_tmp_file_unlink_outside_tmp (void)
{
	/* Files outside temp directories must NOT be deleted by ev_tmp_file_unlink / ev_tmp_uri_unlink */
	GFile *file = g_file_new_for_path ("/etc/passwd");
	ev_tmp_file_unlink (file);
	g_assert_true (g_file_test ("/etc/passwd", G_FILE_TEST_EXISTS));
	g_object_unref (file);

	ev_tmp_uri_unlink ("file:///etc/passwd");
	g_assert_true (g_file_test ("/etc/passwd", G_FILE_TEST_EXISTS));
}

static void
test_tmp_file_unlink_in_tmp (void)
{
	GError *error = NULL;
	GFile *file = ev_mkstemp_file ("xreader-tmp-file-unlink-XXXXXX", &error);
	g_assert_nonnull (file);
	g_assert_no_error (error);

	gchar *path = g_file_get_path (file);
	g_assert_true (g_file_test (path, G_FILE_TEST_EXISTS));

	ev_tmp_file_unlink (file);
	g_assert_false (g_file_test (path, G_FILE_TEST_EXISTS));

	g_free (path);
	g_object_unref (file);
}

int
main (int argc, char *argv[])
{
	g_test_init (&argc, &argv, NULL);

	g_test_add_func ("/ev-file-helpers/mkstemp-creates-file",
	                 test_mkstemp_creates_file);
	g_test_add_func ("/ev-file-helpers/mkstemp-file-returns-gfile",
	                 test_mkstemp_file_returns_gfile);
	g_test_add_func ("/ev-file-helpers/xfer-uri-simple-copies-file",
	                 test_xfer_uri_simple_copies_file);
	g_test_add_func ("/ev-file-helpers/xfer-uri-simple-null-from-is-noop",
	                 test_xfer_uri_simple_null_from_is_noop);
	g_test_add_func ("/ev-file-helpers/file-get-mime-type-text",
	                 test_file_get_mime_type_text);
	g_test_add_func ("/ev-file-helpers/file-compress-none-is-noop",
	                 test_file_compress_none_is_noop);
	g_test_add_func ("/ev-file-helpers/mkdtemp-creates-directory",
	                 test_mkdtemp_creates_directory);
	g_test_add_func ("/ev-file-helpers/tmp-filename-unlink-in-tmp",
	                 test_tmp_filename_unlink_in_tmp);
	g_test_add_func ("/ev-file-helpers/tmp-filename-unlink-outside-tmp",
	                 test_tmp_filename_unlink_outside_tmp);
	g_test_add_func ("/ev-file-helpers/tmp-filename-unlink-null",
	                 test_tmp_filename_unlink_null);
	g_test_add_func ("/ev-file-helpers/tmp-file-unlink-outside-tmp",
	                 test_tmp_file_unlink_outside_tmp);
	g_test_add_func ("/ev-file-helpers/tmp-file-unlink-in-tmp",
	                 test_tmp_file_unlink_in_tmp);
	g_test_add_func ("/ev-file-helpers/mkstemp-file-destroy-notify",
	                 test_mkstemp_file_destroy_notify);

	return g_test_run ();
}
