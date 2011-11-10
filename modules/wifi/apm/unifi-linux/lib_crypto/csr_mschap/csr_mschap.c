/** @file csr_mschap.c
 *
 * MSCHAPv2 cryptographic processing.
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
 *   Microsoft PPP CHAP extensions (v2) are defined in RFC 2759. This file
 *   contains the functions that perform the appropriate cryptographic
 *   operations.
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_crypto/csr_mschap/csr_mschap.c#1 $
 *
 ****************************************************************************/

#include "csr_mschap.h"
#include "csr_types.h"
#include "csr_util.h"
#include "sme_trace/sme_trace.h"
#include "csr_sha1/csr_sha1.h"
#include "csr_des/csr_des.h"
#include "csr_md4/csr_md4.h"
/**
 * @brief Generate the challenge hash as per RFC 2759.
 *
 * @param[in]   peer_challenge : our 16-octet challenge
 * @param[in]   auth_challenge : the 16-octet challenge from the authenticator
 * @param[in]   username       : the null-terminated unicode username
 * @param[out]  challenge      : to hold the 8-octet result.
 */
static void ChallengeHash(CsrUint8 *peer_challenge,
                          CsrUint8 *auth_challenge,
                          CsrUint8 *username,
                          CsrUint8 *challenge)
{
    CSR_CRYPTO_SHA1_CTX *sha1_ctx;
    CsrUint8 digest[CSR_SHA1_DIGEST_LENGTH];

    sha1_ctx = CsrCryptoSha1Init();
    CsrCryptoSha1Update(sha1_ctx, peer_challenge, 16);
    CsrCryptoSha1Update(sha1_ctx, auth_challenge, 16);

    /*
     * Only the user name (as presented by the peer and
     * excluding any prepended domain name)
     * is used as input to SHAUpdate().
     */

    CsrCryptoSha1Update(sha1_ctx, username, CsrStrLen((char *)username));
    CsrCryptoSha1Final(digest, sha1_ctx);

    CsrMemCpy(challenge, digest, 8);
}

/**
 * @brief Prepare a key for use with the DES algorithm.
 *
 * @par Description
 *   This function takes a 7-bit key as input and outputs an 8-bit key which
 *   has parity bits added, as per DES encryption key requirements.
 *   (Note: in practice the parity bits will never be used so we can set them
 *    to whatever we like)
 *
 * @param[in]   in : 7-octet key.
 * @param[out] out : 8-octet key including parity bits in every LSB.
 */
static void prepare_des_key(CsrUint8 *in, CsrUint8 *out)
{
    CsrUint8 i;

    for (i = 0; i < 8; i++)
    {
        out[i] = (CsrUint8)(i!=0?in[i-1]<<(8-i):0) | (CsrUint8)(i!=7?in[i]>>i:0) | 1;
    }
}

/*
 * Description:
 * See description in eapfast_inband_provisioning.h
 *
 *---------------------------------------------------------------------------*/
void CsrCryptoMschapChallengeResponse(CsrUint8 *challenge, CsrUint8 *hash, CsrUint8 *response)
{
    CsrKeySchedule ks;
    CsrUint8 pwd_hash_extension[7];
    CsrUint8 deskey[8];

    /* See RFC:2759 section 8.5 */
    prepare_des_key(hash, deskey);
    CsrCryptoDesSetKey(deskey, &ks);
    CsrCryptoDesEcbEncrypt(challenge, &ks, response);

    prepare_des_key(hash+7, deskey);
    CsrCryptoDesSetKey(deskey, &ks);
    CsrCryptoDesEcbEncrypt(challenge, &ks, response+8);

    pwd_hash_extension[0] = hash[14];
    pwd_hash_extension[1] = hash[15];
    CsrMemSet(pwd_hash_extension + 2, 0, 5);

    prepare_des_key(pwd_hash_extension, deskey);
    CsrCryptoDesSetKey(deskey, &ks);
    CsrCryptoDesEcbEncrypt(challenge, &ks, response+16);
}

/*
 * Description:
 * See description in eapfast_inband_provisioning.h
 *
 *---------------------------------------------------------------------------*/
void CsrCryptoMschapNtPasswordHash(CsrUint8 *password, CsrUint8 *hash)
{
    CsrInt32 i;
    CsrUint8 unicode_pwd[200];

    CsrMemSet(unicode_pwd, 0, sizeof(unicode_pwd));
    /* Unicode the password. */
    for (i=0; i<(int)CsrStrLen((char *)password); i++)
    {
        unicode_pwd[i<<1] = password[i];
        unicode_pwd[(i<<1)+1] = 0;
    }

    /*
     * Use the MD4 algorithm [5] to irreversibly hash Password
     * into PasswordHash.  Only the password is hashed without
     * including any terminating 0.
     */
    CsrCryptoMd4(unicode_pwd, CsrStrLen((char *)password)<<1, hash);
}

