/*
 * Copyright 2017 transmission.aquitaine@yahoo.com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "addr_width.h"
#include "basic_types.h"
#include "zprintf_etc.h"



#ifdef IS64BIT
#ifdef IS_INTEL_ENDIAN

inline uinta byteOf1stBitNZ(uinta x) // assumes x != 0
    {
    if (x & 0x00000000FFFFFFFF)
        {
        if (x & 0x000000000000FFFF)
            return x & 0x00000000000000FF ? 0 : 1;
        else
            return x & 0x0000000000FF0000 ? 2 : 3;
        }
    else
        {
        if (x & 0x0000FFFF00000000)
            return x & 0x000000FF00000000 ? 4 : 5;
        else
            return x & 0x00FF000000000000 ? 6 : 7;
        }
    }

static uinta startMasks[9] =
    {
    0xFFFFFFFFFFFFFFFF,
    0xFFFFFFFFFFFFFF00,
    0xFFFFFFFFFFFF0000,
    0xFFFFFFFFFF000000,
    0xFFFFFFFF00000000,
    0xFFFFFF0000000000,
    0xFFFF000000000000,
    0xFF00000000000000,
    0x0000000000000000
    };

static uinta endMasks[9] =
    {
    0x0000000000000000,
    0x00000000000000FF,
    0x000000000000FFFF,
    0x0000000000FFFFFF,
    0x00000000FFFFFFFF,
    0x000000FFFFFFFFFF,
    0x0000FFFFFFFFFFFF,
    0x00FFFFFFFFFFFFFF,
    0xFFFFFFFFFFFFFFFF
    };

#else

inline uinta byteOf1stBitNZ(uinta x) // assumes x != 0
    {
    if (x & 0xFFFFFFFF00000000)
        {
        if (x & 0xFFFF000000000000)
            return x & 0xFF00000000000000 ? 0 : 1;
        else
            return x & 0x0000FF0000000000 ? 2 : 3;
        }
    else
        {
        if (x & 0x00000000FFFF0000)
            return x & 0x00000000FF000000 ? 4 : 5;
        else
            return x & 0x000000000000FF00 ? 6 : 7;
        }
    }

static uinta startMasks[9] =
    {
    0xFFFFFFFFFFFFFFFF,
    0x00FFFFFFFFFFFFFF,
    0x0000FFFFFFFFFFFF,
    0x000000FFFFFFFFFF,
    0x00000000FFFFFFFF,
    0x0000000000FFFFFF,
    0x000000000000FFFF,
    0x00000000000000FF,
    0x0000000000000000
    };

static uinta endMasks[9] =
    {
    0x0000000000000000,
    0xFF00000000000000,
    0xFFFF000000000000,
    0xFFFFFF0000000000,
    0xFFFFFFFF00000000,
    0xFFFFFFFFFF000000,
    0xFFFFFFFFFFFF0000,
    0xFFFFFFFFFFFFFF00,
    0xFFFFFFFFFFFFFFFF
    };

#endif
#else
#ifdef IS_INTEL_ENDIAN

inline uinta byteOf1stBitNZ(uinta x) // assumes x != 0
    {
    if (x & 0x0000FFFF)
        return x & 0x000000FF ? 0 : 1;
    else
        return x & 0x00FF0000 ? 2 : 3;
    }

static uinta startMasks[5] =
    {
    0xFFFFFFFF,
    0xFFFFFF00,
    0xFFFF0000,
    0xFF000000,
    0x00000000
    };

static uinta endMasks[5] =
    {
    0x00000000,
    0x000000FF,
    0x0000FFFF,
    0x00FFFFFF,
    0xFFFFFFFF,
    };

#else

inline uinta byteOf1stBitNZ(uinta x) // assumes x != 0
    {
    if (x & 0xFFFF0000)
        return x & 0xFF000000 ? 0 : 1;
    else
        return x & 0x0000FF00 ? 2 : 3;
    }

static uinta startMasks[5] =
    {
    0xFFFFFFFF,
    0x00FFFFFF,
    0x0000FFFF,
    0x000000FF,
    0x00000000,
    };

static uinta endMasks[5] =
    {
    0x00000000,
    0xFF000000,
    0xFFFF0000,
    0xFFFFFF00,
    0xFFFFFFFF,
    };

#endif
#endif

#define WORDSIZE   ((uinta)(sizeof(void*)))
#define OFFSETMASK ((uinta)(WORDSIZE - 1))
#define WORDMASK   ((uinta)(~OFFSETMASK))

inline uinta compareBytesAligned(const uint8 *floor1, const uint8 *floor2, uinta offset, uinta len)
    {
    const uinta *p1, *p2;
    if (offset)
        {
        p1 = (uinta*)((uinta)floor1 & WORDMASK);
        p2 = (uinta*)((uinta)floor2 & WORDMASK);
        uinta x = startMasks[offset] & (*p1++ ^ *p2++);
        if (x) return byteOf1stBitNZ(x) - offset;
        }
    else
        {
        p1 = (uinta*)floor1;
        p2 = (uinta*)floor2;
        }
    const uint8 *roof1 = floor1 + len;
    const uinta *p1Roof = (uinta*)((uinta)roof1 & WORDMASK);
    if (p1 < p1Roof)
        {
        uinta x;
        do x = *p1++ ^ *p2++; while (!x && p1 != p1Roof);
        if (x) return byteOf1stBitNZ(x) + (uint8*)(p1 - 1) - floor1;
        }
    offset = roof1 - (uint8*)p1Roof;
    if (offset)
        {
        uinta x = endMasks[offset] & (*p1 ^ *p2);
        if (x) return byteOf1stBitNZ(x) + len - offset;
        }
    return len;
    }

inline uinta compareBytes2Ahead(const uint8 *floor1, uinta offset1, const uint8 *floor2, uinta offset2, uinta len) // aasumes offset1 < offset2
    {
    const uinta *p1, *p2;
    uinta p2Value = 0;
    uinta p2Pending = 0;
    offset2 -= offset1;
    uinta rShift = 8*offset2;
    uinta lShift = 8*(WORDSIZE - offset2);
    if (offset1)
        {
        p1 = (uinta*)((uinta)floor1 & WORDMASK);
        p2 = (uinta*)((uinta)floor2 & WORDMASK);
        p2Value = *p2++;
        p2Pending = *p2++;
#ifdef IS_INTEL_ENDIAN
        p2Value = (p2Value >> rShift) | (p2Pending << lShift);
#else
        p2Value = (p2Value << rShift) | (p2Pending >> lShift);
#endif
        uinta x = startMasks[offset1] & (*p1++ ^ p2Value);
        if (x) return byteOf1stBitNZ(x) - offset1;
        }
    else
        {
        p1 = (uinta*)floor1;
        p2 = (uinta*)((uinta)floor2 & WORDMASK);
        p2Pending = *p2++;
        }
    const uint8 *roof1 = floor1 + len;
    const uinta *p1Roof = (uinta*)((uinta)roof1 & WORDMASK);
    if (p1 < p1Roof)
        {
        uinta x;
        do
            {
            p2Value = p2Pending;
            p2Pending = *p2++;
#ifdef IS_INTEL_ENDIAN
            p2Value = (p2Value >> rShift) | (p2Pending << lShift);
#else
            p2Value = (p2Value << rShift) | (p2Pending >> lShift);
#endif
            x = *p1++ ^ p2Value;
            }
        while (!x && p1 != p1Roof);
        if (x) return byteOf1stBitNZ(x) + (uint8*)(p1 - 1) - floor1;
        }
    offset1 = roof1 - (uint8*)p1Roof;
    if (offset1)
        {
        if (offset1 + offset2 > WORDSIZE)
            {
            p2Value = p2Pending;
            p2Pending = *p2;
#ifdef IS_INTEL_ENDIAN
            p2Value = (p2Value >> rShift) | (p2Pending << lShift);
#else
            p2Value = (p2Value << rShift) | (p2Pending >> lShift);
#endif
            }
        else
#ifdef IS_INTEL_ENDIAN
            p2Value = p2Pending >> rShift;
#else
            p2Value = p2Pending << rShift;
#endif
        uinta x = endMasks[offset1] & (*p1 ^ p2Value);
        if (x) return byteOf1stBitNZ(x) + len - offset1;
        }
    return len;
    }

inline uinta compareBytes(const uint8 *floor1, const uint8 *roof1, const uint8 *floor2, const uint8 *roof2)
    {
    uinta offset1 = roof1 - floor1;
    uinta offset2 = roof2 - floor2;
    uinta len = offset1 < offset2 ? offset1 : offset2;
    switch (len)
        {
        case 1:
            return *floor1 != *floor2 ? 0 : 1;
        case 2:
            if (*floor1 != *floor2) return 0;
            return *++floor1 != *++floor2 ? 1 : 2;
        case 3:
            if (*floor1 != *floor2) return 0;
            if (*++floor1 != *++floor2) return 1;
            return *++floor1 != *++floor2 ? 2 : 3;
        default:
            offset1 = (uinta)floor1 & OFFSETMASK;
            offset2 = (uinta)floor2 & OFFSETMASK;
            if (offset2 < offset1)
                {
                uinta temp = (uinta)floor1;
                floor1 = floor2;
                floor2 = (uint8*)temp;
                temp = offset1;
                offset1 = offset2;
                offset2 = temp;
                }
            uinta x = offset1 + len;
            if (x & WORDMASK && x != WORDSIZE)
                {
                if (offset1 == offset2)
                    return compareBytesAligned(floor1, floor2, offset1, len);
                else
                    return compareBytes2Ahead(floor1, offset1, floor2, offset2, len);
                }
            else
                {
                if (offset1 == offset2)
                    {
                    x = (*((uinta*)((uinta)floor1 & WORDMASK)) ^ *((uinta*)((uinta)floor2 & WORDMASK))) & startMasks[offset1] & endMasks[x];
                    return x ? byteOf1stBitNZ(x) - offset1 : len;
                    }
                else if ((offset2 + len - 1) & WORDMASK)
                    {
                    uinta *p2 = (uinta*)((uinta)floor2 & WORDMASK);
                    offset2 -= offset1;
#ifdef IS_INTEL_ENDIAN
                    x = (*((uinta*)((uinta)floor1 & WORDMASK)) ^ (*p2 >> 8*offset2 | p2[1] << 8*(WORDSIZE - offset2))) & startMasks[offset1] & endMasks[x];
#else
                    x = (*((uinta*)((uinta)floor1 & WORDMASK)) ^ (*p2 << 8*offset2 | p2[1] >> 8*(WORDSIZE - offset2))) & startMasks[offset1] & endMasks[x];
#endif
                    return x ? byteOf1stBitNZ(x) - offset1 : len;
                    }
                else
                    {
#ifdef IS_INTEL_ENDIAN
                    x = (*((uinta*)((uinta)floor1 & WORDMASK)) ^ (*((uinta*)((uinta)floor2 & WORDMASK)) >> 8*(offset2 - offset1))) & startMasks[offset1] & endMasks[x];
#else
                    x = (*((uinta*)((uinta)floor1 & WORDMASK)) ^ (*((uinta*)((uinta)floor2 & WORDMASK)) << 8*(offset2 - offset1))) & startMasks[offset1] & endMasks[x];
#endif
                    return x ? byteOf1stBitNZ(x) - offset1 : len;
                    }
                }
        }
    }



static uint32 randomSeed = 1;
static const uint8 *randomSeedBytes = (uint8*)&randomSeed;

static void randomFill(uint8 *floor, uint8 *roof)
    {
    while (floor < roof)
        {
        randomSeed = 1103515245*randomSeed + 12345;
        *floor++ = randomSeedBytes[0];
        if (floor >= roof) break;
        *floor++ = randomSeedBytes[1];
        }
    }

static void dumpMemory(FILE *fout, const void *floor, const void *roof)
    {
    static const TCHAR *rightMarginFormat = sizeof(void*) == 4 ? _T("%08X:   ") : _T("%016llX:   ");
    TCHAR str[17];
    str[16] = 0;
    uinta padLen = (uinta)floor & 0xF;
    uinta pos = 0;
    uint8 *pmem = (uint8*)floor;
    while (padLen--)
        {
        if (pos == 0) zfprintf(fout, rightMarginFormat, (uinta)((uint8*)floor - (padLen + 1)));
        zfprintf(fout, (pos % 4 == 3 ? _T("     ") : _T("   ")));
        str[pos++] = _T(' ');
        }
    while (pmem < (uint8*)roof)
        {
        if (pos == 0) zfprintf(fout, rightMarginFormat, (uinta)pmem);
        uint8 ch = *pmem++;
        zfprintf(fout, _T("%02X "), (uint32)ch);
        if (pos % 4 == 3) zfprintf(fout, _T("  "));
        str[pos++] = (_T(' ') <= ch && ch <= _T('~')) ? ch : _T('.');
        if (pos == 16)
            {
            zfprintf(fout, _T("%s\n"), str);
            pos = 0;
            }
        }
    if (pos && pos != 16)
        {
        while (pos < 16)
            {
            zfprintf(fout, (pos % 4 == 3 ? _T("     ") : _T("   ")));
            str[pos++] = _T(' ');
            }
        zfprintf(fout, _T("%s\n"), str);
        }
    }

#define BUFLEN (0x10000)
static uint8 buf1[BUFLEN];
static uint8 buf2[BUFLEN];

static void compareBytesCheckResult(const TCHAR *message, const uinta expectedResult, const uint8 *floor1, const uint8 *roof1, const uint8 *floor2, const uint8 *roof2)
    {
    uinta result = compareBytes(floor1, roof1, floor2, roof2);
    if (result != expectedResult)
        {
        uinta len1 = roof1 - floor1;
        uinta len2 = roof2 - floor2;
        uinta startOffset1 = (uinta)floor1 & OFFSETMASK;
        uinta endOffset1 = (uinta)roof1 & OFFSETMASK;
        uinta startOffset2 = (uinta)floor2 & OFFSETMASK;
        uinta endOffset2 = (uinta)roof2 & OFFSETMASK;
        zprintf("\nHOUSTON We'Ve got a problem! %s\n", message);
        zprintf("    Got 0x%K (%yu), But Wanted 0x%K (%yu)\n", result, result, expectedResult, expectedResult);
        zprintf("    Start Offsets (%yu, %yu) End Offsets (%yu, %yu)\n", startOffset1, startOffset2, endOffset1, endOffset2);
        zprintf("    Lengths 0x%K (%yu), 0x%K (%yu)\n", len1, len1, len2, len2);
        zprintf("\nDUMPING BUFFER 1\n");
        dumpMemory(stdout, (const uint8*)buf1, roof1);
        zprintf("\nDUMPING BUFFER 2\n");
        dumpMemory(stdout, (const uint8*)buf2, roof2);
        zprintf("\n");
        exit(0);
        }
    }

static void testAllPositions(uinta offset1, uinta offset2, uinta len)
    {
    randomFill(buf1, buf1 + offset1 + offset2 + len);
    memcpy(buf2 + offset2, buf1 + offset1, len);
    for (uinta i = 0; i < offset1; i++) buf1[i] = 0;
    for (uinta i = 0; i < offset2; i++) buf2[i] = 0xFF;
    for (uinta i = offset1 + len; i < offset1 + len + WORDSIZE; i++) buf1[i] = 0;
    for (uinta i = offset2 + len; i < offset2 + len + WORDSIZE; i++) buf2[i] = 0xFF;
    compareBytesCheckResult("(different ends)", len, buf1 + offset1, buf1 + offset1 + len, buf2 + offset2, buf2 + offset2 + len);
    for (uinta i = 0; i < len; i++)
        {
        buf1[offset1 + i] = ~buf1[offset1 + i];
        compareBytesCheckResult("(different ends)", i, buf1 + offset1, buf1 + offset1 + len, buf2 + offset2, buf2 + offset2 + len);
        buf2[offset2 + i] = ~buf2[offset2 + i];
        }
    for (uinta i = 0; i < offset2; i++) buf2[i] = 0;
    for (uinta i = offset2 + len; i < offset2 + len + WORDSIZE; i++) buf2[i] = 0;
    compareBytesCheckResult("(same ends)", len, buf1 + offset1, buf1 + offset1 + len, buf2 + offset2, buf2 + offset2 + len);
    for (uinta i = 0; i < len; i++)
        {
        buf1[offset1 + i] = ~buf1[offset1 + i];
        compareBytesCheckResult("(same ends)", i, buf1 + offset1, buf1 + offset1 + len, buf2 + offset2, buf2 + offset2 + len);
        buf2[offset2 + i] = ~buf2[offset2 + i];
        }
    }

static void testShortRuns()
    {
    for (uinta len = 1; len < 41; len++)
        for (uinta offset1 = 0; offset1 < WORDSIZE; offset1++)
            for (uinta offset2 = 0; offset2 < WORDSIZE; offset2++)
                testAllPositions(offset1, offset2, len);
    }

static void testLongRuns()
    {
    for (uinta len = 7; len < 8*7*7; len += 1)
        for (uinta offset1 = 0; offset1 < WORDSIZE; offset1++)
            for (uinta offset2 = 0; offset2 < WORDSIZE; offset2++)
                testAllPositions(offset1, offset2, len);
    }

static void test()
    {
    testShortRuns();
    testLongRuns();
//    randomFill(buf1, buf1 + BUFLEN);
//    memcpy(buf2, buf1, BUFLEN);
//    uinta result = compareBytes(buf1, buf1 + BUFLEN, buf2, buf2 + BUFLEN);
//    printf("BUFLEN = %llu, result = %llu\n", (uint64)BUFLEN, (uint64)result);
//    buf2[BUFLEN - 2] = 0;
//    result = compareBytes(buf1, buf1 + BUFLEN, buf2, buf2 + BUFLEN);
//    printf("BUFLEN = %llu, result = %llu\n", (uint64)BUFLEN, (uint64)result);
    }

int main(int argc, char *argv[])
    {
    test();
    return 0;
    }

















































