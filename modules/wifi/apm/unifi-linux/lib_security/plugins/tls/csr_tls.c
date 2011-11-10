/** @file csr_tls.c
 *
 * Security Manager TLS support functions.
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2008. All rights reserved.
 *
 *   Refer to LICENSE.txt included with this source for details on the
 *   license terms.
 *
 * @section DESCRIPTION
 *   TLS support functions.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_security/plugins/tls/csr_tls.c#5 $
 *
 ****************************************************************************/

#include "csr_tls.h"
#include "sme_trace/sme_trace.h"
#include "csr_crypto.h"
#include "csr_asn1/csr_asn1.h"
#include "csr_des/csr_des.h"

#ifndef MINIMUM
#define MINIMUM(a,b) (((a) < (b))? (a) : (b))
#endif
#ifndef MAXIMUM
#define MAXIMUM(a,b) (((a) > (b))? (a) : (b))
#endif

/* TLS protocol codes */
#define TLS_CHANGE_CYPHER_SPEC_PROTOCOL 20
#define TLS_ALERT_PROTOCOL              21
#define TLS_HANDSHAKE_PROTOCOL          22
#define TLS_APPLICATION_DATA_PROTOCOL   23

/**
 * @brief The p_hashing function as defined by RFC: 2246
 *
 * @par Description
 *      This calculation uses a combination of a HMAC_MD5 hash and a HMAC_SHA1
 *      hash. The output (hash) can be as long as you like. p_hash will
 *      generate enough data to fill it.
 *
 * @param[in]   secret : the pre-master shared secret
 * @param[in]   secret_len : the length of the pre-master shared secret in octets
 * @param[in]   seed : the p_hash seed
 * @param[in]   seed_len : length of the p_hash seed in octets
 * @param[out]  answer : buffer for containing the answer
 * @param[in]   answer_len : the size the output buffer in octets
 */

/*lint -save -e429 -e669 */
static void p_hash(CsrUint8 *secret, CsrUint32 secret_len,
                   CsrUint8 *seed, CsrUint32 seed_len,
                   CsrUint8 *answer, CsrUint32 answer_len,
                   CsrBool alg) /*0 = sha1, 1 = md5 */
{
    CsrUint32 i;
    CsrUint8 *a;
    CsrUint32 a_len;
    CsrUint32 data_done = 0;

    if ((a = CsrPmalloc(MAXIMUM(seed_len, CSR_SHA1_DIGEST_LENGTH))) == NULL)
    {
        sme_trace_error((TR_SECURITY_LIB, "p_hash(). Memory allocation error (a)."));
        return;
    }

    CsrMemCpy(a, seed, seed_len); /* A0 */
    a_len = seed_len;

    while (data_done != answer_len)
    {
        CsrUint8 output[CSR_SHA1_DIGEST_LENGTH];
        CsrUint32 output_len;

        /* Calculate A(i) = HMAC_sha1(secret, A(i-1)) */
        /* need to either allocate a bigger buffer or to Malloc one. */
         /* TODO */
        CsrUint8 a_next[CSR_SHA1_DIGEST_LENGTH+100];
/*        CsrUint8 a_next[CSR_SHA1_DIGEST_LENGTH+seed_len];
 */
        CsrUint32 a_next_len;
        if (alg)
        {
            CsrCryptoCallHmacMd5(NULL, NULL,
                                 secret, secret_len, a, a_len, a_next);
            a_next_len = CSR_CRYPTO_MD5_DIGEST_LENGTH;
        }
        else
        {
            CsrCryptoCallHmacSha1(NULL, NULL,
                                  secret, secret_len, a, a_len, a_next);
            a_next_len = CSR_SHA1_DIGEST_LENGTH;
        }
        /* concatenate the seed (A(i) + seed)*/
        for (i=0; i<seed_len; i++)
        {
            a_next[a_next_len + i] = seed[i];
        }

        /* calculate the next block of output = HMAX_sha1(secret, (A(i) + seed)) */
        if (alg)
        {
            CsrCryptoCallHmacMd5(NULL, NULL,
                                 secret, secret_len, a_next, a_next_len + seed_len, output);
            output_len = CSR_CRYPTO_MD5_DIGEST_LENGTH;
        }
        else
        {
            CsrCryptoCallHmacSha1(NULL, NULL,
                                  secret, secret_len, a_next, a_next_len + seed_len, output);
            output_len = CSR_SHA1_DIGEST_LENGTH;
        }
        /* copy as much as we need */
        for (i=0; i<output_len; i++)
        {
            answer[data_done++] = output[i];
            if (data_done == answer_len)
                break;
        }

        /* update A(i-1) for next iteration */
        CsrMemCpy(a, a_next, a_next_len);
        a_len = a_next_len;
    }

    CsrPfree(a);
}
/*lint -restore */

/*
 * See header file for description.
 * ------------------------------------------------------------------------- */
