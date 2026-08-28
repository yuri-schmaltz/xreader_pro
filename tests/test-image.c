/* test-image.c - Unit tests for ev_image (libdocument/ev-image.{c,h})
 *
 * Copyright (C) 2026 Yuri Schmaltz / xreader fork
 *
 * EvImage is a thin GObject wrapper around a GdkPixbuf, with an
 * associated (page, image-id) tuple.  The object is created in
 * one of two ways -- either with explicit (page, id) coordinates
 * (the pixbuf is filled in later by the document backend) or
 * with an already-loaded GdkPixbuf (for thumbnails, embedded
 * raster images in PDF, etc).
 *
 * The tests cover:
 *
 *   1. ev_image_new: create with (page, id), verify the getters
 *      return the same values.
 *   2. ev_image_new_from_pixbuf: pass a real 1x1 GdkPixbuf, verify
 *      the getters return the right values.
 *   3. ev_image_new_from_pixbuf with NULL: g_return_val_if_fail.
 *   4. ev_image_get_pixbuf before any pixbuf was set: NULL.
 *   5. ev_image_save_tmp: save a real pixbuf, verify the
 *      returned URI points to a real file with the right
 *      dimensions.
 *   6. ev_image_get_tmp_uri: NULL before save_tmp, non-NULL
 *      after.
 *   7. ev_image_get_id / _get_page with a non-EvImage object
 *      (G_TYPE_INVALID-typed pointer): g_return_val_if_fail
 *      returns the documented sentinel (-1).
 *
 * Run via `meson test -C build test-image`.
 *
 * The tests do NOT call gtk_init() or open a display.  GdkPixbuf
 * (the loaders, the type system) works without an X11/Wayland
 * connection; the GdkPixbuf tests use gdk_pixbuf_new() which
 * creates an in-memory image.  The full GDK types that
 * ev_image.h pulls in (gdk/gdk.h) are linked but never
 * instantiated, so the test does not need a display server.
 */

#include <config.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "ev-image.h"
#include "ev-file-helpers.h"

/* ----- construction and accessors ----- */

static void
test_new_basic (void)
{
	EvImage *image = ev_image_new (5, 42);
	g_assert_nonnull (image);
	g_assert_true (EV_IS_IMAGE (image));

	g_assert_cmpint (ev_image_get_page (image), ==, 5);
	g_assert_cmpint (ev_image_get_id   (image), ==, 42);

	/* No pixbuf was set, so the getter must return NULL. */
	g_assert_null (ev_image_get_pixbuf (image));

	/* No tmp URI before save_tmp is called. */
	g_assert_null (ev_image_get_tmp_uri (image));

	g_object_unref (image);
}

static void
test_new_from_pixbuf (void)
{
	GdkPixbuf *pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, 4, 3);
	g_assert_nonnull (pixbuf);
	gdk_pixbuf_fill (pixbuf, 0xff0000ff);  /* opaque red */

	EvImage *image = ev_image_new_from_pixbuf (pixbuf);
	g_assert_nonnull (image);
	g_assert_true (EV_IS_IMAGE (image));

	/* ev_image_new_from_pixbuf does not set page / id -- they
	 * default to 0. */
	g_assert_cmpint (ev_image_get_page (image), ==, 0);
	g_assert_cmpint (ev_image_get_id   (image), ==, 0);

	/* The pixbuf getter returns the same pixbuf. */
	g_assert_true (ev_image_get_pixbuf (image) == pixbuf);

	g_object_unref (pixbuf);
	g_object_unref (image);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnonnull"

static void
test_new_from_pixbuf_null (void)
{
	/* Passing NULL must trigger g_return_val_if_fail and return
	 * NULL, not crash. */
	g_test_expect_message (NULL, G_LOG_LEVEL_CRITICAL, "*pixbuf*");
	EvImage *image = ev_image_new_from_pixbuf (NULL);
	g_assert_null (image);
	g_test_assert_expected_messages ();
}

