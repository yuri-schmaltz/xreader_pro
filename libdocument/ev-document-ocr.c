/* ev-document-ocr.c
 *  this file is part of xreader, a document viewer
 *
 * Copyright (C) 2026 Xreader Developers
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

#include "config.h"
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>
#include "ev-document-ocr.h"
#include "ev-file-helpers.h"

G_DEFINE_INTERFACE (EvDocumentOCR, ev_document_ocr, 0)
G_DEFINE_BOXED_TYPE (EvOCRResult, ev_ocr_result, ev_ocr_result_copy, ev_ocr_result_free)

static void
ev_document_ocr_default_init (EvDocumentOCRInterface *klass)
{
}

EvOCRResult *
ev_ocr_result_new (const gchar       *text,
                   const EvRectangle *rect,
                   gdouble            confidence)
{
	EvOCRResult *result = g_new0 (EvOCRResult, 1);
	result->text = g_strdup (text);
	if (rect)
		result->rect = *rect;
	result->confidence = confidence;
	return result;
}

EvOCRResult *
ev_ocr_result_copy (const EvOCRResult *result)
{
	if (!result)
		return NULL;

	return ev_ocr_result_new (result->text, &result->rect, result->confidence);
}

void
ev_ocr_result_free (EvOCRResult *result)
{
	if (!result)
		return;

	g_free (result->text);
	g_free (result);
}

gboolean
ev_document_ocr_supports_ocr (EvDocumentOCR *document_ocr)
{
	EvDocumentOCRInterface *iface;

	g_return_val_if_fail (EV_IS_DOCUMENT_OCR (document_ocr), FALSE);

	iface = EV_DOCUMENT_OCR_GET_IFACE (document_ocr);
	if (iface->supports_ocr)
		return iface->supports_ocr (document_ocr);

	return TRUE;
}

gchar *
ev_document_ocr_get_page_text (EvDocumentOCR *document_ocr,
                               EvPage        *page,
                               const gchar   *lang,
                               GError       **error)
{
	EvDocumentOCRInterface *iface;

	g_return_val_if_fail (EV_IS_DOCUMENT_OCR (document_ocr), NULL);
	g_return_val_if_fail (EV_IS_PAGE (page), NULL);

	iface = EV_DOCUMENT_OCR_GET_IFACE (document_ocr);
	if (iface->get_page_ocr_text)
		return iface->get_page_ocr_text (document_ocr, page, lang, error);

	g_set_error_literal (error,
	                     EV_DOCUMENT_ERROR,
	                     EV_DOCUMENT_ERROR_UNSUPPORTED_CONTENT,
	                     _("OCR text recognition is not implemented for this backend"));
	return NULL;
}

GList *
ev_document_ocr_get_page_layout (EvDocumentOCR *document_ocr,
                                 EvPage        *page,
                                 const gchar   *lang,
                                 GError       **error)
{
	EvDocumentOCRInterface *iface;

	g_return_val_if_fail (EV_IS_DOCUMENT_OCR (document_ocr), NULL);
	g_return_val_if_fail (EV_IS_PAGE (page), NULL);

	iface = EV_DOCUMENT_OCR_GET_IFACE (document_ocr);
	if (iface->get_page_ocr_layout)
		return iface->get_page_ocr_layout (document_ocr, page, lang, error);

	return NULL;
}

gchar *
ev_document_ocr_recognize_surface (cairo_surface_t *surface,
                                   const gchar     *lang,
                                   GError         **error)
{
	gchar *tesseract_path;
	gchar *tmp_png = NULL;
	gchar *tmp_out_base = NULL;
	gchar *tmp_out_txt = NULL;
	gchar *result_text = NULL;
	gint fd_in, fd_out;
	cairo_status_t cstatus;

	g_return_val_if_fail (surface != NULL, NULL);

	tesseract_path = g_find_program_in_path ("tesseract");
	if (!tesseract_path) {
		g_set_error_literal (error,
		                     EV_DOCUMENT_ERROR,
		                     EV_DOCUMENT_ERROR_INVALID,
		                     _("Tesseract OCR engine is not installed on this system"));
		return NULL;
	}

	fd_in = ev_mkstemp ("ocr_in.XXXXXX.png", &tmp_png, error);
	if (fd_in < 0) {
		g_free (tesseract_path);
		return NULL;
	}
	close (fd_in);

	cstatus = cairo_surface_write_to_png (surface, tmp_png);
	if (cstatus != CAIRO_STATUS_SUCCESS) {
		g_set_error (error,
		             EV_DOCUMENT_ERROR,
		             EV_DOCUMENT_ERROR_INVALID,
		             _("Failed to render page image for OCR: %s"),
		             cairo_status_to_string (cstatus));
		ev_tmp_filename_unlink (tmp_png);
		g_free (tmp_png);
		g_free (tesseract_path);
		return NULL;
	}

	fd_out = ev_mkstemp ("ocr_out.XXXXXX", &tmp_out_base, error);
	if (fd_out < 0) {
		ev_tmp_filename_unlink (tmp_png);
		g_free (tmp_png);
		g_free (tesseract_path);
		return NULL;
	}
	close (fd_out);

	const gchar *ocr_lang = (lang && *lang) ? lang : "eng";
	gchar *argv[] = {
		tesseract_path,
		tmp_png,
		tmp_out_base,
		(gchar *) "-l",
		(gchar *) ocr_lang,
		(gchar *) "--oem",
		(gchar *) "1",
		(gchar *) "-c",
		(gchar *) "tessedit_create_txt=1",
		NULL
	};

	gint exit_status = 0;
	if (g_spawn_sync (NULL, argv, NULL, G_SPAWN_DEFAULT, NULL, NULL, NULL, NULL, &exit_status, error)) {
		tmp_out_txt = g_strdup_printf ("%s.txt", tmp_out_base);
		if (g_file_test (tmp_out_txt, G_FILE_TEST_EXISTS)) {
			g_file_get_contents (tmp_out_txt, &result_text, NULL, NULL);
			g_unlink (tmp_out_txt);
		}
		g_free (tmp_out_txt);
	}

	ev_tmp_filename_unlink (tmp_png);
	ev_tmp_filename_unlink (tmp_out_base);
	g_free (tmp_png);
	g_free (tmp_out_base);
	g_free (tesseract_path);

	return result_text ? result_text : g_strdup ("");
}