/*lint -save -e429 */
void prf(const CsrUint8 *secret, const CsrUint32 secret_len,
         const char *label, const CsrUint32 label_len,
         const CsrUint8 *seed, const CsrUint32 seed_len,
         CsrUint8 *answer, const CsrUint32 answer_len)
{
    CsrUint32 i;
    CsrUint8 *s1 = NULL;
    CsrUint8 *s2 = NULL;
    CsrUint8 *a1 = NULL;
    CsrUint8 *a2 = NULL;
    CsrUint8 *phash_seed = NULL;
    CsrUint32 s_len = secret_len>>1;

    /* If secret_len is odd we need to round up the lengths of s1 and s2.
     * The first byte of s2 is the same as the last byte of s1. */
    if (secret_len & 1)
        s_len++;

    if ( ((s1 = CsrPmalloc(s_len)) == NULL) ||
         ((s2 = CsrPmalloc(s_len)) == NULL) ||
         ((phash_seed = CsrPmalloc(label_len+seed_len)) == NULL) ||
         ((a1 = CsrPmalloc(answer_len)) == NULL) ||
         ((a2 = CsrPmalloc(answer_len)) == NULL)
       )
    {
        sme_trace_error((TR_SECURITY_LIB, "prf(). Memory allocation error."));
    }
    else
    {
        CsrMemCpy(s1, secret, s_len);
        CsrMemCpy(s2, secret+(secret_len>>1), s_len);

        CsrMemCpy(phash_seed, label, label_len);
        CsrMemCpy(phash_seed + label_len, seed, seed_len);

        p_hash(s1, s_len, phash_seed, label_len + seed_len, a1, answer_len, 1);
        p_hash(s2, s_len, phash_seed, label_len + seed_len, a2, answer_len, 0);

        for (i=0; i<answer_len; i++)
            answer[i] = a1[i] ^ a2[i];
    }

    if (s1) CsrPfree(s1);
    if (s2) CsrPfree(s2);
    if (phash_seed) CsrPfree(phash_seed);
    if (a1) CsrPfree(a1);
    if (a2) CsrPfree(a2);
}
/*lint -restore */

/*
 * See header file for description.
 * ------------------------------------------------------------------------- */
/*lint -save -e669 */
void t_prf(const CsrUint8 *key, const CsrUint32 key_len,
           const char *label, const CsrUint32 label_len,
           const CsrUint8 *seed, const CsrUint32 seed_len,
           CsrUint8 *output, const CsrUint32 output_len)
{
    CsrUint8 *S;
    CsrUint8 T[CSR_SHA1_DIGEST_LENGTH];
    CsrUint32 T_length = 0;
    CsrUint32 S_head, S_tail;
    CsrUint32 bytes_done = 0;

    if ((S = CsrPmalloc(CSR_SHA1_DIGEST_LENGTH + label_len + 1 + seed_len + 3/*suffix*/))
        == NULL)
    {
        sme_trace_error((TR_SECURITY_LIB, "t_prf(). Memory allocation error (S)."));
        return;
    }

    /* S = label + "" + seed  (leave some room at the head to add T(n-1) */
    CsrMemCpy(&(S[CSR_SHA1_DIGEST_LENGTH]), label, label_len);
    S[CSR_SHA1_DIGEST_LENGTH+label_len] = 0;
    CsrMemCpy(&(S[CSR_SHA1_DIGEST_LENGTH+label_len+1]), seed, seed_len);

    /* add the "OutputLength + n" suffix */
    S_tail = CSR_SHA1_DIGEST_LENGTH + label_len + 1 + seed_len;
    S[S_tail++] = (output_len >> 8) & 0xff;
    S[S_tail++] = output_len & 0xff;
    S[S_tail] = 1;

    CsrMemSet(T, 0, CSR_SHA1_DIGEST_LENGTH);
    while(bytes_done<output_len)
    {
        S_head = CSR_SHA1_DIGEST_LENGTH;

        /* T(n) = HMAC-SHA1(key, T(n-1) + S + OutputLength + n) */
        CsrMemCpy(S, T, T_length);
        S_head -= T_length;
        CsrCryptoCallHmacSha1(NULL, NULL,
                              key, key_len, &(S[S_head]), S_tail + 1 - S_head, T);
        T_length = CSR_SHA1_DIGEST_LENGTH;
        /* Now add T(n) to the output */
        CsrMemCpy(output+bytes_done, T, MINIMUM(CSR_SHA1_DIGEST_LENGTH, output_len - bytes_done));
        bytes_done += CSR_SHA1_DIGEST_LENGTH;

        /* Increment the suffix */
        S[S_tail]++;
    }

    CsrPfree(S);
}
/*lint -restore */

/**
 * @brief Increment a big-endian 64-bit sequence number.
 *
 * @par Description
 *      This is used for incrementing the read and write sequence numbers.
 *
 * @param[in]   seq_no : pointer to the sequence number
 */
void inc_seq_no(CsrUint8 *seq_no)
{
    CsrInt32 i;
    for (i=7; i>=0; i--)
        if (seq_no[i]++ != 255)
            break;
}

/**
 * @brief Check the message authentication code of a TLS message from the AP.
 *
 * @par Description
 *      This takes a TLS message and a MAC code from the AP and checks that
 *      the MAC is ok. MAC code always comes immediately after the TLS message
 *      so that's where it is assumed to be.
 *
 * @param[in]   msg_type : The TLS content type of the message
 * @param[in]   data : The TLS message
 * @param[in]   length : length of the message (not including MAC code). The
 *                       MAC code is assumed to be the 20 octets of data AFTER
 *                       length in the data buffer.
 * @param[in]   tls : The TLS context structure we are currently working with.
 */
static CsrBool check_mac(CsrUint8 msg_type, CsrUint8 *data, CsrUint32 length, tls_data *tls)
{
    CsrUint8 mac[MAC_LENGTH];
    CsrUint8 *mac_source_data = CsrPmalloc(length + 8+5);
    CsrBool ret = FALSE;

    sme_trace_info((TR_SECURITY_LIB, "check_mac() :: length = %d", length));

    if (mac_source_data)
    {
        CsrMemCpy(mac_source_data, tls->read_seq_no, 8);
        mac_source_data[8] = msg_type;
        mac_source_data[9] = 3;  /* protocol version hi */
        mac_source_data[10] = 1; /* protocol version low */
        mac_source_data[11] = (length >> 8) & 0xff; /* fragment length hi */
        mac_source_data[12] = length & 0xff; /* fragment length low */
        CsrMemCpy(mac_source_data+8+5, data, length);

        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_DEBUG, "check_mac() :: server_write_MAC_secret", tls->keys.server_write_MAC_secret, MAC_LENGTH));
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_DEBUG, "check_mac() :: mac_source_data", mac_source_data, 8+5+length));

        CsrCryptoCallHmacSha1(NULL, NULL,
                              tls->keys.server_write_MAC_secret,
                              MAC_LENGTH,
                              mac_source_data,
                              8+5+length,
                              mac);

        if (CsrMemCmp(mac, data+length, MAC_LENGTH) == 0)
        {
            ret = TRUE;
        }
        else
        {
            sme_trace_error((TR_SECURITY_LIB, "check_mac(). MAC error."));
            sme_trace_hex((TR_SECURITY_LIB, TR_LVL_DEBUG, "check_mac() :: computed MAC", mac, MAC_LENGTH));
            sme_trace_hex((TR_SECURITY_LIB, TR_LVL_DEBUG, "check_mac() :: expected MAC", data+length, MAC_LENGTH));
        }
    }
    else
    {
        sme_trace_error((TR_SECURITY_LIB, "check_mac(). Memory allocation failure."));
    }

    CsrPfree(mac_source_data);
    return ret;
}

