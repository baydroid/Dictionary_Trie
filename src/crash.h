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
#ifndef CRASH_H_
#define CRASH_H_



#ifdef _MSC_VER /* Microsoft compiler */

inline static void crash_internals(const TCHAR *msg, unsigned lineNumber, const TCHAR *function, const TCHAR *filename)
    {
    zfprintf(stderr, _T("%s INTERNAL BUG: %s\n"), PROGRAM_NAME, msg);
    zfprintf(stderr, _T("Program version: %s; line: %u; function: %s; file: %s.\n"), PROGRAM_VERSION_STR, lineNumber, function, filename);
    DWORD le = GetLastError();
    zfprintf(stderr, _T("GetLastError() %d\n"), le);
    TCHAR buffer[0x400];
    buffer[0] = 0;
    _tcserror_s(buffer, 0x400, errno);
    zfprintf(stderr, _T("Errno %d: %s\n"), errno, buffer);
    *((int*)0)=5;
    exit(le);
    }

#ifdef _UNICODE
#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)
#define __WFILE__ WIDEN(__FILE__)
#define __WFUNCTION__ WIDEN(__FUNCTION__)
#define crash(msg) (crash_internals((msg), __LINE__, __WFUNCTION__, __WFILE__))
#define check_crash(result, okValue, msg) (check_crash_internals((result), (okValue), (msg), __LINE__, __WFUNCTION__, __WFILE__))
#else
#define crash(msg) (crash_internals((msg), __LINE__, __FUNCTION__, __FILE__))
#define check_crash(result, okValue, msg) (check_crash_internals((result), (okValue), (msg), __LINE__, __FUNCTION__, __FILE__))
#endif

#endif



#ifdef __GNUC__ /* GNU compiler */

inline static void crash_internals(const TCHAR *msg, unsigned lineNumber, const TCHAR *function, const TCHAR *filename)
    {
    zfprintf(stderr, _T("%s INTERNAL BUG: %s\n"), PROGRAM_NAME, msg);
    zfprintf(stderr, _T("Program version: %s; line: %u; function: %s; file: %s.\n"), PROGRAM_VERSION_STR, lineNumber, function, filename);
    zfprintf(stderr, _T("Errno %d: %s\n"), errno, strerror(errno));
    *((int*)0)=5;
    exit(errno ? errno : -1);
    }

#define crash(msg) (crash_internals((msg), __LINE__, __func__, __FILE__))
#define check_crash(result, okValue, msg) (check_crash_internals((result), (okValue), (msg), __LINE__, __func__, __FILE__))

#endif



inline static int check_crash_internals(int result, int okValue, const TCHAR *msg, unsigned lineNumber, const TCHAR *function, const TCHAR *filename)
    {
    if (result != okValue) crash_internals(msg, lineNumber, function, filename);
    return result;
    }



#endif /* CRASH_H_ */
