/* ev-document-signatures.c
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
#include "ev-document-signatures.h"

G_DEFINE_INTERFACE (EvDocumentSignatures, ev_document_signatures, 0)
G_DEFINE_BOXED_TYPE (EvSignature, ev_signature, ev_signature_copy, ev_signature_free)

static void
ev_document_signatures_default_init (EvDocumentSignaturesInterface *klass)
{
}

EvSignature *
ev_signature_new (void)
{
	EvSignature *sig = g_new0 (EvSignature, 1);
	sig->status = EV_SIGNATURE_STATUS_UNKNOWN;
	sig->page = -1;
	return sig;
}

EvSignature *
ev_signature_copy (const EvSignature *sig)
{
	EvSignature *copy;

	if (!sig)
		return NULL;

	copy = g_new0 (EvSignature, 1);
	copy->signer_name = g_strdup (sig->signer_name);
	copy->signing_time = g_strdup (sig->signing_time);
	copy->reason = g_strdup (sig->reason);
	copy->location = g_strdup (sig->location);
	copy->certificate_issuer = g_strdup (sig->certificate_issuer);
	copy->certificate_subject = g_strdup (sig->certificate_subject);
	copy->status = sig->status;
	copy->page = sig->page;

	return copy;
}

void
ev_signature_free (EvSignature *sig)
{
	if (!sig)
		return;

	g_free (sig->signer_name);
	g_free (sig->signing_time);
	g_free (sig->reason);
	g_free (sig->location);
	g_free (sig->certificate_issuer);
	g_free (sig->certificate_subject);
	g_free (sig);
}

gboolean
ev_document_signatures_has_signatures (EvDocumentSignatures *document_signatures)
{
	EvDocumentSignaturesInterface *iface;

	g_return_val_if_fail (EV_IS_DOCUMENT_SIGNATURES (document_signatures), FALSE);

	iface = EV_DOCUMENT_SIGNATURES_GET_IFACE (document_signatures);
	if (iface->has_signatures)
		return iface->has_signatures (document_signatures);

	return FALSE;
}

GList *
ev_document_signatures_get_signatures (EvDocumentSignatures *document_signatures)
{
	EvDocumentSignaturesInterface *iface;

	g_return_val_if_fail (EV_IS_DOCUMENT_SIGNATURES (document_signatures), NULL);

	iface = EV_DOCUMENT_SIGNATURES_GET_IFACE (document_signatures);
	if (iface->get_signatures)
		return iface->get_signatures (document_signatures);

	return NULL;
}

const gchar *
ev_signature_status_to_string (EvSignatureStatus status)
{
	switch (status) {
	case EV_SIGNATURE_STATUS_VALID:
		return _("Valid digital signature");
	case EV_SIGNATURE_STATUS_INVALID:
		return _("Invalid digital signature (document modified)");
	case EV_SIGNATURE_STATUS_DIGEST_MISMATCH:
		return _("Signature hash digest mismatch");
	case EV_SIGNATURE_STATUS_UNTRUSTED:
		return _("Valid signature, untrusted certificate authority");
	case EV_SIGNATURE_STATUS_UNKNOWN:
	default:
		return _("Signature status unknown or unverified");
	}
}