/**
 * @brief Parse a TLS change cypher spec message.
 *
 * @param[in/out]   data : a pointer to a pointer to the start of the applciation
 *                         data. Will be updated to the end of the packet.
 * @param[in]        tls : the tls_data context structure which we may update
 */
static void parse_change_cypher_spec(CsrUint8 **data, tls_data *tls)
{
    sme_trace_info((TR_SECURITY_LIB, "change cypher spec %d", **data));
    (*data)++;
    tls->cypher_suite = tls->pending_cypher_suite;
    /* Is this the correct place to reset the read sequence number? */
    CsrMemSet(tls->read_seq_no, 0, 8);
}

/**
 * @brief Parse a TLS alert message.
 *
 * @param[in/out]   data : a pointer to a pointer to the start of the applciation
 *                         data. Will be updated to the end of the packet.
 * @param[in]        tls : the tls_data context structure which we may update
 */
static void parse_alert(CsrUint8 **data, tls_data *tls)
{
#ifndef SME_TRACE_NO_ERROR
    CsrInt32 i=0;
    const char *alert_level[3] = {"warning", "fatal", "unknown"};
    const char *alert_description[] = {
        "\x00 close_notify",
        "\x0a unexpected_message",
        "\x14 bad_record_mac",
        "\x15 decryption_failed",
        "\x16 record_overflow",
        "\x1e decompression_failure",
        "\x28 handshake_failure",
        "\x2a bad_certificate",
        "\x2b unsupported_certificate",
        "\x2c certificate_revoked",
        "\x2d certificate_expired",
        "\x2e certificate_unknown",
        "\x2f illegal_parameter",
        "\x30 unknown_ca",
        "\x31 access_denied",
        "\x32 decode_error",
        "\x33 decrypt_error",
        "\x3c export_restriction",
        "\x46 protocol_version",
        "\x47 insufficient_security",
        "\x50 internal_error",
        "\x5a user_canceled",
        "\x64 no_renegotiation",
        "\xff unknown"
    };
    CsrUint32 warning_code = **data-1;
    (*data)++;
    if (warning_code > 2)
        warning_code = 2;
    while ((alert_description[i][0] != (char)0xff) && (alert_description[i][0] != **data))
        i++;
    sme_trace_error((TR_SECURITY_LIB,
                     "TLS ALERT! Level: %s. Description:%s.",
                     alert_level[warning_code], alert_description[i]+1));
    (*data)++;
#endif
}

/**
 * @brief Parse a handshake message block.
 *
 * @par Description
 *      Handles the whole TLS handshake protocol.
 *
 * @param[in/out]   data : a pointer to a pointer to the start of the applciation
 *                         data. Will be updated to the end of the packet.
 * @param[in]     length : the length of the total TLS packet containing the
 *                         application data.
 * @param[in]        tls : the tls_data context structure which we may update
 * @param[out] update_flags : flags to indicate which fields of tls have been
 *                            modified and what content has been discovered in
 *                            the packet.
 * @param[in]  decyphered_data : a pre-allocated workspace as big as the packet,
 *                               to use for decyphering_data.
 *
 * @return
 *      TRUE = parsed ok. FALSE = error.
 */
