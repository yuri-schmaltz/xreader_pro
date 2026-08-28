/* test-ocr.c - GLib Testing unit test for EvDocumentOCR and EvOCRResult */

#include "config.h"
#include <glib.h>
#include <cairo.h>
#include "ev-document-ocr.h"

static void
test_ocr_result_lifecycle (void)
{
	EvRectangle rect = { 10.0, 20.0, 100.0, 150.0 };
	EvOCRResult *result = ev_ocr_result_new ("Sample OCR Text", &rect, 95.5);
	g_assert_nonnull (result);
	g_assert_cmpstr (result->text, ==, "Sample OCR Text");
	g_assert_cmpfloat (result->rect.x1, ==, 10.0);
	g_assert_cmpfloat (result->rect.y1, ==, 20.0);
	g_assert_cmpfloat (result->confidence, ==, 95.5);

	EvOCRResult *copy = ev_ocr_result_copy (result);
	g_assert_nonnull (copy);
	g_assert_cmpstr (copy->text, ==, "Sample OCR Text");
	g_assert_cmpfloat (copy->confidence, ==, 95.5);

	ev_ocr_result_free (copy);
	ev_ocr_result_free (result);
}

static void
test_ocr_surface_recognition (void)
{
	cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_RGB24, 200, 50);
	cairo_t *cr = cairo_create (surface);
	cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
	cairo_paint (cr);
	cairo_destroy (cr);

	GError *error = NULL;
	gchar *text = ev_document_ocr_recognize_surface (surface, "eng", &error);
	/* If tesseract is not installed on this system, error is populated gracefully */
	if (!text) {
		g_assert_nonnull (error);
		g_error_free (error);
	} else {
		g_assert_nonnull (text);
		g_free (text);
	}

	cairo_surface_destroy (surface);
}

int
main (int argc, char *argv[])
{
	g_test_init (&argc, &argv, NULL);

	g_test_add_func ("/ocr/result-lifecycle", test_ocr_result_lifecycle);
	g_test_add_func ("/ocr/surface-recognition", test_ocr_surface_recognition);

	return g_test_run ();
}