/* Defined in RFC:3079 */
static void GetMasterKey(CsrUint8 *PasswordHashHash,
                         CsrUint8 *NTResponse,
                         CsrUint8 *MasterKey)
{
    CSR_CRYPTO_SHA1_CTX *sha1_ctx;
    CsrUint8 Digest[20];
    CsrUint8 Magic1[27] =
    {
        0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x74,
        0x68, 0x65, 0x20, 0x4d, 0x50, 0x50, 0x45, 0x20, 0x4d,
        0x61, 0x73, 0x74, 0x65, 0x72, 0x20, 0x4b, 0x65, 0x79
    };

    CsrMemSet(Digest, 0, 20);

    sha1_ctx = CsrCryptoSha1Init();
    CsrCryptoSha1Update(sha1_ctx, PasswordHashHash, 16);
    CsrCryptoSha1Update(sha1_ctx, NTResponse, 24);
    CsrCryptoSha1Update(sha1_ctx, Magic1, 27);
    CsrCryptoSha1Final(Digest, sha1_ctx);

    CsrMemCpy(MasterKey, Digest, 16);
}

/* Defined in RFC:3079 */
static void GetAsymetricStartKey(CsrUint8 *MasterKey,
                                 CsrUint8 *SessionKey,
                                 CsrUint32 SessionKeyLength,
                                 CsrBool IsSend,
                                 CsrBool IsServer)
{
    CSR_CRYPTO_SHA1_CTX *sha1_ctx;
    CsrUint8 Digest[20];
    CsrUint8 *s;
    CsrUint8 SHSpad1[40]; /* clear to zero below */
    CsrUint8 SHSpad2[40]; /* set to 0xf2 below */
    CsrUint8 Magic2[84] =
    {
        0x4f, 0x6e, 0x20, 0x74, 0x68, 0x65, 0x20, 0x63, 0x6c, 0x69,
        0x65, 0x6e, 0x74, 0x20, 0x73, 0x69, 0x64, 0x65, 0x2c, 0x20,
        0x74, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x74, 0x68,
        0x65, 0x20, 0x73, 0x65, 0x6e, 0x64, 0x20, 0x6b, 0x65, 0x79,
        0x3b, 0x20, 0x6f, 0x6e, 0x20, 0x74, 0x68, 0x65, 0x20, 0x73,
        0x65, 0x72, 0x76, 0x65, 0x72, 0x20, 0x73, 0x69, 0x64, 0x65,
        0x2c, 0x20, 0x69, 0x74, 0x20, 0x69, 0x73, 0x20, 0x74, 0x68,
        0x65, 0x20, 0x72, 0x65, 0x63, 0x65, 0x69, 0x76, 0x65, 0x20,
        0x6b, 0x65, 0x79, 0x2e
    };
    CsrUint8 Magic3[84] =
    {
        0x4f, 0x6e, 0x20, 0x74, 0x68, 0x65, 0x20, 0x63, 0x6c, 0x69,
        0x65, 0x6e, 0x74, 0x20, 0x73, 0x69, 0x64, 0x65, 0x2c, 0x20,
        0x74, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x74, 0x68,
        0x65, 0x20, 0x72, 0x65, 0x63, 0x65, 0x69, 0x76, 0x65, 0x20,
        0x6b, 0x65, 0x79, 0x3b, 0x20, 0x6f, 0x6e, 0x20, 0x74, 0x68,
        0x65, 0x20, 0x73, 0x65, 0x72, 0x76, 0x65, 0x72, 0x20, 0x73,
        0x69, 0x64, 0x65, 0x2c, 0x20, 0x69, 0x74, 0x20, 0x69, 0x73,
        0x20, 0x74, 0x68, 0x65, 0x20, 0x73, 0x65, 0x6e, 0x64, 0x20,
        0x6b, 0x65, 0x79, 0x2e
    };

    CsrMemSet(Digest, 0, 20);
    CsrMemSet(SHSpad1, 0, 40);
    CsrMemSet(SHSpad2, 0xf2, 40);

    if (IsSend)
    {
        if (IsServer)
            s = Magic3;
        else
            s = Magic2;
    }
    else
    {
        if (IsServer)
            s = Magic2;
        else
            s = Magic3;
    }

    sha1_ctx = CsrCryptoSha1Init();
    CsrCryptoSha1Update(sha1_ctx, MasterKey, 16);
    CsrCryptoSha1Update(sha1_ctx, SHSpad1, 40);
    CsrCryptoSha1Update(sha1_ctx, s, 84);
    CsrCryptoSha1Update(sha1_ctx, SHSpad2, 40);
    CsrCryptoSha1Final(Digest, sha1_ctx);

    CsrMemCpy(SessionKey, Digest, SessionKeyLength);
}

