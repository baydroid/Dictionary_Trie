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
#ifdef __GNUC__ /* GNU compiler */

#define _vtprintf  vprintf
#define _vftprintf vfprintf
#define _vstprintf vsprintf
#define _tprintf   printf

#endif



#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "zprintf_etc.h"



static const TCHAR *transformFormat(const TCHAR *format);
static unsigned findChangePoint(TCHAR **read, TCHAR **write);
static unsigned isFlagCh(TCHAR ch);
static unsigned isLetterCh(TCHAR ch);



int zprintf(const TCHAR *format, ...)
    {
    int n;
    const TCHAR *f = transformFormat(format);
    va_list args;
    va_start(args, format);
    n = _vtprintf(f, args);
    va_end(args);
    if (f != format) free((void*)f);
    return n;
    }

int zfprintf(FILE *output, const TCHAR *format, ...)
    {
    int n;
    const TCHAR *f = transformFormat(format);
    va_list args;
    va_start(args, format);
    n = _vftprintf(output, f, args);
    va_end(args);
    if (f != format) free((void*)f);
    return n;
    }

#ifdef ZPRINTF_ETC_INCLUDE_ZSPRINTF
int zsprintf(TCHAR *buffer, const TCHAR *format, ...)
    {
    int n;
    const TCHAR *f = transformFormat(format);
    va_list args;
    va_start(args, format);
    n = _vstprintf(buffer, f, args);
    va_end(args);
    if (f != format) free((void*)f);
    return n;
    }
#endif



#define TERMINATOR (0)
#define LOWER_Y    (1)
#define LOWER_K    (2)
#define UPPER_K    (3)

static int deltas32[4] = { 0, -1,  2,  2 };
static int deltas64[4] = { 0,  1,  5,  5 };

static const TCHAR *replacements32[4] = { 0,        0,    _T("08x"),    _T("08X") };
static const TCHAR *replacements64[4] = { 0, _T("ll"), _T("016llx"), _T("016llX") };



/* source        64     32
        y        ll
       %K   %016llX   %08X
       %k   %016llx   %08x */

static const TCHAR *transformFormat(const TCHAR *format)
    {
    TCHAR *newFormat, *wr;
    int *deltaArray = sizeof(void*) == 8 ? deltas64 : deltas32;
    const TCHAR **replacementsArray = sizeof(void*) == 8 ? replacements64 : replacements32;
    TCHAR *rd = (TCHAR*)format;
    int delta = 0;
    unsigned nothingFound = 1;
    for ( ; ; )
        {
        unsigned whatFound = findChangePoint(&rd, 0);
        if (whatFound == TERMINATOR) break;
        nothingFound = 0;
        delta += deltaArray[whatFound];
        }
    if (nothingFound) return format;
    newFormat = wr = (TCHAR*)malloc(sizeof(TCHAR)*((rd - format) + delta));
    rd = (TCHAR*)format;
    for ( ; ; )
        {
        TCHAR ch;
        unsigned whatFound = findChangePoint(&rd, &wr);
        const TCHAR *replacement = replacementsArray[whatFound];
        if (whatFound == TERMINATOR) break;
        if (replacement) while ((ch = *replacement++)) *wr++ = ch;
        }
    return newFormat;
    }

static unsigned findChangePoint(TCHAR **read, TCHAR **write)
    {
    TCHAR ch;
    TCHAR *rd = *read;
    TCHAR *wr = write ? *write : 0;
    for ( ; ; )
        {
        for ( ; ; )
            {
            ch = *rd++;
            if (wr) *wr++ = ch;
            if (ch == _T('%'))
                break;
            else if (!ch)
                {
                *read = rd;
                if (write) *write = wr;
                return TERMINATOR;
                }
            }
        switch ((ch = *rd++))
            {
            case _T('K'):
                *read = rd;
                if (write) *write = wr;
                return UPPER_K;
            case _T('k'):
                *read = rd;
                if (write) *write = wr;
                return LOWER_K;
            case _T('%'):
                if (wr) *wr++ = ch;
                break;
            case 0:
                *read = rd;
                if (write)
                    {
                    *wr++ = ch;
                    *write = wr;
                    }
                return TERMINATOR;
            default:
                while (isFlagCh(ch))
                    {
                    if (wr) *wr++ = ch;
                    ch = *rd++;
                    }
                while (ch && !isLetterCh(ch))
                    {
                    if (wr) *wr++ = ch;
                    ch = *rd++;
                    }
                switch (ch)
                    {
                    case _T('y'):
                        *read = rd;
                        if (write) *write = wr;
                        return LOWER_Y;
                    case 0:
                        *read = rd;
                        if (write)
                            {
                            *wr++ = ch;
                            *write = wr;
                            }
                        return TERMINATOR;
                    default:
                        if (wr) *wr++ = ch;
                        break;
                    }
                break;
            }
        }
    }

static unsigned isFlagCh(TCHAR ch)
    {
    switch (ch)
        {
        case _T('-'): case _T('+'): case _T('0'): case _T(' '): case _T('#'): case _T('\''): case _T('I'):
            return 1;
        default:
            return 0;
        }
    }

static unsigned isLetterCh(TCHAR ch)
    {
    switch (ch)
        {
        case _T('a'): case _T('b'): case _T('c'): case _T('d'): case _T('e'): case _T('f'): case _T('g'):
        case _T('h'): case _T('i'): case _T('j'): case _T('k'): case _T('l'): case _T('m'): case _T('n'):
        case _T('o'): case _T('p'): case _T('q'): case _T('r'): case _T('s'): case _T('t'): case _T('u'):
        case _T('v'): case _T('w'): case _T('x'): case _T('y'): case _T('z'):
        case _T('A'): case _T('B'): case _T('C'): case _T('D'): case _T('E'): case _T('F'): case _T('G'):
        case _T('H'): case _T('I'): case _T('J'): case _T('K'): case _T('L'): case _T('M'): case _T('N'):
        case _T('O'): case _T('P'): case _T('Q'): case _T('R'): case _T('S'): case _T('T'): case _T('U'):
        case _T('V'): case _T('W'): case _T('X'): case _T('Y'): case _T('Z'):
            return 1;
        default:
            return 0;
        }
    }



/* #define include_test_main */
#ifdef include_test_main
int _tmain(int argc, TCHAR* argv[])
    {
    const TCHAR *f1In = _T("%k   %K   %yu   %d   %%");
    const TCHAR *f1Out = transformFormat(f1In);
    _tprintf(_T("Input  %s\nOutput %s\n"), f1In, f1Out);
    return 0;
    }
#endif
