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
//#define TEST_VLK_BINARY_TRIE (1)



#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "addr_width.h"
#include "basic_types.h"
#include "zprintf_etc.h"
#include "vlk_binary_trie.h"





static void dump(VLKBinaryTrie *vbt)
    {
#ifdef TEST_VLK_BINARY_TRIE
    vbt->dump();
#endif
    }

static void performIteration(VLKBinaryTrie *vbt, VLKBinaryTrieIterator *itor, bool expunge)
    {
    uinta value;
    while (itor->next((void**)&value))
        {
        uinta keyLen;
        itor->computeKeyLen(&keyLen);
        uint8 *key = (uint8*)malloc(keyLen);
        itor->regenerateKey(key, keyLen);
        zprintf(_T("%04yu \"%.*s\" ---> %yu"), keyLen, keyLen, key, value);
        free(key);
        if (expunge)
            {
            itor->expunge();
            zprintf(_T(" EXPUNGING\n"));
            dump(vbt);
            }
        else
            zprintf(_T("\n"));
        }
    }

static void iterate(VLKBinaryTrie *vbt, bool expunge)
    {
    zprintf(_T("Iterating trie of %yu kvps\n"), vbt->size());
    VLKBinaryTrieIterator *itor = new VLKBinaryTrieIterator();
    itor->reset(vbt);
    performIteration(vbt, itor, expunge);
    delete itor;
    }

static void itorateConfined(VLKBinaryTrie *vbt, const TCHAR *key, bool expunge)
    {
    uinta keyLen = sizeof(TCHAR)*_tcslen(key);
    const uint8 *keyFloor = (uint8*)key;
    const uint8 *keyRoof = (uint8*)key + keyLen;
    VLKBinaryTrieIterator *itor = new VLKBinaryTrieIterator();
    itor->reset(vbt, &keyFloor, keyRoof, YES);
    zprintf(_T("Iterating trie of %yu kvps CONFINED matched %yd bytes of \"%s\"\n"), vbt->size(), (keyFloor - (uint8*)key), key);
    performIteration(vbt, itor, expunge);
    delete itor;
    }

static bool expunge(VLKBinaryTrie *vbt, const TCHAR *key, uinta *value)
    {
    uinta keyLen = sizeof(TCHAR)*_tcslen(key);
    bool boolResult = vbt->expunge((uint8*)key, (uint8*)(key + keyLen), (void**)value);
    const TCHAR *strResult = boolResult ? _T("YES") : _T("NO");
    zprintf(_T(">>>>> expunge() key = \"%s\", value = %yu returned %s\n"), key, *value, strResult);
    return boolResult;
    }

static bool set(VLKBinaryTrie *vbt, const TCHAR *key, uinta value)
    {
    uinta keyLen = sizeof(TCHAR)*_tcslen(key);
    bool boolResult = vbt->set((uint8*)key, (uint8*)(key + keyLen), (void*)value);
    const TCHAR *strResult = boolResult ? _T("YES") : _T("NO");
    zprintf(_T(">>>>> set() key = \"%s\", value = %yu returned %s\n"), key, value, strResult);
    return boolResult;
    }
/*
static bool setIfPresent(VLKBinaryTrie *vbt, TCHAR *key, uinta value)
    {
    uinta keyLen = sizeof(TCHAR)*_tcslen(key);
    TCHAR *result = vbt->setIfPresent((uint8*)key, (uint8*)(key + keyLen), (void*)value) ? _T("YES") : _T("NO");
    zprintf(_T(">>>>> setIfPresent() key = \"%s\", value = %yu returned %s\n"), key, value, result);
    }
*/
static bool get(VLKBinaryTrie *vbt, const TCHAR *key, uinta *value)
    {
    uinta keyLen = sizeof(TCHAR)*_tcslen(key);
    bool boolResult = vbt->get((uint8*)key, (uint8*)(key + keyLen), (void**)value);
    const TCHAR *strResult = boolResult ? _T("YES") : _T("NO");
    zprintf(_T(">>>>> get() key = \"%s\", value = %yu returned %s\n"), key, *value, strResult);
    return boolResult;
    }
/*
static bool getIfPresentElseSet(VLKBinaryTrie *vbt, TCHAR *key, uinta *value)
    {
    uinta valueBefore = *value;
    uinta keyLen = sizeof(TCHAR)*_tcslen(key);
    TCHAR *result = vbt->getIfPresentElseSet((uint8*)key, (uint8*)(key + keyLen), (void**)value) ? _T("YES") : _T("NO");
    zprintf(_T(">>>>> getIfPresentElseSet() key = \"%s\", value before = %yu after = %yu returned %s\n"), key, valueBefore, *value, result);
    }

static bool swap(VLKBinaryTrie *vbt, TCHAR *key, uinta *value)
    {
    uinta valueBefore = *value;
    uinta keyLen = sizeof(TCHAR)*_tcslen(key);
    TCHAR *result = vbt->swap((uint8*)key, (uint8*)(key + keyLen), (void**)value) ? _T("YES") : _T("NO");
    zprintf(_T(">>>>> getIfPresentElseSet() key = \"%s\", value before = %yu after = %yu returned %s\n"), key, valueBefore, *value, result);
    }

static bool swapIfPresent(VLKBinaryTrie *vbt, TCHAR *key, uinta *value)
    {
    uinta valueBefore = *value;
    uinta keyLen = sizeof(TCHAR)*_tcslen(key);
    TCHAR *result = vbt->swapIfPresent((uint8*)key, (uint8*)(key + keyLen), (void**)value) ? _T("YES") : _T("NO");
    zprintf(_T(">>>>> getIfPresentElseSet() key = \"%s\", value before = %yu after = %yu returned %s\n"), key, valueBefore, *value, result);
    }
*/