/*
 * Description:
 * See description in eapfast_inband_provisioning.h
 *
 *---------------------------------------------------------------------------*/
void CsrCryptoMschapGenerateNTResponseAndSessionKey(CsrUint8 *auth_challenge,
                                     CsrUint8 *peer_challenge,
                                     CsrUint8 *username,
                                     CsrUint8 *password,
                                     CsrUint8 *response,
                                     CsrUint8 *session_key)
{
    CsrUint8 challenge[8];
    CsrUint8 password_hash[16];
    CsrUint8 password_hash_hash[16];
    CsrUint8 master_key[16];

    /* Generate the response */
    ChallengeHash(peer_challenge, auth_challenge, username, challenge);
    CsrCryptoMschapNtPasswordHash(password, password_hash);
    CsrCryptoMschapChallengeResponse(challenge, password_hash, response);

    /* Generate the session key */
    CsrCryptoMd4(password_hash, 16, password_hash_hash);
    GetMasterKey(password_hash_hash, response, master_key);
    GetAsymetricStartKey(master_key, session_key, 16, TRUE, TRUE);
    GetAsymetricStartKey(master_key, session_key+16, 16, FALSE, TRUE);
}

/* Defined in RFC:2759 */
void GenerateAuthenticatorResponse(CsrUint8 *auth_challenge,
                                   CsrUint8 *peer_challenge,
                                   CsrUint8 *username,
                                   CsrUint8 *password,
                                   CsrUint8 *nt_response,
                                   CsrUint8 *auth_response)
{
    CsrUint8 challenge[8];
    CsrUint8 password_hash[16];
    CsrUint8 password_hash_hash[16];
    CsrUint8 i, digest[20];
    CSR_CRYPTO_SHA1_CTX *sha1_ctx;
    const CsrUint8 charArray[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

    /*
     * "Magic" constants used in response generation
     */

    CsrUint8 Magic1[39] =
         {0x4D, 0x61, 0x67, 0x69, 0x63, 0x20, 0x73, 0x65, 0x72, 0x76,
          0x65, 0x72, 0x20, 0x74, 0x6F, 0x20, 0x63, 0x6C, 0x69, 0x65,
          0x6E, 0x74, 0x20, 0x73, 0x69, 0x67, 0x6E, 0x69, 0x6E, 0x67,
          0x20, 0x63, 0x6F, 0x6E, 0x73, 0x74, 0x61, 0x6E, 0x74};

    CsrUint8 Magic2[41] =
         {0x50, 0x61, 0x64, 0x20, 0x74, 0x6F, 0x20, 0x6D, 0x61, 0x6B,
          0x65, 0x20, 0x69, 0x74, 0x20, 0x64, 0x6F, 0x20, 0x6D, 0x6F,
          0x72, 0x65, 0x20, 0x74, 0x68, 0x61, 0x6E, 0x20, 0x6F, 0x6E,
          0x65, 0x20, 0x69, 0x74, 0x65, 0x72, 0x61, 0x74, 0x69, 0x6F,
          0x6E};

      /*
       * Hash the password with MD4
       */

    CsrCryptoMschapNtPasswordHash(password, password_hash);

      /*
       * Now hash the hash
       */

    CsrCryptoMd4(password_hash, 16, password_hash_hash);

    CsrMemSet(digest, 0, 20);

    sha1_ctx = CsrCryptoSha1Init();
    CsrCryptoSha1Update(sha1_ctx, password_hash_hash, 16);
    CsrCryptoSha1Update(sha1_ctx, nt_response, 24);
    CsrCryptoSha1Update(sha1_ctx, Magic1, 39);
    CsrCryptoSha1Final(digest, sha1_ctx);

    ChallengeHash(peer_challenge, auth_challenge, username, challenge);

    sha1_ctx = CsrCryptoSha1Init();
    CsrCryptoSha1Update(sha1_ctx, digest, 20);
    CsrCryptoSha1Update(sha1_ctx, challenge, 8);
    CsrCryptoSha1Update(sha1_ctx, Magic2, 41);
    CsrCryptoSha1Final(digest, sha1_ctx);

    /*
     * Encode the value of 'Digest' as "S=" followed by
     * 40 ASCII hexadecimal digits and return it in
     * AuthenticatorResponse.
     * For example,
     *   "S=0123456789ABCDEF0123456789ABCDEF01234567"
     */
    auth_response[0] = 'S'; auth_response[1] = '=';
    for (i = 0; i < 20; i++)
    {
        auth_response[2 + i*2] = charArray[((digest[i] & 0xf0) >> 4)];
        auth_response[2 + i*2 + 1] = charArray[(digest[i] & 0x0f)];
    }
}