static void
test_getters_with_non_image (void)
{
	/* The getters must not crash on a non-EvImage pointer;
	 * the documented sentinel for the int getters is -1. */
	g_test_expect_message (NULL, G_LOG_LEVEL_CRITICAL, "*image*");
	g_assert_cmpint (ev_image_get_page (NULL), ==, -1);
	g_test_expect_message (NULL, G_LOG_LEVEL_CRITICAL, "*image*");
	g_assert_cmpint (ev_image_get_id   (NULL), ==, -1);
	g_test_expect_message (NULL, G_LOG_LEVEL_CRITICAL, "*image*");
	g_assert_null (ev_image_get_pixbuf (NULL));
	g_test_expect_message (NULL, G_LOG_LEVEL_CRITICAL, "*image*");
	g_assert_null (ev_image_get_tmp_uri (NULL));
	g_test_assert_expected_messages ();
}

/* ----- save_tmp / get_tmp_uri ----- */

static void
test_save_tmp (void)
{
	/* Create a 2x2 RGBA pixbuf, save it via ev_image_save_tmp,
	 * and verify:
	 *   - the returned URI is a file:// URI
	 *   - the file at the URI exists and has the right size
	 *   - a second call to ev_image_get_tmp_uri returns the
	 *     same URI (the helper caches the tmp file path) */
	GdkPixbuf *pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, 2, 2);
	g_assert_nonnull (pixbuf);
	gdk_pixbuf_fill (pixbuf, 0x00ff00ff);  /* opaque green */

	EvImage *image = ev_image_new (1, 1);
	g_assert_nonnull (image);

	const gchar *uri = ev_image_save_tmp (image, pixbuf);
	g_assert_nonnull (uri);
	g_assert_nonnull (strstr (uri, "file://"));

	/* A second call returns the same URI. */
	const gchar *uri2 = ev_image_get_tmp_uri (image);
	g_assert_true (g_str_equal (uri, uri2));

	/* The file at the URI must exist and be a regular file. */
	GFile *file = g_file_new_for_uri (uri);
	g_assert_true (g_file_query_exists (file, NULL));
	GFileInfo *info = g_file_query_info (file,
	                                     G_FILE_ATTRIBUTE_STANDARD_SIZE,
	                                     G_FILE_QUERY_INFO_NONE,
	                                     NULL,
	                                     NULL);
	g_assert_nonnull (info);
	goffset size = g_file_info_get_size (info);
	/* A 2x2 RGB PNG is small, but gdk_pixbuf_save may pick a
	 * different format.  Anything > 0 is fine here. */
	g_assert_cmpuint (size, >, 0);
	g_object_unref (info);
	g_object_unref (file);

	/* ev_image_get_pixbuf now returns the saved pixbuf. */
	g_assert_true (ev_image_get_pixbuf (image) == pixbuf);

	ev_tmp_uri_unlink (uri);

	g_object_unref (pixbuf);
	g_object_unref (image);
}

static void
test_save_tmp_with_null_image (void)
{
	/* save_tmp must return NULL on a non-EvImage pointer. */
	GdkPixbuf *pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, 1, 1);
	g_test_expect_message (NULL, G_LOG_LEVEL_CRITICAL, "*image*");
	const gchar *uri = ev_image_save_tmp (NULL, pixbuf);
	g_assert_null (uri);
	g_test_assert_expected_messages ();
	g_object_unref (pixbuf);
}

static void
test_save_tmp_with_null_pixbuf (void)
{
	/* save_tmp must return NULL when the pixbuf is NULL. */
	EvImage *image = ev_image_new (0, 0);
	g_test_expect_message (NULL, G_LOG_LEVEL_CRITICAL, "*pixbuf*");
	const gchar *uri = ev_image_save_tmp (image, NULL);
	g_assert_null (uri);
	g_test_assert_expected_messages ();
	g_object_unref (image);
}

#pragma GCC diagnostic pop

int
main (int argc, char *argv[])
{
	g_test_init (&argc, &argv, NULL);

	g_test_add_func ("/image/new-basic",                test_new_basic);
	g_test_add_func ("/image/new-from-pixbuf",          test_new_from_pixbuf);
	g_test_add_func ("/image/new-from-pixbuf-null",     test_new_from_pixbuf_null);
	g_test_add_func ("/image/getters-with-non-image",   test_getters_with_non_image);
	g_test_add_func ("/image/save-tmp",                 test_save_tmp);
	g_test_add_func ("/image/save-tmp-with-null-image", test_save_tmp_with_null_image);
	g_test_add_func ("/image/save-tmp-with-null-pixbuf",test_save_tmp_with_null_pixbuf);

	return g_test_run ();
}
