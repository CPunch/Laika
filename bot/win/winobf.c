#include "obf.h"

/*
    Most of this file was adapted from
    https://github.com/LloydLabs/Windows-API-Hashing/blob/master/resolve.c

    Checkout their repository! All I did was minor formatting changes and some misc. cleanup
*/

#include <process.h>
#include <windows.h>

/* ======================================[[ API Hashing ]]====================================== */

#define RESOLVE_NAME_MAX       4096
#define RESOLVE_REL_CALC(x, y) ((LPBYTE)x + y)

/*
    getHashName(LPCSTR) -> uint32_t
    uses the SuperFastHash algorithm to create an unsigned 32-bit hash
*/
uint32_t getHashName(LPCSTR cszName)
{
    SIZE_T uNameLen, i;
    PBYTE pbData = (PBYTE)cszName;
    uint32_t u32Hash = 0, u32Buf = 0;
    INT iRemain;

    if (cszName == NULL)
        return 0;

    if ((uNameLen = strnlen_s(cszName, RESOLVE_NAME_MAX)) == 0)
        return 0;

    iRemain = (uNameLen & 3);
    uNameLen >>= 2;

    for (i = uNameLen; i > 0; i--) {
        u32Hash += *(const UINT16 *)pbData;
        u32Buf = (*(const UINT16 *)(pbData + 2) << 11) ^ u32Hash;
        u32Hash = (u32Hash << 16) ^ u32Buf;
        pbData += (2 * sizeof(UINT16));
        u32Hash += u32Hash >> 11;
    }

    switch (iRemain) {
    case 1:
        u32Hash += *pbData;
        u32Hash ^= u32Hash << 10;
        u32Hash += u32Hash >> 1;
        break;
    case 2:
        u32Hash += *(const UINT16 *)pbData;
        u32Hash ^= u32Hash << 11;
        u32Hash += u32Hash >> 17;
        break;
    case 3:
        u32Hash += *(const UINT16 *)pbData;
        u32Hash ^= u32Hash << 16;
        u32Hash ^= pbData[sizeof(UINT16)] << 18;
        u32Hash += u32Hash >> 11;
        break;
    }

    u32Hash ^= u32Hash << 3;
    u32Hash += u32Hash >> 5;
    u32Hash ^= u32Hash << 4;
    u32Hash += u32Hash >> 17;
    u32Hash ^= u32Hash << 25;
    u32Hash += u32Hash >> 6;

    return u32Hash;
}

/* fork of the resolve_find() with the weird struct stripped. also library cleanup for the fail
    condition was added */
void *findByHash(LPCSTR module, uint32_t hash)
{
    HMODULE hLibrary;
    PIMAGE_DOS_HEADER pDOSHdr;
    PIMAGE_NT_HEADERS pNTHdr;
    PIMAGE_EXPORT_DIRECTORY pIED;
    PDWORD pdwAddress, pdwNames;
    PWORD pwOrd;

    if ((hLibrary = LoadLibraryA(module)) == NULL)
        return NULL;

    /* 
        the rest of this function just does the same thing GetProcAddress() does, but using
        our hash function to find the right function. this is also more obfuscated to the
        REer, however they would probably immediately recognize what this function is doing
        just from the LoadLibraryA() call.
    */

    /* grab DOS headers & verify */
    pDOSHdr = (PIMAGE_DOS_HEADER)hLibrary;
    if (pDOSHdr->e_magic != IMAGE_DOS_SIGNATURE)
        goto _findByHashFail;

    /* grab NT headers & verify */
    pNTHdr = (PIMAGE_NT_HEADERS)RESOLVE_REL_CALC(hLibrary, pDOSHdr->e_lfanew);
    if (pNTHdr->Signature != IMAGE_NT_SIGNATURE)
        goto _findByHashFail;

    /* verify that this NT file is a DLL & actually exports functions */
    if ((pNTHdr->FileHeader.Characteristics & IMAGE_FILE_DLL) == 0 ||
        pNTHdr->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress == 0 ||
        pNTHdr->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size == 0)
        goto _findByHashFail;

    pIED = (PIMAGE_EXPORT_DIRECTORY)RESOLVE_REL_CALC(
        hLibrary,
        pNTHdr->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

    pdwAddress = (PDWORD)RESOLVE_REL_CALC(hLibrary, pIED->AddressOfFunctions);
    pdwNames = (PDWORD)RESOLVE_REL_CALC(hLibrary, pIED->AddressOfNames);
    pwOrd = (PWORD)RESOLVE_REL_CALC(hLibrary, pIED->AddressOfNameOrdinals);

    /* walk library export table, compare hashes until we find a match */
    for (DWORD i = 0; i < pIED->AddressOfFunctions; i++) {
        if (getHashName((LPCSTR)RESOLVE_REL_CALC(hLibrary, pdwNames[i])) == hash)
            /* return the pointer to our function. we don't worry about closing the library's
                handle because we'll need it loaded until we exit. */
            return (void *)RESOLVE_REL_CALC(hLibrary, pdwAddress[pwOrd[i]]);
    }

_findByHashFail:
    /* function was not found, close the library handle since we don't need it anymore */
    FreeLibrary(hLibrary);
    return NULL;
}

#undef RESOLVE_REL_CALC

/* ======================================[[ Exposed API ]]====================================== */

_ShellExecuteA oShellExecuteA;
_CreatePseudoConsole oCreatePseudoConsole;
_ClosePseudoConsole oClosePseudoConsole;
_CreateProcessA oCreateProcessA;

/* TODO:
    GetEnvironmentVariable

    windows registry related API
*/

void laikaO_init()
{
    uint32_t hash;

    /* TODO: these library strings should probably be obfuscated (by a skid box maybe?) */
    oShellExecuteA = (_ShellExecuteA)findByHash("shell32.dll", 0x89858cd3);
    oCreatePseudoConsole = (_CreatePseudoConsole)findByHash("kernel32.dll", 0x7310ef7);
    oClosePseudoConsole = (_ClosePseudoConsole)findByHash("kernel32.dll", 0xeff42590);
    oCreateProcessA = (_CreateProcessA)findByHash("kernel32.dll", 0x9e687c1d);

/*
    hash = getHashName("InitializeProcThreadAttributeList");
    printf("InitializeProcThreadAttributeList: real is %p, hashed is %p. [HASH: %x]\n",
           (void *)InitializeProcThreadAttributeList,
           findByHash("kernel32.dll", hash), hash);
*/
}