static CsrBool parse_handshake(void *clientContext,
                               CsrUint8 **data,
                               CsrUint16 tls_length,
                               tls_data *tls,
                               CsrUint32 *update_flags,
                               CsrUint8 *decyphered_data)
{
    CsrUint8 *remaining;
    CsrUint8 *start;
    CsrUint8 handshake_type;
    CsrUint32 handshake_length;
    CsrUint8 *handshake_start;
    CsrBool unencrypted = FALSE;

    sme_trace_info((TR_SECURITY_LIB, "handshake"));

    switch(tls->cypher_suite)
    {
        case TLS_RSA_WITH_RC4_128_SHA:
        {
            /* Handshake is encrypted with RC4-128 */
            CsrUint8 server_client_random[64];
            phase1_key_block kb;

            sme_trace_info((TR_SECURITY_LIB, "parse_handshake :: TLS_RSA_WITH_RC4_128_SHA"));
            /* Todo: move the key setting into the protocol-specific code?
             * Remove "phase1" from the TLS module. */

            /* This is the first encrypted message so calculate the
             * keys for the decypher which we will use from here on. */
            CsrMemCpy(server_client_random, &(tls->server_random), 32);
            CsrMemCpy(&(server_client_random[32]), &(tls->client_random), 32);
            t_prf(tls->PAC_key, 32,
                  "PAC to master secret label hash", 31,
                  server_client_random, 64,
                  tls->master_secret, TLS_MASTER_SECRET_LENGTH);
            prf(tls->master_secret, TLS_MASTER_SECRET_LENGTH,
                "key expansion", 13,
                server_client_random, 64,
                (CsrUint8 *)&(kb), sizeof(kb));

            /* Make the keying information current */
            CsrMemCpy(tls->keys.client_write_MAC_secret, kb.client_write_MAC_secret, MAC_LENGTH);
            CsrMemCpy(tls->keys.server_write_MAC_secret, kb.server_write_MAC_secret, MAC_LENGTH);
            CsrMemCpy(tls->keys.client_write_key, kb.client_write_key, 16);
            CsrMemCpy(tls->keys.server_write_key, kb.server_write_key, 16);
            CsrMemCpy(tls->keys.session_key_seed, kb.session_key_seed, 40);

            /* Set the key for the first time. */
            CsrRc4SetKey(&tls->rc4_server_write_key, 16, kb.server_write_key);

            /* Do the actual decypher */
            CsrRc4(&tls->rc4_server_write_key, 36, *data, decyphered_data);

            if (!check_mac(TLS_HANDSHAKE_PROTOCOL, decyphered_data, 4+12, tls))
                return FALSE;

            remaining = decyphered_data;
            *data += 36;
        }
        break;
    case TLS_DH_anon_WITH_AES_128_CBC_SHA:
        {
            sme_trace_info((TR_SECURITY_LIB, "parse_handshake :: TLS_DH_anon_WITH_AES_128_CBC_SHA"));
            /* Handshake is encrypted with AES-128-CBC */
            {
                CsrUint32 outLength;

                CsrCryptoAes128CbcDecrypt(tls->keys.server_write_key, tls->keys.server_write_IV,
                        *data, tls_length, decyphered_data, &outLength, PAD_MODE_TLS);
             }

            if (!check_mac(TLS_HANDSHAKE_PROTOCOL, decyphered_data, 4+12, tls))
                return FALSE;

            remaining = decyphered_data;
            *data += tls_length;
        }
        break;
    case TLS_RSA_WITH_3DES_EDE_CBC_SHA:
        {
            CsrKeySchedule ks1, ks2, ks3;
            CsrUint32 outLength;

            sme_trace_info((TR_SECURITY_LIB, "TLS_RSA_WITH_3DES_EDE_CBC_SHA cipher suite"));
            CsrCryptoDesSetKey(&tls->keys.server_write_key[0], &ks1);
            sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "3DES key 1", &tls->keys.server_write_key[0], 8));
            CsrCryptoDesSetKey(&tls->keys.server_write_key[8], &ks2);
            sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "3DES key 2", &tls->keys.server_write_key[8], 8));
            CsrCryptoDesSetKey(&tls->keys.server_write_key[16], &ks3);
            sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "3DES key 3", &tls->keys.server_write_key[16], 8));
            sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "3DES IV", tls->keys.server_write_IV, 8));
            CsrCrypto3DesCbcDecrypt(&ks1, &ks2, &ks3, tls->keys.server_write_IV, *data, tls_length, decyphered_data,
                    &outLength, PAD_MODE_TLS);

            if (!check_mac(TLS_HANDSHAKE_PROTOCOL, decyphered_data, 4+12, tls))
                return FALSE;

            remaining = decyphered_data;
            *data += tls_length;
        }
        break;
    default:
        sme_trace_info((TR_SECURITY_LIB, "parse_handshake :: Handshake is unencrypted"));
        /* Handshake is unencrypted */
        unencrypted = TRUE;
        remaining = *data;
        break;
    }

    start = remaining;

    /* This section may need to run more than once if there are multiple handshake messages in the TLS message */
    do
    {
        handshake_type = remaining[0];
        handshake_length = (remaining[1]<<16) + (remaining[2]<<8) + remaining[3];

        handshake_start = remaining;

        remaining += 4;

        switch (handshake_type)
        {
        case 2/*server hello*/:
            {
                CsrUint32 session_id_length;

                sme_trace_info((TR_SECURITY_LIB, "parse_handshake() :: Server version = %d %d.",
                                remaining[0], remaining[1]));
                if ((remaining[0] != 3) || (remaining[1] != 1))
                    return FALSE;
                remaining += 2;
                CsrMemCpy(&(tls->server_random), remaining, sizeof(Random));
                *update_flags |= SERVER_RANDOM_MODIFIED;
                remaining += sizeof(Random);
                session_id_length = *remaining++;
                sme_trace_info((TR_SECURITY_LIB, "parse_handshake() :: Session id length = %d.", session_id_length));
                if(session_id_length > 0 && session_id_length <= TLS_MAX_SESSION_ID_LENGTH)
                {
                    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "parse_handshake() :: Server session id", remaining, session_id_length));
                    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "parse_handshake() :: Client session id", tls->session_id, tls->session_id_length));
                    if(CsrMemCmp(tls->session_id, remaining, session_id_length) == 0)
                    {
                        sme_trace_info((TR_SECURITY_LIB, "parse_handshake() :: Session id match"));
                        tls->session_flags |= SESSION_ID_MATCH;
                    }
                    else
                    {
                        sme_trace_info((TR_SECURITY_LIB, "parse_handshake() :: Session id updated"));
                        CsrMemCpy(tls->session_id, remaining, session_id_length);
                        tls->session_id_length = session_id_length;
                        tls->session_flags |= SESSION_ID_UPDATED;
                    }
                }
                else
                {
                    /* Invalid session id length - ignore any cached session id */
                    tls->session_id_length = 0;
                }

                remaining += session_id_length;

                /* Why are the cypher suite and compression method fields
                 * not vectors? According to TLS RFC 2246 they should be.
                 * Empirical evidence shows they aren't, though. */
                sme_trace_info((TR_SECURITY_LIB, "parse_handshake() :: Cipher Suite = %d %d.",
                                remaining[0], remaining[1]));
                tls->pending_cypher_suite = (remaining[0]<<8) + remaining[1];
                *update_flags |= CYPHER_SUITE_MODIFIED;
                remaining += 2;
                if(tls->session_flags & SESSION_ID_MATCH)
                {
                    tls_key_expansion(tls);
                }
                sme_trace_info((TR_SECURITY_LIB, "parse_handshake() :: Compression Method = %d.", *remaining));
                if (*remaining++ != 0)
                    return FALSE;
            }
            break;
        case 11: /* Certificate */
            {
                CsrUint32 certificates_length;
                CsrUint32 certificate_length; /*lint -e550*/
                const CsrUint8 *der_encoding;

                sme_trace_info((TR_SECURITY_LIB, "parse_handshake() :: Certificate"));
                certificates_length = (remaining[0]<<16) + (remaining[1]<<8) + remaining[2];
                remaining += 3;
                sme_trace_info((TR_SECURITY_LIB, "parse_handshake() :: Certificates Length = %d", certificates_length));
                certificate_length = (remaining[0]<<16) + (remaining[1]<<8) + remaining[2];
                sme_trace_info((TR_SECURITY_LIB, "parse_handshake() :: Certificate Length = %d", certificate_length));

                /* Only interested in the first certificate which has the RSA Public Key for the peer */
                der_encoding = remaining + 3;

                tls->server_modulus_length = CsrCryptoAsn1GetRsaPublicModulus(&der_encoding);
                sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "parse_handshake() :: Server RSA Public Modulus", der_encoding, tls->server_modulus_length));
                CsrMemCpy(tls->server_modulus, der_encoding, tls->server_modulus_length);
                der_encoding += tls->server_modulus_length;

                tls->server_exponent_length = CsrCryptoAsn1GetRsaPublicExponent(&der_encoding);
                sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "parse_handshake() :: Server RSA Public Exponent", der_encoding, tls->server_exponent_length));
                CsrMemCpy(tls->server_exponent, der_encoding, tls->server_exponent_length);
                der_encoding += tls->server_exponent_length;

                remaining += certificates_length;
            }
            break;
        case 12/*server key exchange*/:
            {
                /* The Server Key Exchange message contains three elements:
                 *  1) dh_p - this is the server's public key
                 *  2) dh_g - Diffie-Hellman base
                 *  3) dh_y - the prime
                 */
                CsrUint16 dh_public_key_length;
                CsrUint16 dh_g_length;

                /* Read DH_p - Server prime number */
                CsrUint16 prime_length = (remaining[0]<<8) + remaining[1];
                sme_trace_info((TR_SECURITY_LIB, "Server prime length = %d.", prime_length));
                if (prime_length != 256)
                    return FALSE; /* unexpected server prime length */
                remaining += 2;
                CsrMemCpy(tls->server_prime, remaining, prime_length);
                *update_flags |= SERVER_PRIME_MODIFIED;
                remaining += prime_length;

                /* DH_g. The Diffie-Hellman base. Used for calculating our own public/private key pair. */
                dh_g_length = (remaining[0]<<8) + remaining[1];
                remaining+=2;
                if (dh_g_length != 1)
                {
                    sme_trace_error
                        ((TR_SECURITY_LIB,
                          "Unexpected Diffie-Hellman base length: %d.",
                          dh_g_length));
                    return FALSE;
                }
                tls->dh_g = *remaining++;
                *update_flags |= DH_G_MODIFIED;

                /* Read DH_Ys - Server DH public key */
                dh_public_key_length = (remaining[0]<<8) + remaining[1];
                sme_trace_info((TR_SECURITY_LIB,
                                "Server DH public key length = %d.",
                                dh_public_key_length));
                if (dh_public_key_length != 256)
                    return FALSE; /* unexpected key length */
                remaining += 2;
                CsrMemCpy(tls->server_dh_public_key, remaining, dh_public_key_length);
                *update_flags |= SERVER_DH_PUBLIC_KEY_MODIFIED;
                remaining += dh_public_key_length;
            }
            break;
        case 13: /* Certificate Request */
            sme_trace_info((TR_SECURITY_LIB, "parse_handshake() :: Certificate Request"));
            remaining += handshake_length;
            break;
        case 14/*done*/:
            sme_trace_info((TR_SECURITY_LIB, "parse_handshake() :: Server Done"));
            *update_flags |= SERVER_DONE;
            remaining += handshake_length;
            break;
        case 20/*finished*/:
            {
                /* check the finished contents */
                CsrUint8 seed[CSR_CRYPTO_MD5_DIGEST_LENGTH + CSR_SHA1_DIGEST_LENGTH];
                CsrUint8 expected[12];

                {
                    CSR_CRYPTO_MD5_CTX *md5_ctx = CsrCryptoCloneMd5Context(tls->md5_ctx);
                    CsrCryptoCallMd5Final(NULL, NULL,
                                          seed, md5_ctx);
                    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "parse_handshake() :: MD5 hash", seed, CSR_CRYPTO_MD5_DIGEST_LENGTH));
                }

                {
                    CSR_CRYPTO_SHA1_CTX *sha1_ctx = CsrCryptoCloneSha1Context(tls->sha1_ctx);
                    CsrCryptoCallSha1Final(NULL, NULL,
                                           &seed[CSR_CRYPTO_MD5_DIGEST_LENGTH], sha1_ctx);
                    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "parse_handshake() :: SHA1 hash", &seed[CSR_CRYPTO_MD5_DIGEST_LENGTH], CSR_SHA1_DIGEST_LENGTH));
                }

                prf(tls->master_secret, TLS_MASTER_SECRET_LENGTH,
                    "server finished", 15,
                    seed, CSR_CRYPTO_MD5_DIGEST_LENGTH + CSR_SHA1_DIGEST_LENGTH,
                    expected, 12);

                if (CsrMemCmp(decyphered_data+4, expected, 12) != 0)
                {
                    sme_trace_error
                        ((TR_SECURITY_LIB,
                          "parse_tls(). Unexpected server finished content."));
                    return FALSE;
                }
            }
            break;
        default:
            return FALSE; /* unexpected Handshake type */
        } /* switch (handshake_type) */

        CsrCryptoCallMd5Update(NULL, NULL,
                               tls->md5_ctx, handshake_start, 4+handshake_length);
        CsrCryptoCallSha1Update(NULL, NULL,
                                tls->sha1_ctx, handshake_start, 4+handshake_length);
    }
    while(unencrypted && remaining - start < tls_length);

    if (!tls->cypher_suite)
    {
        *data = remaining;
    }
    return TRUE;
}

