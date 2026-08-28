/* ev-document-signatures.h
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

#ifndef EV_DOCUMENT_SIGNATURES_H
#define EV_DOCUMENT_SIGNATURES_H

#include <glib-object.h>
#include <glib.h>

#include "ev-document.h"

G_BEGIN_DECLS

#define EV_TYPE_DOCUMENT_SIGNATURES            (ev_document_signatures_get_type ())
#define EV_DOCUMENT_SIGNATURES(o)              (G_TYPE_CHECK_INSTANCE_CAST ((o), EV_TYPE_DOCUMENT_SIGNATURES, EvDocumentSignatures))
#define EV_DOCUMENT_SIGNATURES_IFACE(k)        (G_TYPE_CHECK_CLASS_CAST ((k), EV_TYPE_DOCUMENT_SIGNATURES, EvDocumentSignaturesInterface))
#define EV_IS_DOCUMENT_SIGNATURES(o)           (G_TYPE_CHECK_INSTANCE_TYPE ((o), EV_TYPE_DOCUMENT_SIGNATURES))
#define EV_IS_DOCUMENT_SIGNATURES_IFACE(k)     (G_TYPE_CHECK_CLASS_TYPE ((k), EV_TYPE_DOCUMENT_SIGNATURES))
#define EV_DOCUMENT_SIGNATURES_GET_IFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), EV_TYPE_DOCUMENT_SIGNATURES, EvDocumentSignaturesInterface))

#define EV_TYPE_SIGNATURE                      (ev_signature_get_type ())

typedef struct _EvDocumentSignatures          EvDocumentSignatures;
typedef struct _EvDocumentSignaturesInterface EvDocumentSignaturesInterface;
typedef struct _EvSignature                   EvSignature;

typedef enum {
	EV_SIGNATURE_STATUS_VALID,
	EV_SIGNATURE_STATUS_INVALID,
	EV_SIGNATURE_STATUS_DIGEST_MISMATCH,
	EV_SIGNATURE_STATUS_UNTRUSTED,
	EV_SIGNATURE_STATUS_UNKNOWN
} EvSignatureStatus;

struct _EvSignature {
	gchar             *signer_name;
	gchar             *signing_time;
	gchar             *reason;
	gchar             *location;
	gchar             *certificate_issuer;
	gchar             *certificate_subject;
	EvSignatureStatus  status;
	gint               page;
};

struct _EvDocumentSignaturesInterface
{
	GTypeInterface base_iface;

	/* Methods */
	gboolean  (* has_signatures)  (EvDocumentSignatures *document_signatures);
	GList    *(* get_signatures)  (EvDocumentSignatures *document_signatures);
};

GType        ev_document_signatures_get_type       (void) G_GNUC_CONST;
GType        ev_signature_get_type                 (void) G_GNUC_CONST;

EvSignature *ev_signature_new                      (void);
EvSignature *ev_signature_copy                     (const EvSignature *sig);
void         ev_signature_free                     (EvSignature       *sig);

gboolean     ev_document_signatures_has_signatures (EvDocumentSignatures *document_signatures);
GList       *ev_document_signatures_get_signatures (EvDocumentSignatures *document_signatures);

const gchar *ev_signature_status_to_string         (EvSignatureStatus  status);

G_END_DECLS

#endif /* EV_DOCUMENT_SIGNATURES_H */
