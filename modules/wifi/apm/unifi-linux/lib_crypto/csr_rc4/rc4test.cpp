// rc4test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <cstdio>

#include "csr_rc4.h"
#include "types.h"


static const unsigned char arc4_test_key[3][8] =
{
    { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF },
    { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
};

static const unsigned char arc4_test_pt[3][8] =
{
    { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
};

static const unsigned char arc4_test_ct[3][8] =
{
    { 0x75, 0xB7, 0x87, 0x80, 0x99, 0xE0, 0xC5, 0x96 },
    { 0x74, 0x94, 0xC2, 0xE7, 0x10, 0x4B, 0x08, 0x79 },
    { 0xDE, 0x18, 0x89, 0x41, 0xA3, 0x37, 0x5D, 0x3A }
};

CsrInt32 _tmain(int argc, _TCHAR* argv[])
{
    CsrUint8 encypted[100];
    CsrUint8 decypted[100];
    CsrUint8 i;

    CsrRc4Key rc4Key;

    printf( "original was: ");
    for (i=0; i<8; i++)
        printf("%02x ", arc4_test_key[0][i]);

    printf( "\n");

    csr_rc4_set_key(&rc4Key, 8, arc4_test_key[0]);
    csr_rc4(&rc4Key, 8, arc4_test_pt[0], encypted);


    printf( "encypted was: ");
    for (i=0; i<8; i++)
        printf("%02x ", encypted[i]);

    printf( "\n");

    csr_rc4_set_key(&rc4Key, 8, arc4_test_key[0]);
    csr_rc4(&rc4Key, 8, encypted, decypted);


printf( "decypted was: ");
    for (i=0; i<8; i++)
        printf("%02x ", decypted[i]);

    printf( "\n");


    





    std::puts("Press any key to continue...");
    std::getchar();

    return 0;
}