/**
 * @brief Parse an application data block.
 *
 * @par Description
 *      Application data parsing is mostly the job of the specific protocol
 *      client. This function does the common decyphering and MAC checking
 *      but defers actual processing to the callback function.
 *
 * @param[in/out]   data : a pointer to a pointer to the start of the applciation
 *                         data. Will be updated to the end of the packet.
 * @param[in]     length : the length of the total TLS packet containing the
 *                         application data.
 * @param[in]        tls : the tls_data context structure which we may update
 * @param[out] update_flags : flags to indicate which fields of tls have been
 *                            modified and what content has been discovered in
 *                            the packet.
 * @param[in]   callback : callback function for protocol-specific data.
 * @param[in]  decyphered_data : a pre-allocated workspace as big as the packet,
 *                               to use for decyphering_data.
 *
 * @return
 *      TRUE = parsed ok. FALSE = error.
 */
static CsrBool parse_application_data(void *clientContext,
                                      CsrUint8 **data,
                                      CsrUint16 tls_length,
                                      tls_data *tls,
                                      CsrUint32 *update_flags,
                                      parse_tls_callback callback,
                                      CsrUint8 *decyphered_data)
{
    /* always decypher */
    CsrUint8 *scan_decyphered = decyphered_data;
    CsrUint32 outLength = 0;

    sme_trace_info((TR_SECURITY_LIB, "parse_application_data"));
    switch(tls->cypher_suite)
    {
    case TLS_RSA_WITH_RC4_128_SHA:
        sme_trace_info((TR_SECURITY_LIB, "parse_application_data :: TLS_RSA_WITH_RC4_128_SHA"));
        /* Don't set key again. Use the keying material left over
         * from the previous decypher operation. */
        CsrRc4(&tls->rc4_server_write_key, tls_length, *data, decyphered_data);
        sme_trace_info((TR_SECURITY_LIB, "parse_application_data :: tls_length = %d", tls_length));
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "parse_application_data :: decyphered_data", decyphered_data, tls_length));
        outLength = tls_length - MAC_LENGTH;
        break;
    case TLS_DH_anon_WITH_AES_128_CBC_SHA:
        sme_trace_info((TR_SECURITY_LIB, "parse_application_data :: TLS_DH_anon_WITH_AES_128_CBC_SHA"));
        CsrCryptoAes128CbcDecrypt(tls->keys.server_write_key, tls->keys.server_write_IV,
                *data, tls_length, decyphered_data, &outLength, PAD_MODE_TLS);
        break;

    case TLS_RSA_WITH_3DES_EDE_CBC_SHA:
        sme_trace_info((TR_SECURITY_LIB, "TLS_RSA_WITH_3DES_EDE_CBC_SHA cipher suite"));
        {
            CsrKeySchedule ks1, ks2, ks3;

            CsrCryptoDesSetKey(&tls->keys.server_write_key[0], &ks1);
            sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "3DES key 1", &tls->keys.server_write_key[0], 8));
            CsrCryptoDesSetKey(&tls->keys.server_write_key[8], &ks2);
            sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "3DES key 2", &tls->keys.server_write_key[8], 8));
            CsrCryptoDesSetKey(&tls->keys.server_write_key[16], &ks3);
            sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "3DES key 3", &tls->keys.server_write_key[16], 8));
            sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "3DES IV", tls->keys.server_write_IV, 8));
            CsrCrypto3DesCbcDecrypt(&ks1, &ks2, &ks3, tls->keys.server_write_IV, *data, tls_length, decyphered_data,
                    &outLength, PAD_MODE_TLS);
        }
        break;

    default:
        sme_trace_info((TR_SECURITY_LIB, "parse_application_data :: Unsupported cipher suite (%04x)", tls->cypher_suite));
        break;
    }

    /* Consume the application protocol messages. */
    sme_trace_debug((TR_SECURITY_LIB, "parse_application_data :: outLength = %d", outLength));
    if (!callback(clientContext,
                  &scan_decyphered,
                  (CsrUint16)outLength,
                  update_flags,
                  tls))
        return FALSE;

    sme_trace_debug((TR_SECURITY_LIB, "parse_application_data :: scan_decyphered - decyphered_data = %d", scan_decyphered - decyphered_data));
    /* Check the MAC here - need to be careful with the data length as
     * there may be padding on the end of the data */
    if (!check_mac(TLS_APPLICATION_DATA_PROTOCOL,
                   decyphered_data,
                   (CsrUint16)(scan_decyphered - decyphered_data),
                   tls))
        return FALSE;


    *data += tls_length;
    return TRUE;
}

