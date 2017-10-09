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
#ifndef ZPRINTF_ETC_H_
#define ZPRINTF_ETC_H_

#ifdef __cplusplus
extern "C" {
#endif



#define zprintf  zprintf_etc_zprintf
#define zfprintf zprintf_etc_zfprintf
#define zsprintf zprintf_etc_zsprintf



#ifdef _MSC_VER /* Microsoft compiler */

#include <tchar.h>

#endif



#ifdef __GNUC__ /* GNU compiler */

#define TCHAR   char
#define _T(s)   s
#define _tmain  main
#define _tcslen strlen
#define _tcscpy strcpy
#define _tfopen fopen

#endif



extern int zprintf(const TCHAR *format, ...);
extern int zfprintf(FILE *output, const TCHAR *format, ...);
#ifdef ZPRINTF_ETC_INCLUDE_ZSPRINTF
extern int zsprintf(TCHAR *buffer, const TCHAR *format, ...);
#endif



#ifdef __cplusplus
}
#endif

#endif /* ZPRINTF_ETC_H_ */
