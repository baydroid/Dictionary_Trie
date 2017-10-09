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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "addr_width.h"
#include "basic_types.h"
#include "zprintf_etc.h"



static uinta totalAllocated = 0;
static uinta mallocCount    = 0;
static uinta freeCount      = 0;



void reporting_basicReport()
    {
    inta blockCount = mallocCount - freeCount;
    inta bytesPerBlock = blockCount ? totalAllocated/blockCount : 0;
    zprintf(_T("ALLOCATED bytes = %yu blocks = %yd bytes/block = %yd PROCESSED mallocs = %yu frees = %yu\n"), totalAllocated, blockCount, bytesPerBlock, mallocCount, freeCount);
    }

void *reporting_malloc(uinta len)
    {
    totalAllocated += len;
    mallocCount++;
    uinta *m =  (uinta*)malloc(len + sizeof(void*));
    *m = len;
#ifdef REpORT_INDIVIDUAL_ACTIONS
    fflush(stdout);
    zprintf(_T("malloc(%yu) AFTERWARDS "), len);
    reporting_basicReport();
    fflush(stdout);
#endif
    return m + 1;
    }

void reporting_free(void *memory)
    {
    uinta *m = (uinta*)memory - 1;
    uinta len = *m;
    totalAllocated -= len;
    freeCount++;
    free(m);
#ifdef REpORT_INDIVIDUAL_ACTIONS
    fflush(stdout);
    zprintf(_T("free(%yu) AFTERWARDS "), len);
    reporting_basicReport();
    fflush(stdout);
#endif
    }