/**
 * @brief Form TLS record with application data, encrypt and add MAC (TTLS, PEAP, FAST etc)
 *
 * @par Description
 *
 *
 * @pre
  *
 * @return
 *   void
 */
void BuildTlsApplicationData(eapMethod* method, CsrUint8 *data, CsrUint16 length,
                             CsrUint8 *output, CsrUint32 *outLength)
{
    CsrUint8 *macData = CsrPmalloc(length + 8 + 5 + CSR_SHA1_DIGEST_LENGTH);
    CsrUint8 *fillout, *startEnc;
    TlsContext* tlsContext = (TlsContext*)method->methodContext;

    sme_trace_entry((TR_SECURITY_LIB, "BuildTlsApplicationData"));

    *outLength = 0;

    inc_seq_no(tlsContext->tls.write_seq_no);

    CsrMemCpy(macData, tlsContext->tls.write_seq_no, 8);
    /* for next time */
    inc_seq_no(tlsContext->tls.write_seq_no);

    fillout = macData + 8;

    /* TLS record layer */
    *fillout++ = TLS_CONTENT_TYPE_APPLICATION_DATA;
    *fillout++ = TLS_PROTOCOL_VERSION_MAJOR;
    *fillout++ = TLS_PROTOCOL_VERSION_MINOR;
    *fillout++ = (length >> 8) & 0xff; /* length (u16) */
    *fillout++ = length & 0xff;

    CsrMemCpy(fillout, data, length);
    startEnc = fillout;
    fillout += length;

    /* MAC length and key length are same here */
    CsrCryptoCallHmacSha1(tlsContext->context->cryptoContext, NULL, tlsContext->tls.keys.client_write_MAC_secret,
            sizeof(tlsContext->tls.keys.client_write_MAC_secret), macData, (CsrUint32)(fillout - macData), fillout);

    fillout += CSR_SHA1_DIGEST_LENGTH;

    switch(tlsContext->tls.cypher_suite)
    {
    case TLS_RSA_WITH_3DES_EDE_CBC_SHA:
        sme_trace_info((TR_SECURITY_LIB, "TLS_RSA_WITH_3DES_EDE_CBC_SHA cipher suite"));
        {
            CsrKeySchedule ks1, ks2, ks3;
            CsrUint32 inLength;

            inLength = (CsrUint32)(fillout - startEnc);
            sme_trace_info((TR_SECURITY_LIB, "3DES plaintext length = %d", inLength));
            sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "3DES plaintext", startEnc, inLength));
            CsrCryptoDesSetKey(&tlsContext->tls.keys.client_write_key[0], &ks1);
            sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "3DES key 1", &tlsContext->tls.keys.client_write_key[0], 8));
            CsrCryptoDesSetKey(&tlsContext->tls.keys.client_write_key[8], &ks2);
            sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "3DES key 2", &tlsContext->tls.keys.client_write_key[8], 8));
            CsrCryptoDesSetKey(&tlsContext->tls.keys.client_write_key[16], &ks3);
            sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "3DES key 3", &tlsContext->tls.keys.client_write_key[16], 8));
            sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "3DES IV", tlsContext->tls.keys.client_write_IV, 8));
            CsrCrypto3DesCbcEncrypt(&ks1, &ks2, &ks3, tlsContext->tls.keys.client_write_IV, startEnc, inLength, (output + 5), outLength, PAD_MODE_TLS);
            sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "Finished encrypted", (output + 5), *outLength));
        }
        break;
    case TLS_RSA_WITH_AES_128_CBC_SHA:
        sme_trace_info((TR_SECURITY_LIB, "TLS_RSA_WITH_AES_128_CBC_SHA cipher suite"));
        {
            CsrUint32 inLength;

            inLength = (CsrUint32)(fillout - startEnc);
            sme_trace_info((TR_SECURITY_LIB, "AES128 plaintext length = %d", inLength));
            sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "AES128 plaintext", startEnc, inLength));
            CsrCryptoAes128CbcEncrypt(tlsContext->tls.keys.client_write_key, tlsContext->tls.keys.client_write_IV, startEnc, inLength, (output + 5), outLength, PAD_MODE_TLS);
            sme_trace_info((TR_SECURITY_LIB, "AES128 ciphertext length = %d", *outLength));
            sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "AES128 ciphertext", (output + 5), *outLength));
        }
        break;
    default:
        sme_trace_info((TR_SECURITY_LIB, "Unsupported cipher suite(%04x)", tlsContext->tls.cypher_suite));
        break;
    }

    /* Header for the output */
    output[0] = TLS_CONTENT_TYPE_APPLICATION_DATA;
    output[1] = TLS_PROTOCOL_VERSION_MAJOR;
    output[2] = TLS_PROTOCOL_VERSION_MINOR;
    output[3] = (*outLength >> 8) & 0xff;
    output[4] = *outLength & 0xff;

    *outLength += 5;

    CsrPfree(macData);
    return;
}