static void test1()
    {
    VLKBinaryTrie *vbt = new VLKBinaryTrie();
    dump(vbt);
    set(vbt, _T("Hail ISIS!"), 101);
    dump(vbt);
    set(vbt, _T("Hail Bohemia!"), 102);
    dump(vbt);
    set(vbt, _T("Holy mackeral"), 103);
    dump(vbt);
    set(vbt, _T("Holy mackeral Batman"), 104);
    dump(vbt);
    set(vbt, _T("Holy mackeral"), 203);
    dump(vbt);
    set(vbt, _T("Huzzar"), 105);
    dump(vbt);
    set(vbt, _T("Highway"), 106);
    dump(vbt);
    set(vbt, _T("Heklina"), 107);
    dump(vbt);
    set(vbt, _T("Hmmmph! said the elephant"), 107);
    dump(vbt);
    set(vbt, _T("HVAC"), 107);
    dump(vbt);
    set(vbt, _T("HD"), 112);
    dump(vbt);
    set(vbt, _T("Highway Route 66"), 108);
    dump(vbt);
    set(vbt, _T("Highway-999"), 109);
    dump(vbt);
    set(vbt, _T("Highway/Loway"), 110);
    dump(vbt);
    set(vbt, _T("form"), 120);
    dump(vbt);
    set(vbt, _T("Hail"), 111);
    dump(vbt);
    set(vbt, _T("Ha"), 113);
    dump(vbt);

    itorateConfined(vbt, _T("Ha"), NO);
    itorateConfined(vbt, _T("Hat"), NO);
    itorateConfined(vbt, _T("Highway"), NO);
    itorateConfined(vbt, _T("Hip"), NO);
    itorateConfined(vbt, _T("form"), NO);
    itorateConfined(vbt, _T("Q"), NO);

    uinta dummy;
    get(vbt, _T("Hail ISIS!"), &dummy);
    get(vbt, _T("Hail Bohemia!"), &dummy);
    get(vbt, _T("Holy mackeral"), &dummy);
    get(vbt, _T("Holy mackeral Batman"), &dummy);
    get(vbt, _T("Huzzar"), &dummy);
    get(vbt, _T("Highway"), &dummy);
    get(vbt, _T("Heklina"), &dummy);
    get(vbt, _T("Hmmmph! said the elephant"), &dummy);
    get(vbt, _T("HVAC"), &dummy);
    get(vbt, _T("HD"), &dummy);
    get(vbt, _T("Highway Route 66"), &dummy);
    get(vbt, _T("form"), &dummy);

    get(vbt, _T("Hail ISIS"), &dummy);
    get(vbt, _T("Hail Bohemia!l"), &dummy);
    get(vbt, _T("Holy mackera"), &dummy);
    get(vbt, _T("Holy mackeral Batmann"), &dummy);
    get(vbt, _T("Huzza"), &dummy);
    get(vbt, _T("Highway1"), &dummy);
    get(vbt, _T("Heklin"), &dummy);
    get(vbt, _T("Hmmmph! said the elephant "), &dummy);
    get(vbt, _T("HVA"), &dummy);
    get(vbt, _T("HDD"), &dummy);
    get(vbt, _T("Highway Route 1"), &dummy);
    get(vbt, _T("forms"), &dummy);
    get(vbt, _T("Hail "), &dummy);

    iterate(vbt, YES);
    dump(vbt);
    expunge(vbt, _T("Hail ISIS!"), &dummy);
    dump(vbt);
    expunge(vbt, _T("Hail Bohemia!"), &dummy);
    dump(vbt);
    expunge(vbt, _T("Holy mackeral"), &dummy);
    dump(vbt);
    expunge(vbt, _T("Holy mackeral Batman"), &dummy);
    dump(vbt);
    expunge(vbt, _T("Huzzar"), &dummy);
    dump(vbt);
    expunge(vbt, _T("Highway"), &dummy);
    dump(vbt);
    expunge(vbt, _T("Heklina"), &dummy);
    dump(vbt);
    expunge(vbt, _T("Hmmmph! said the elephant"), &dummy);
    dump(vbt);
    expunge(vbt, _T("HVAC"), &dummy);
    dump(vbt);
    expunge(vbt, _T("HD"), &dummy);
    dump(vbt);
    expunge(vbt, _T("Highway Route 66"), &dummy);
    dump(vbt);
    expunge(vbt, _T("form"), &dummy);
    dump(vbt);
    expunge(vbt, _T("Highway-999"), &dummy);
    dump(vbt);
    expunge(vbt, _T("Highway/Loway"), &dummy);
    dump(vbt);
    expunge(vbt, _T("Ha"), &dummy);
    dump(vbt);
    expunge(vbt, _T("Hail "), &dummy);
    dump(vbt);

    itorateConfined(vbt, _T("Hip"), NO);
    itorateConfined(vbt, _T("form"), NO);
    itorateConfined(vbt, _T("Q"), NO);
    iterate(vbt, NO);

    delete vbt;
    }



int _tmain(int argc, char *argv[])
    {
    test1();
    return 0;
    }






































