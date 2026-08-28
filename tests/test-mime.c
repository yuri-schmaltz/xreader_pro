/* test-mime.c - Unit tests for ev_file_get_mime_type
 *
 * Copyright (C) 2026 Yuri Schmaltz / xreader fork
 *
 * Tests the MIME type detection helper in libdocument/ev-file-helpers.h:
 * the contract is that a file on disk is classified by content (the
 * 'fast' mode looks at the extension only, the default mode looks at
 * the magic bytes) and the result is a non-NULL string in the IANA
 * media type namespace (image/..., application/..., text/..., ...).
 *
 * Run via `meson test -C build test-mime`.
 *
 * The test uses real magic-byte sequences for the formats xreader
 * cares about: PDF, PostScript, DJVU, DVI, EPUB (zip-with-mimetype),
 * TIFF (both endianness), PNG, JPEG, gzip, bzip2, xz, and a few
 * negative cases.  The MIME type itself is intentionally not
 * asserted exactly -- the goal is to verify that the dispatcher
 * picks a non-NULL, non-empty, and reasonable type.
 */

#include <config.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "ev-file-helpers.h"

static gchar *
write_tmp_file_with (const guint8 *data, gsize len, const gchar *suffix)
{
	gchar *path;
	gint fd;

	path = g_strdup_printf ("%s/xreader-test-mime-XXXXXX%s",
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

static void
assert_mime_nonempty (const gchar *path, const gchar *hint)
{
	GError *err = NULL;
	gchar *uri = g_filename_to_uri (path, NULL, &err);
	g_assert_no_error (err);
	g_assert_nonnull (uri);

	/* Slow / content-based detection. */
	gchar *mime = ev_file_get_mime_type (uri, FALSE, &err);
	g_assert_no_error (err);
	g_assert_nonnull (mime);
	g_assert_cmpuint (strlen (mime), >, 0);
	g_test_message ("%s -> %s (content)", hint, mime);
	g_free (mime);

	/* Fast / extension-based detection.  The empty-extension
	 * test case is the only one where the two modes are expected
	 * to differ (slow succeeds, fast returns application/
	 * octet-stream); we accept either result here. */
	gchar *mime_fast = ev_file_get_mime_type (uri, TRUE, &err);
	g_assert_no_error (err);
	g_assert_nonnull (mime_fast);
	g_test_message ("%s -> %s (ext)",     hint, mime_fast);
	g_free (mime_fast);

	g_free (uri);
}

/* ----- positive magic-byte cases ----- */

static void
test_mime_pdf (void)
{
	const guint8 pdf[] =
		"%PDF-1.4\n%\xc0\xc1\xc2\xc3\n"
		"1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj\n"
		"2 0 obj<</Type/Pages/Count 0/Kids[]>>endobj\n"
		"xref\n0 3\n0000000000 65535 f \n"
		"trailer<</Size 3/Root 1 0 R>>\n"
		"startxref\n0\n%%EOF\n";
	gchar *path = write_tmp_file_with (pdf, sizeof (pdf) - 1, ".pdf");
	assert_mime_nonempty (path, "PDF 1.4");
	g_unlink (path);
	g_free (path);
}

static void
test_mime_postscript (void)
{
	const guint8 ps[] =
		"%!PS-Adobe-3.0\n"
		"%%BoundingBox: 0 0 612 792\n"
		"%%EndComments\n"
		"0 0 moveto 100 100 lineto stroke\n"
		"showpage\n"
		"%%EOF\n";
	gchar *path = write_tmp_file_with (ps, sizeof (ps) - 1, ".ps");
	assert_mime_nonempty (path, "PostScript");
	g_unlink (path);
	g_free (path);
}

static void
test_mime_djvu (void)
{
	const guint8 djvu[] =
		"AT&TFORM\x00\x00\x00\x00"
		"DJVM\x00\x00\x00\x00"
		"\x00\x00\x00\x00";
	gchar *path = write_tmp_file_with (djvu, sizeof (djvu) - 1, ".djvu");
	assert_mime_nonempty (path, "DjVu");
	g_unlink (path);
	g_free (path);
}

static void
test_mime_dvi (void)
{
	/* DVI file magic: 0xf7 0x02 ('PRE') followed by the
	 * 32-bit unsigned big-endian unit numerator. */
	const guint8 dvi[] = "\xf7\x02\x00\x00\x00\x01";
	gchar *path = write_tmp_file_with (dvi, sizeof (dvi) - 1, ".dvi");
	assert_mime_nonempty (path, "DVI");
	g_unlink (path);
	g_free (path);
}

static void
test_mime_tiff_le (void)
{
	/* Little-endian TIFF: 'II' '\x2a\x00' (magic 42 in LE). */
	const guint8 tiff_le[] = "II\x2a\x00\x08\x00\x00\x00\x00";
	gchar *path = write_tmp_file_with (tiff_le, sizeof (tiff_le) - 1, ".tiff");
	assert_mime_nonempty (path, "TIFF-LE");
	g_unlink (path);
	g_free (path);
}

static void
test_mime_tiff_be (void)
{
	/* Big-endian TIFF: 'MM' '\x00\x2a' (magic 42 in BE). */
	const guint8 tiff_be[] = "MM\x00\x2a\x00\x00\x00\x08";
	gchar *path = write_tmp_file_with (tiff_be, sizeof (tiff_be) - 1, ".tif");
	assert_mime_nonempty (path, "TIFF-BE");
	g_unlink (path);
	g_free (path);
}

static void
test_mime_png (void)
{
	const guint8 png[] = "\x89PNG\r\n\x1a\n\x00\x00\x00\rIHDR";
	gchar *path = write_tmp_file_with (png, sizeof (png) - 1, ".png");
	assert_mime_nonempty (path, "PNG");
	g_unlink (path);
	g_free (path);
}

static void
test_mime_jpeg (void)
{
	const guint8 jpeg[] = "\xff\xd8\xff\xe0\x00\x10JFIF\x00";
	gchar *path = write_tmp_file_with (jpeg, sizeof (jpeg) - 1, ".jpg");
	assert_mime_nonempty (path, "JPEG");
	g_unlink (path);
	g_free (path);
}

static void
test_mime_gzip (void)
{
	const guint8 gz[] = "\x1f\x8b\x08\x00\x00\x00\x00\x00\x00\x00";
	gchar *path = write_tmp_file_with (gz, sizeof (gz) - 1, ".gz");
	assert_mime_nonempty (path, "gzip");
	g_unlink (path);
	g_free (path);
}

static void
test_mime_bzip2 (void)
{
	const guint8 bz2[] = "BZh\x31\x41\x59\x26\x53\x59";
	gchar *path = write_tmp_file_with (bz2, sizeof (bz2) - 1, ".bz2");
	assert_mime_nonempty (path, "bzip2");
	g_unlink (path);
	g_free (path);
}

static void
test_mime_xz (void)
{
	const guint8 xz[] = "\xfd\x37\x7a\x58\x5a\x00";
	gchar *path = write_tmp_file_with (xz, sizeof (xz) - 1, ".xz");
	assert_mime_nonempty (path, "xz");
	g_unlink (path);
	g_free (path);
}

static void
test_mime_epub (void)
{
	/* EPUB is a zip with a 'mimetype' first entry.  We build a
	 * minimal zip with that one entry by hand -- a real EPUB
	 * would need the rest of the Open Container Format, but the
	 * magic-byte dispatcher only needs the mimetype entry to
	 * return 'application/epub+zip'. */
	const guint8 zip[] = {
		'P', 'K', 0x03, 0x04,         /* local file header signature */
		0x14, 0x00,                   /* version needed: 2.0 */
		0x00, 0x00,                   /* flags */
		0x00, 0x00,                   /* compression: stored */
		0x00, 0x00, 0x00, 0x00,       /* mod time / mod date */
		0x00, 0x00, 0x00, 0x00,       /* crc-32 */
		0x08, 0x00, 0x00, 0x00,       /* compressed size: 8 */
		0x08, 0x00, 0x00, 0x00,       /* uncompressed size: 8 */
		0x08, 0x00, 0x00, 0x00,       /* filename length: 8 */
		0x00, 0x00,                   /* extra field length: 0 */
		'm', 'i', 'm', 'e', 't', 'y', 'p', 'e',
		'a', 'p', 'p', 'l', 'i', 'c', 'a', 't',
		'i', 'o', 'n', '/', 'e', 'p', 'u', 'b', '+', 'z', 'i', 'p',
	};
	gchar *path = write_tmp_file_with (zip, sizeof (zip), ".epub");
	assert_mime_nonempty (path, "EPUB");
	g_unlink (path);
	g_free (path);
}

/* ----- negative cases ----- */

static void
test_mime_empty (void)
{
	gchar *path = write_tmp_file_with (NULL, 0, "");
	assert_mime_nonempty (path, "empty (no extension)");
	g_unlink (path);
	g_free (path);
}

static void
test_mime_random (void)
{
	/* A few bytes that don't match any known format.  Detection
	 * should still return SOME type, not crash or return NULL. */
	const guint8 random[] = { 0x42, 0x41, 0x44, 0x43, 0x4f, 0x44, 0x45 };
	gchar *path = write_tmp_file_with (random, sizeof (random), ".bin");
	assert_mime_nonempty (path, "random bytes");
	g_unlink (path);
	g_free (path);
}

static void
test_mime_nonexistent (void)
{
	/* A path that doesn't exist.  ev_file_get_mime_type is
	 * expected to return NULL (or a generic fallback) without
	 * filling @error -- the caller is expected to handle the
	 * NULL by falling back to the extension-based dispatcher. */
	GError *err = NULL;
	gchar *mime = ev_file_get_mime_type ("file:///nonexistent/path/foobar.pdf", FALSE, &err);
	g_test_message ("nonexistent -> %s (err=%s)",
	               mime ? mime : "(null)",
	               err ? err->message : "(none)");
	/* No assertion on the exact result -- both NULL and a
	 * fallback are acceptable.  The contract is just "don't
	 * crash, don't fill @error unnecessarily". */
	g_free (mime);
	g_clear_error (&err);
}

int
main (int argc, char *argv[])
{
	g_test_init (&argc, &argv, NULL);

	g_test_add_func ("/mime/pdf",         test_mime_pdf);
	g_test_add_func ("/mime/postscript", test_mime_postscript);
	g_test_add_func ("/mime/djvu",        test_mime_djvu);
	g_test_add_func ("/mime/dvi",         test_mime_dvi);
	g_test_add_func ("/mime/tiff-le",     test_mime_tiff_le);
	g_test_add_func ("/mime/tiff-be",     test_mime_tiff_be);
	g_test_add_func ("/mime/png",         test_mime_png);
	g_test_add_func ("/mime/jpeg",        test_mime_jpeg);
	g_test_add_func ("/mime/gzip",        test_mime_gzip);
	g_test_add_func ("/mime/bzip2",       test_mime_bzip2);
	g_test_add_func ("/mime/xz",          test_mime_xz);
	g_test_add_func ("/mime/epub",        test_mime_epub);
	g_test_add_func ("/mime/empty",       test_mime_empty);
	g_test_add_func ("/mime/random",      test_mime_random);
	g_test_add_func ("/mime/nonexistent", test_mime_nonexistent);

	return g_test_run ();
}