void tls_key_expansion(tls_data *tls)
{
    CsrUint8 random_seed[64];
    const char *label;
    CsrUint8 *keys_block, *keys_block_offset;
    CsrUint32 keys_block_length;

    CsrMemCpy(random_seed, &tls->server_random, 32);
    CsrMemCpy(&(random_seed[32]), &tls->client_random, 32);
    sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "tls_key_expansion() :: Server+Client random", random_seed, 64));

    label = "key expansion";
    switch(tls->pending_cypher_suite)
    {
    case TLS_RSA_WITH_AES_128_CBC_SHA:
    case TLS_RSA_WITH_RC4_128_SHA:
        keys_block_length = 144;
        keys_block = (CsrUint8*) CsrPmalloc(keys_block_length);
        prf(tls->master_secret, 48, label, CsrStrLen(label), random_seed, 64, keys_block, keys_block_length);
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "tls_key_expansion() :: key block", keys_block, keys_block_length));
        keys_block_offset = keys_block;
        CsrMemCpy(tls->keys.client_write_MAC_secret, keys_block_offset, 20);
        keys_block_offset += 20;
        CsrMemCpy(tls->keys.server_write_MAC_secret, keys_block_offset, 20);
        keys_block_offset += 20;
        CsrMemCpy(tls->keys.client_write_key, keys_block_offset, 16);
        keys_block_offset += 16;
        CsrMemCpy(tls->keys.server_write_key, keys_block_offset, 16);
        keys_block_offset += 16;
        CsrMemCpy(tls->keys.client_write_IV, keys_block_offset, 16);
        keys_block_offset += 16;
        CsrMemCpy(tls->keys.server_write_IV, keys_block_offset, 16);
        keys_block_offset += 16;
        CsrMemCpy(tls->keys.session_key_seed, keys_block_offset, 40);
        CsrPfree(keys_block);
        break;
    case TLS_RSA_WITH_3DES_EDE_CBC_SHA:
        keys_block_length = 104;
        keys_block = (CsrUint8*) CsrPmalloc(keys_block_length);
        prf(tls->master_secret, 48, label, CsrStrLen(label), random_seed, 64, keys_block, keys_block_length);
        sme_trace_hex((TR_SECURITY_LIB, TR_LVL_INFO, "tls_key_expansion() :: key block", keys_block, keys_block_length));
        keys_block_offset = keys_block;
        CsrMemCpy(tls->keys.client_write_MAC_secret, keys_block_offset, 20);
        keys_block_offset += 20;
        CsrMemCpy(tls->keys.server_write_MAC_secret, keys_block_offset, 20);
        keys_block_offset += 20;
        CsrMemCpy(tls->keys.client_write_key, keys_block_offset, 24);
        keys_block_offset += 24;
        CsrMemCpy(tls->keys.server_write_key, keys_block_offset, 24);
        keys_block_offset += 24;
        CsrMemCpy(tls->keys.client_write_IV, keys_block_offset, 8);
        keys_block_offset += 8;
        CsrMemCpy(tls->keys.server_write_IV, keys_block_offset, 8);
        CsrPfree(keys_block);
        break;
    default:
        sme_trace_info((TR_SECURITY_LIB, "tls_key_expansion() :: Unsupported cipher suite (%04x)", tls->pending_cypher_suite));
        break;
    }
}

