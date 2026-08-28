/* ev-document-ocr.h
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

#if !defined (__EV_XREADER_DOCUMENT_H_INSIDE__) && !defined (XREADER_COMPILATION)
#error "Only <xreader-document.h> can be included directly."
#endif

#ifndef EV_DOCUMENT_OCR_H
#define EV_DOCUMENT_OCR_H

#include <glib-object.h>
#include <glib.h>
#include <cairo.h>

#include "ev-document.h"
#include "ev-page.h"

G_BEGIN_DECLS

#define EV_TYPE_DOCUMENT_OCR            (ev_document_ocr_get_type ())
#define EV_DOCUMENT_OCR(o)              (G_TYPE_CHECK_INSTANCE_CAST ((o), EV_TYPE_DOCUMENT_OCR, EvDocumentOCR))
#define EV_DOCUMENT_OCR_IFACE(k)        (G_TYPE_CHECK_CLASS_CAST ((k), EV_TYPE_DOCUMENT_OCR, EvDocumentOCRInterface))
#define EV_IS_DOCUMENT_OCR(o)           (G_TYPE_CHECK_INSTANCE_TYPE ((o), EV_TYPE_DOCUMENT_OCR))
#define EV_IS_DOCUMENT_OCR_IFACE(k)     (G_TYPE_CHECK_CLASS_TYPE ((k), EV_TYPE_DOCUMENT_OCR))
#define EV_DOCUMENT_OCR_GET_IFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), EV_TYPE_DOCUMENT_OCR, EvDocumentOCRInterface))

#define EV_TYPE_OCR_RESULT              (ev_ocr_result_get_type ())

typedef struct _EvDocumentOCR          EvDocumentOCR;
typedef struct _EvDocumentOCRInterface EvDocumentOCRInterface;
typedef struct _EvOCRResult            EvOCRResult;

struct _EvOCRResult {
	gchar       *text;
	EvRectangle  rect;
	gdouble      confidence;
};

struct _EvDocumentOCRInterface
{
	GTypeInterface base_iface;

	/* Methods */
	gboolean  (* supports_ocr)       (EvDocumentOCR *document_ocr);
	gchar    *(* get_page_ocr_text)  (EvDocumentOCR *document_ocr,
	                                  EvPage        *page,
	                                  const gchar   *lang,
	                                  GError       **error);
	GList    *(* get_page_ocr_layout)(EvDocumentOCR *document_ocr,
	                                  EvPage        *page,
	                                  const gchar   *lang,
	                                  GError       **error);
};

GType        ev_document_ocr_get_type        (void) G_GNUC_CONST;
GType        ev_ocr_result_get_type          (void) G_GNUC_CONST;

EvOCRResult *ev_ocr_result_new               (const gchar       *text,
                                              const EvRectangle *rect,
                                              gdouble            confidence);
EvOCRResult *ev_ocr_result_copy              (const EvOCRResult *result);
void         ev_ocr_result_free              (EvOCRResult       *result);

gboolean     ev_document_ocr_supports_ocr    (EvDocumentOCR     *document_ocr);
gchar       *ev_document_ocr_get_page_text   (EvDocumentOCR     *document_ocr,
                                              EvPage            *page,
                                              const gchar       *lang,
                                              GError           **error);
GList       *ev_document_ocr_get_page_layout (EvDocumentOCR     *document_ocr,
                                              EvPage            *page,
                                              const gchar       *lang,
                                              GError           **error);

gchar       *ev_document_ocr_recognize_surface (cairo_surface_t *surface,
                                                const gchar     *lang,
                                                GError         **error);

G_END_DECLS

#endif /* EV_DOCUMENT_OCR_H */
