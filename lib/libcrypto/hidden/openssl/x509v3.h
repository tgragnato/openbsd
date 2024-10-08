/* $OpenBSD: x509v3.h,v 1.15 2024/08/31 10:03:03 tb Exp $ */
/*
 * Copyright (c) 2022 Bob Beck <beck@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _LIBCRYPTO_X509V3_H
#define _LIBCRYPTO_X509V3_H

#ifndef _MSC_VER
#include_next <openssl/x509v3.h>
#else
#include "../include/openssl/x509v3.h"
#endif
#include "crypto_namespace.h"

LCRYPTO_USED(BASIC_CONSTRAINTS_new);
LCRYPTO_USED(BASIC_CONSTRAINTS_free);
LCRYPTO_USED(d2i_BASIC_CONSTRAINTS);
LCRYPTO_USED(i2d_BASIC_CONSTRAINTS);
LCRYPTO_USED(AUTHORITY_KEYID_new);
LCRYPTO_USED(AUTHORITY_KEYID_free);
LCRYPTO_USED(d2i_AUTHORITY_KEYID);
LCRYPTO_USED(i2d_AUTHORITY_KEYID);
LCRYPTO_USED(PKEY_USAGE_PERIOD_new);
LCRYPTO_USED(PKEY_USAGE_PERIOD_free);
LCRYPTO_USED(d2i_PKEY_USAGE_PERIOD);
LCRYPTO_USED(i2d_PKEY_USAGE_PERIOD);
LCRYPTO_USED(GENERAL_NAME_new);
LCRYPTO_USED(GENERAL_NAME_free);
LCRYPTO_USED(d2i_GENERAL_NAME);
LCRYPTO_USED(i2d_GENERAL_NAME);
LCRYPTO_USED(GENERAL_NAME_dup);
LCRYPTO_USED(GENERAL_NAME_cmp);
LCRYPTO_USED(v2i_ASN1_BIT_STRING);
LCRYPTO_USED(i2v_ASN1_BIT_STRING);
LCRYPTO_USED(i2v_GENERAL_NAME);
LCRYPTO_USED(GENERAL_NAME_print);
LCRYPTO_USED(GENERAL_NAMES_new);
LCRYPTO_USED(GENERAL_NAMES_free);
LCRYPTO_USED(d2i_GENERAL_NAMES);
LCRYPTO_USED(i2d_GENERAL_NAMES);
LCRYPTO_USED(i2v_GENERAL_NAMES);
LCRYPTO_USED(v2i_GENERAL_NAMES);
LCRYPTO_USED(OTHERNAME_new);
LCRYPTO_USED(OTHERNAME_free);
LCRYPTO_USED(d2i_OTHERNAME);
LCRYPTO_USED(i2d_OTHERNAME);
LCRYPTO_USED(EDIPARTYNAME_new);
LCRYPTO_USED(EDIPARTYNAME_free);
LCRYPTO_USED(d2i_EDIPARTYNAME);
LCRYPTO_USED(i2d_EDIPARTYNAME);
LCRYPTO_USED(OTHERNAME_cmp);
LCRYPTO_USED(GENERAL_NAME_set0_value);
LCRYPTO_USED(GENERAL_NAME_get0_value);
LCRYPTO_USED(GENERAL_NAME_set0_othername);
LCRYPTO_USED(GENERAL_NAME_get0_otherName);
LCRYPTO_USED(i2s_ASN1_OCTET_STRING);
LCRYPTO_USED(s2i_ASN1_OCTET_STRING);
LCRYPTO_USED(EXTENDED_KEY_USAGE_new);
LCRYPTO_USED(EXTENDED_KEY_USAGE_free);
LCRYPTO_USED(d2i_EXTENDED_KEY_USAGE);
LCRYPTO_USED(i2d_EXTENDED_KEY_USAGE);
LCRYPTO_USED(i2a_ACCESS_DESCRIPTION);
LCRYPTO_USED(CERTIFICATEPOLICIES_new);
LCRYPTO_USED(CERTIFICATEPOLICIES_free);
LCRYPTO_USED(d2i_CERTIFICATEPOLICIES);
LCRYPTO_USED(i2d_CERTIFICATEPOLICIES);
LCRYPTO_USED(POLICYINFO_new);
LCRYPTO_USED(POLICYINFO_free);
LCRYPTO_USED(d2i_POLICYINFO);
LCRYPTO_USED(i2d_POLICYINFO);
LCRYPTO_USED(POLICYQUALINFO_new);
LCRYPTO_USED(POLICYQUALINFO_free);
LCRYPTO_USED(d2i_POLICYQUALINFO);
LCRYPTO_USED(i2d_POLICYQUALINFO);
LCRYPTO_USED(USERNOTICE_new);
LCRYPTO_USED(USERNOTICE_free);
LCRYPTO_USED(d2i_USERNOTICE);
LCRYPTO_USED(i2d_USERNOTICE);
LCRYPTO_USED(NOTICEREF_new);
LCRYPTO_USED(NOTICEREF_free);
LCRYPTO_USED(d2i_NOTICEREF);
LCRYPTO_USED(i2d_NOTICEREF);
LCRYPTO_USED(CRL_DIST_POINTS_new);
LCRYPTO_USED(CRL_DIST_POINTS_free);
LCRYPTO_USED(d2i_CRL_DIST_POINTS);
LCRYPTO_USED(i2d_CRL_DIST_POINTS);
LCRYPTO_USED(DIST_POINT_new);
LCRYPTO_USED(DIST_POINT_free);
LCRYPTO_USED(d2i_DIST_POINT);
LCRYPTO_USED(i2d_DIST_POINT);
LCRYPTO_USED(DIST_POINT_NAME_new);
LCRYPTO_USED(DIST_POINT_NAME_free);
LCRYPTO_USED(d2i_DIST_POINT_NAME);
LCRYPTO_USED(i2d_DIST_POINT_NAME);
LCRYPTO_USED(ISSUING_DIST_POINT_new);
LCRYPTO_USED(ISSUING_DIST_POINT_free);
LCRYPTO_USED(d2i_ISSUING_DIST_POINT);
LCRYPTO_USED(i2d_ISSUING_DIST_POINT);
LCRYPTO_USED(DIST_POINT_set_dpname);
LCRYPTO_USED(NAME_CONSTRAINTS_check);
LCRYPTO_USED(ACCESS_DESCRIPTION_new);
LCRYPTO_USED(ACCESS_DESCRIPTION_free);
LCRYPTO_USED(d2i_ACCESS_DESCRIPTION);
LCRYPTO_USED(i2d_ACCESS_DESCRIPTION);
LCRYPTO_USED(AUTHORITY_INFO_ACCESS_new);
LCRYPTO_USED(AUTHORITY_INFO_ACCESS_free);
LCRYPTO_USED(d2i_AUTHORITY_INFO_ACCESS);
LCRYPTO_USED(i2d_AUTHORITY_INFO_ACCESS);
LCRYPTO_USED(POLICY_MAPPING_new);
LCRYPTO_USED(POLICY_MAPPING_free);
LCRYPTO_USED(GENERAL_SUBTREE_new);
LCRYPTO_USED(GENERAL_SUBTREE_free);
LCRYPTO_USED(NAME_CONSTRAINTS_new);
LCRYPTO_USED(NAME_CONSTRAINTS_free);
LCRYPTO_USED(POLICY_CONSTRAINTS_new);
LCRYPTO_USED(POLICY_CONSTRAINTS_free);
LCRYPTO_USED(a2i_GENERAL_NAME);
LCRYPTO_USED(v2i_GENERAL_NAME);
LCRYPTO_USED(v2i_GENERAL_NAME_ex);
LCRYPTO_USED(X509V3_conf_free);
LCRYPTO_USED(X509V3_EXT_nconf_nid);
LCRYPTO_USED(X509V3_EXT_nconf);
LCRYPTO_USED(X509V3_EXT_add_nconf_sk);
LCRYPTO_USED(X509V3_EXT_add_nconf);
LCRYPTO_USED(X509V3_EXT_REQ_add_nconf);
LCRYPTO_USED(X509V3_EXT_CRL_add_nconf);
LCRYPTO_USED(X509V3_EXT_conf_nid);
LCRYPTO_USED(X509V3_EXT_conf);
LCRYPTO_USED(X509V3_set_nconf);
LCRYPTO_USED(X509V3_set_ctx);
LCRYPTO_USED(i2s_ASN1_INTEGER);
LCRYPTO_USED(s2i_ASN1_INTEGER);
LCRYPTO_USED(i2s_ASN1_ENUMERATED);
LCRYPTO_USED(i2s_ASN1_ENUMERATED_TABLE);
LCRYPTO_USED(X509V3_EXT_get);
LCRYPTO_USED(X509V3_EXT_get_nid);
LCRYPTO_USED(X509V3_add_standard_extensions);
LCRYPTO_USED(X509V3_parse_list);
LCRYPTO_USED(X509V3_EXT_d2i);
LCRYPTO_USED(X509V3_get_d2i);
LCRYPTO_USED(X509V3_EXT_i2d);
LCRYPTO_USED(X509V3_add1_i2d);
LCRYPTO_USED(hex_to_string);
LCRYPTO_USED(string_to_hex);
LCRYPTO_USED(X509V3_EXT_val_prn);
LCRYPTO_USED(X509V3_EXT_print);
LCRYPTO_USED(X509V3_EXT_print_fp);
LCRYPTO_USED(X509V3_extensions_print);
LCRYPTO_USED(X509_check_ca);
LCRYPTO_USED(X509_check_purpose);
LCRYPTO_USED(X509_supported_extension);
LCRYPTO_USED(X509_check_issued);
LCRYPTO_USED(X509_check_akid);
LCRYPTO_USED(X509_PURPOSE_get_count);
LCRYPTO_USED(X509_PURPOSE_get0);
LCRYPTO_USED(X509_PURPOSE_get_by_sname);
LCRYPTO_USED(X509_PURPOSE_get0_name);
LCRYPTO_USED(X509_PURPOSE_get0_sname);
LCRYPTO_USED(X509_PURPOSE_get_id);
LCRYPTO_USED(X509_get_extension_flags);
LCRYPTO_USED(X509_get_key_usage);
LCRYPTO_USED(X509_get_extended_key_usage);
LCRYPTO_USED(X509_get1_email);
LCRYPTO_USED(X509_REQ_get1_email);
LCRYPTO_USED(X509_email_free);
LCRYPTO_USED(X509_get1_ocsp);
LCRYPTO_USED(X509_check_host);
LCRYPTO_USED(X509_check_email);
LCRYPTO_USED(X509_check_ip);
LCRYPTO_USED(X509_check_ip_asc);
LCRYPTO_USED(a2i_IPADDRESS);
LCRYPTO_USED(a2i_IPADDRESS_NC);
LCRYPTO_USED(a2i_ipadd);
LCRYPTO_USED(X509V3_NAME_from_section);
LCRYPTO_USED(ASRange_new);
LCRYPTO_USED(ASRange_free);
LCRYPTO_USED(d2i_ASRange);
LCRYPTO_USED(i2d_ASRange);
LCRYPTO_USED(ASIdOrRange_new);
LCRYPTO_USED(ASIdOrRange_free);
LCRYPTO_USED(d2i_ASIdOrRange);
LCRYPTO_USED(i2d_ASIdOrRange);
LCRYPTO_USED(ASIdentifierChoice_new);
LCRYPTO_USED(ASIdentifierChoice_free);
LCRYPTO_USED(d2i_ASIdentifierChoice);
LCRYPTO_USED(i2d_ASIdentifierChoice);
LCRYPTO_USED(ASIdentifiers_new);
LCRYPTO_USED(ASIdentifiers_free);
LCRYPTO_USED(d2i_ASIdentifiers);
LCRYPTO_USED(i2d_ASIdentifiers);
LCRYPTO_USED(IPAddressRange_new);
LCRYPTO_USED(IPAddressRange_free);
LCRYPTO_USED(d2i_IPAddressRange);
LCRYPTO_USED(i2d_IPAddressRange);
LCRYPTO_USED(IPAddressOrRange_new);
LCRYPTO_USED(IPAddressOrRange_free);
LCRYPTO_USED(d2i_IPAddressOrRange);
LCRYPTO_USED(i2d_IPAddressOrRange);
LCRYPTO_USED(IPAddressChoice_new);
LCRYPTO_USED(IPAddressChoice_free);
LCRYPTO_USED(d2i_IPAddressChoice);
LCRYPTO_USED(i2d_IPAddressChoice);
LCRYPTO_USED(IPAddressFamily_new);
LCRYPTO_USED(IPAddressFamily_free);
LCRYPTO_USED(d2i_IPAddressFamily);
LCRYPTO_USED(i2d_IPAddressFamily);
LCRYPTO_USED(X509v3_asid_add_inherit);
LCRYPTO_USED(X509v3_asid_add_id_or_range);
LCRYPTO_USED(X509v3_addr_add_inherit);
LCRYPTO_USED(X509v3_addr_add_prefix);
LCRYPTO_USED(X509v3_addr_add_range);
LCRYPTO_USED(X509v3_addr_get_afi);
LCRYPTO_USED(X509v3_addr_get_range);
LCRYPTO_USED(X509v3_asid_is_canonical);
LCRYPTO_USED(X509v3_addr_is_canonical);
LCRYPTO_USED(X509v3_asid_canonize);
LCRYPTO_USED(X509v3_addr_canonize);
LCRYPTO_USED(X509v3_asid_inherits);
LCRYPTO_USED(X509v3_addr_inherits);
LCRYPTO_USED(X509v3_asid_subset);
LCRYPTO_USED(X509v3_addr_subset);
LCRYPTO_USED(X509v3_asid_validate_path);
LCRYPTO_USED(X509v3_addr_validate_path);
LCRYPTO_USED(X509v3_asid_validate_resource_set);
LCRYPTO_USED(X509v3_addr_validate_resource_set);
LCRYPTO_USED(ERR_load_X509V3_strings);
#if defined(LIBRESSL_NAMESPACE)
extern LCRYPTO_USED(BASIC_CONSTRAINTS_it);
extern LCRYPTO_USED(AUTHORITY_KEYID_it);
extern LCRYPTO_USED(PKEY_USAGE_PERIOD_it);
extern LCRYPTO_USED(GENERAL_NAME_it);
extern LCRYPTO_USED(GENERAL_NAMES_it);
extern LCRYPTO_USED(OTHERNAME_it);
extern LCRYPTO_USED(EDIPARTYNAME_it);
extern LCRYPTO_USED(EXTENDED_KEY_USAGE_it);
extern LCRYPTO_USED(CERTIFICATEPOLICIES_it);
extern LCRYPTO_USED(POLICYINFO_it);
extern LCRYPTO_USED(POLICYQUALINFO_it);
extern LCRYPTO_USED(USERNOTICE_it);
extern LCRYPTO_USED(NOTICEREF_it);
extern LCRYPTO_USED(CRL_DIST_POINTS_it);
extern LCRYPTO_USED(DIST_POINT_it);
extern LCRYPTO_USED(DIST_POINT_NAME_it);
extern LCRYPTO_USED(DIST_POINT_it);
extern LCRYPTO_USED(ISSUING_DIST_POINT_it);
extern LCRYPTO_USED(ACCESS_DESCRIPTION_it);
extern LCRYPTO_USED(AUTHORITY_INFO_ACCESS_it);
extern LCRYPTO_USED(POLICY_MAPPING_it);
extern LCRYPTO_USED(POLICY_MAPPINGS_it);
extern LCRYPTO_USED(GENERAL_SUBTREE_it);
extern LCRYPTO_USED(NAME_CONSTRAINTS_it);
extern LCRYPTO_USED(POLICY_CONSTRAINTS_it);
extern LCRYPTO_USED(ASRange_it);
extern LCRYPTO_USED(ASIdOrRange_it);
extern LCRYPTO_USED(ASIdentifierChoice_it);
extern LCRYPTO_USED(ASIdentifiers_it);
extern LCRYPTO_USED(IPAddressRange_it);
extern LCRYPTO_USED(IPAddressOrRange_it);
extern LCRYPTO_USED(IPAddressChoice_it);
extern LCRYPTO_USED(IPAddressFamily_it);
#endif

#endif /* _LIBCRYPTO_X509V3_H */