/* Install a session ID */
void install_tls_session(TlsContext* tlsContext)
{
    if (tlsContext->tls.session_flags & SESSION_ID_UPDATED)
    {
        sme_trace_info((TR_SECURITY_LIB, "install_tls_session() :: SESSION_ID_UPDATED"));
        CsrMemCpy(tlsContext->key.keydata, tlsContext->tls.master_secret, TLS_MASTER_SECRET_LENGTH);
        CsrMemCpy(tlsContext->key.keydata + TLS_MASTER_SECRET_LENGTH, tlsContext->tls.session_id, tlsContext->tls.session_id_length);
        tlsContext->key.length = TLS_MASTER_SECRET_LENGTH + tlsContext->tls.session_id_length;
        (tlsContext->context->callbacks.installSession)(tlsContext->context->externalContext,
                                                        tlsContext->key.keydata,
                                                        tlsContext->key.length);
    }
}

/*
 * See header file for description.
 * ------------------------------------------------------------------------- */
CsrBool parse_tls (void *clientContext,
                   CsrUint8 *packet,
                   CsrUint32 length,
                   tls_data *tls,
                   CsrUint32 *update_flags,
                   parse_tls_callback callback)
{
    CsrUint8 *scan = packet;

    *update_flags = 0;

    while (scan < packet + length)
    {
        CsrUint8 content;
        CsrUint16 tls_length;
        CsrUint8 *workspace;

        /* Read TLS header - this is never encrypted. */
        content = *scan++;
        sme_trace_info((TR_SECURITY_LIB, "TLS type = %d.", content));
        sme_trace_info((TR_SECURITY_LIB, "TLS protocol version = %d %d.", scan[0], scan[1]));
        if (scan[0] != 3) return FALSE;
        if (scan[1] != 1) return FALSE;
        scan += 2;
        tls_length = (scan[0] << 8) + scan[1];
        scan += 2;
        sme_trace_info((TR_SECURITY_LIB, "TLS length = %d.", tls_length));
        if ((scan + tls_length) > packet + length)
            return FALSE;

        /* Process the content - this MAY be encrypted, depending on the
         * content and the current cypher. */
        switch (content)
        {
        case TLS_CHANGE_CYPHER_SPEC_PROTOCOL:
            sme_trace_info((TR_SECURITY_LIB, "Change cipher spec"));
            parse_change_cypher_spec(&scan, tls);
            *update_flags |= CHANGE_CYPHER_SPEC_INCLUDED;
            break;

        case TLS_ALERT_PROTOCOL:
            parse_alert(&scan, tls);
            break;

        case TLS_HANDSHAKE_PROTOCOL:
            if ((workspace = CsrPmalloc(tls_length)) == NULL)
            {
                sme_trace_error((TR_SECURITY_LIB, "memory allocation failure"));
                return FALSE;
            }
            if (!parse_handshake(clientContext, &scan, tls_length, tls, update_flags, workspace))
            {
                CsrPfree(workspace);
                return FALSE;
            }
            CsrPfree(workspace);
            break;

        case TLS_APPLICATION_DATA_PROTOCOL:
            if (callback == NULL)
            {
                sme_trace_error
                    ((TR_SECURITY_LIB,
                      "No callback for application-specific TLS data."));
                return FALSE;
            }
            if ((workspace = CsrPmalloc(tls_length)) == NULL)
            {
                sme_trace_error((TR_SECURITY_LIB, "memory allocation failure."));
                return FALSE;
            }
            if (!parse_application_data(clientContext, &scan, tls_length, tls,
                                        update_flags, callback, workspace))
            {
                CsrPfree(workspace);
                return FALSE;
            }
            CsrPfree(workspace);
            break;

        default:
            sme_trace_warn((TR_SECURITY_LIB, "Unexpected TLS record type"));
            return FALSE; /* unexpected TLS record type */
        } /* switch (content) */

        if(content != TLS_CHANGE_CYPHER_SPEC_PROTOCOL)
        {
            /* Increment the read sequence number */
            inc_seq_no(tls->read_seq_no);
        }
    } /* while (scan < packet + length) */

    return TRUE;
}
