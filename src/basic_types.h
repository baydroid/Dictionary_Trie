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
#ifndef BASIC_TYPES_H_
#define BASIC_TYPES_H_



#ifdef _MSC_VER /* Microsoft compiler */

typedef   signed     __int8    int8;
typedef   signed     __int16   int16;
typedef   signed     __int32   int32;
typedef   signed     __int64   int64;
typedef   unsigned   __int8    uint8;
typedef   unsigned   __int16   uint16;
typedef   unsigned   __int32   uint32;
typedef   unsigned   __int64   uint64;

#define _64(x)     (x ## i64)

#endif

#ifdef __GNUC__ /* GNU compiler */

typedef   signed     char        int8;
typedef   signed     short       int16;
typedef   signed     int         int32;
typedef   signed     long long   int64;
typedef   unsigned   char        uint8;
typedef   unsigned   short       uint16;
typedef   unsigned   int         uint32;
typedef   unsigned   long long   uint64;

#define _64(x)     (x ## LL)

#endif



#if ADDR_WIDTH == 64

#define IS64BIT (1)

typedef   int64    inta;
typedef   uint64   uinta;

#else /* assume ADDR_WIDTH == 32 */

#define IS32BIT (1)

typedef   int32    inta;
typedef   uint32   uinta;

#endif



typedef   uint8    bool8;
typedef   uint16   bool16;
typedef   uint32   bool32;
typedef   uint64   bool64;
typedef   uinta    boola;



inline uinta alignLen64(uinta len)
    {
    return len & 7 ? (len & ~((uinta)7)) + 8 : len;
    }

inline uinta alignLen32(uinta len)
    {
    return len & 3 ? (len & ~((uinta)3)) + 4 : len;
    }



#define YES        (1)
#define NO         (0)

#define MAX_INT8   ((int8)(0x7F))
#define MIN_INT8   ((int8)(0x80))
#define MAX_INT16  ((int16)(0x7FFF))
#define MIN_INT16  ((int16)(0x8000))
#define MAX_INT32  ((int32)(0x7FFFFFFF))
#define MIN_INT32  ((int32)(0x80000000))
#define MAX_INT64  ((int64)(_64(0x7FFFFFFFFFFFFFFF)))
#define MIN_INT64  ((int64)(_64(0x8000000000000000)))

#define MAX_UINT8  ((uint8)(0xFF))
#define MIN_UINT8  ((uint8)(0))
#define MAX_UINT16 ((uint16)(0xFFFF))
#define MIN_UINT16 ((uint16)(0))
#define MAX_UINT32 ((uint32)(0xFFFFFFFF))
#define MIN_UINT32 ((uint32)(0))
#define MAX_UINT64 ((uint64)(_64(0xFFFFFFFFFFFFFFFF)))
#define MIN_UINT64 ((uint64)(0))



#if ADDR_WIDTH == 64

#define MAX_INTA   MAX_INT64
#define MIN_INTA   MIN_INT64
#define MAX_UINTA  MAX_UINT64
#define MIN_UINTA  MIN_UINT64

inline uinta alignLen(uinta len)
    {
    return alignLen64(len);
    }

#else

#define MAX_INTA   MAX_INT32
#define MIN_INTA   MIN_INT32
#define MAX_UINTA  MAX_UINT32
#define MIN_UINTA  MIN_UINT32

inline uinta alignLen(uinta len)
    {
    return alignLen32(len);
    }

#endif



#endif /* BASIC_TYPES_H_ */
