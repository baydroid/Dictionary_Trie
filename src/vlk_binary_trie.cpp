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



#define PROGRAM_VERSION_STR _T("0.0.1")
#define PROGRAM_NAME        _T("vlk_binary_trie")
#define BANNER_STR          (PROGRAM_NAME _T(" *BETA* version ") PROGRAM_VERSION_STR _T(", Apache License v2.0, transmission.aquitaine@yahoo.com"))
#include "crash.h"



#define WORD_SIZE   ((uinta)(sizeof(void*)))
#define OFFSET_MASK ((uinta)(WORD_SIZE - 1))
#define WORD_MASK   ((uinta)(~OFFSET_MASK))



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

inline uinta compareBytesAligned(const uint8 *floor1, const uint8 *floor2, uinta offset, uinta len)
    {
    const uinta *p1, *p2;
    if (offset)
        {
        p1 = (uinta*)((uinta)floor1 & WORD_MASK);
        p2 = (uinta*)((uinta)floor2 & WORD_MASK);
        uinta x = startMasks[offset] & (*p1++ ^ *p2++);
        if (x) return byteOf1stBitNZ(x) - offset;
        }
    else
        {
        p1 = (uinta*)floor1;
        p2 = (uinta*)floor2;
        }
    const uint8 *roof1 = floor1 + len;
    const uinta *p1Roof = (uinta*)((uinta)roof1 & WORD_MASK);
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

inline uinta compareBytes2Ahead(const uint8 *floor1, uinta offset1, const uint8 *floor2, uinta offset2, uinta len) // assumes offset1 < offset2
    {
    const uinta *p1, *p2;
    uinta p2Value = 0;
    uinta p2Pending = 0;
    offset2 -= offset1;
    uinta rShift = 8*offset2;
    uinta lShift = 8*(WORD_SIZE - offset2);
    if (offset1)
        {
        p1 = (uinta*)((uinta)floor1 & WORD_MASK);
        p2 = (uinta*)((uinta)floor2 & WORD_MASK);
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
        p2 = (uinta*)((uinta)floor2 & WORD_MASK);
        p2Pending = *p2++;
        }
    const uint8 *roof1 = floor1 + len;
    const uinta *p1Roof = (uinta*)((uinta)roof1 & WORD_MASK);
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
        if (offset1 + offset2 > WORD_SIZE)
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
            offset1 = (uinta)floor1 & OFFSET_MASK;
            offset2 = (uinta)floor2 & OFFSET_MASK;
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
            if (x & WORD_MASK && x != WORD_SIZE)
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
                    x = (*((uinta*)((uinta)floor1 & WORD_MASK)) ^ *((uinta*)((uinta)floor2 & WORD_MASK))) & startMasks[offset1] & endMasks[x];
                    return x ? byteOf1stBitNZ(x) - offset1 : len;
                    }
                else if ((offset2 + len - 1) & WORD_MASK)
                    {
                    uinta *p2 = (uinta*)((uinta)floor2 & WORD_MASK);
                    offset2 -= offset1;
#ifdef IS_INTEL_ENDIAN
                    x = (*((uinta*)((uinta)floor1 & WORD_MASK)) ^ (*p2 >> 8*offset2 | p2[1] << 8*(WORD_SIZE - offset2))) & startMasks[offset1] & endMasks[x];
#else
                    x = (*((uinta*)((uinta)floor1 & WORD_MASK)) ^ (*p2 << 8*offset2 | p2[1] >> 8*(WORD_SIZE - offset2))) & startMasks[offset1] & endMasks[x];
#endif
                    return x ? byteOf1stBitNZ(x) - offset1 : len;
                    }
                else
                    {
#ifdef IS_INTEL_ENDIAN
                    x = (*((uinta*)((uinta)floor1 & WORD_MASK)) ^ (*((uinta*)((uinta)floor2 & WORD_MASK)) >> 8*(offset2 - offset1))) & startMasks[offset1] & endMasks[x];
#else
                    x = (*((uinta*)((uinta)floor1 & WORD_MASK)) ^ (*((uinta*)((uinta)floor2 & WORD_MASK)) << 8*(offset2 - offset1))) & startMasks[offset1] & endMasks[x];
#endif
                    return x ? byteOf1stBitNZ(x) - offset1 : len;
                    }
                }
        }
    }



// VLKBinaryTrie Flags
#define NULL_PRESENT     (0x01)
#define ZERO_LEN_PRESENT (0x02)

// Node Flags
#define TERMINAL         (0x01)

// Node Type
#define DECISION         (0x01)
#define RUN              (0x02)
#define SHORT_END        (0x03)

// Iterator/Reference position identifiers
#define AT_BEFORE_START  (0x01)
#define AT_NULL          (0x02)
#define AT_ZERO_LEN      (0x03)
#define AT_ROOT          (0x04)
#define AT_AFTER_END     (0x05)

// Maximum number of spare bytes allowed after the end of the data in a Run, must be < 0xFFFC on 32 bit systems, < 0xFFFFFFF8 on 64 bit systems.
#define MAX_TRAILING_LEN (0x4000)



struct VLKBinaryTrie::Node
    {
    Node *backlink;
#ifdef IS64BIT
    uint32 trailingLen;
    uint16 spare1;
#else
    uint16 trailingLen;
#endif
    uint8 bitMask;
    unsigned nodeType        : 2;
    unsigned isTerminal      : 1;
    unsigned dataFloorOffset : 3;
    unsigned spare2          : 2;

    bool     isDecision()  { return nodeType == DECISION;  }
    bool     isRun()       { return nodeType == RUN;       }
    bool     isShortEnd()  { return nodeType == SHORT_END; }
    Decision *asDecision() { return (Decision*)this;       }
    Run      *asRun()      { return (Run*)this;            }
    ShortEnd *asShortEnd() { return (ShortEnd*)this;       }

    inline bool isEndValueNode();
    inline void replaceLink(Node *oldLink, Node *newLink);
    inline void getValue(void **value);
    inline void setValue(void *value);
    inline Node *down(uinta index);
    };

struct VLKBinaryTrie::Decision : VLKBinaryTrie::Node
    {
    Node *links[2];
    };

struct VLKBinaryTrie::ShortEnd : VLKBinaryTrie::Node
    {
    void *value;
    Node *link;
    };

struct VLKBinaryTrie::RunSizer : VLKBinaryTrie::Node
    {
    uint8 *dataRoof;
    union
        {
        void *value;
        Node *link;
        };
    };

struct VLKBinaryTrie::Run : VLKBinaryTrie::RunSizer
    {
    uint8 minDataFloor[1];

    uint8 *dataFloor()              { return minDataFloor + dataFloorOffset;                                                                }
    Node  *endValueNode()           { return isTerminal ? this : link;                                                                      }
    void  *endValue()               { return isTerminal ? value : link->asShortEnd()->value;                                                }
    Node  *setEndValue(void *value) { if (isTerminal) { this->value = value; return this; } link->asShortEnd()->value = value; return link; }
    };



bool VLKBinaryTrie::Node::isEndValueNode()
    {
    switch (nodeType)
        {
        case DECISION:  return NO;
        case RUN:       return isTerminal;
        case SHORT_END: return YES;
        default:        crash(_T("VLKBinaryTrie bad nodeType")); return NO;
        }
    }

void VLKBinaryTrie::Node::replaceLink(Node *oldLink, Node *newLink)
    {
    switch (nodeType)
        {
        case DECISION:  asDecision()->links[asDecision()->links[0] == oldLink ? 0 : 1] = newLink; break;
        case RUN:       asRun()->link = newLink;                                                  break;
        case SHORT_END: asShortEnd()->link = newLink;                                             break;
        default:        crash(_T("VLKBinaryTrie bad nodeType"));                                  break;
        }
    }

void VLKBinaryTrie::Node::getValue(void **value)
    {
    switch (nodeType)
        {
        case RUN:       if (isTerminal) *value = asRun()->value; break;
        case SHORT_END: *value = asShortEnd()->value;            break;
        default:                                                 break;
        }
    }

void VLKBinaryTrie::Node::setValue(void *value)
    {
    switch (nodeType)
        {
        case RUN:       if (isTerminal) asRun()->value = value; break;
        case SHORT_END: asShortEnd()->value = value;            break;
        default:                                                break;
        }
    }

VLKBinaryTrie::Node *VLKBinaryTrie::Node::down(uinta index)
    {
    switch (nodeType)
        {
        case DECISION:  return asDecision()->links[index];
        case RUN:       return isTerminal ? 0 : asRun()->link;
        case SHORT_END: return asShortEnd()->link;
        default:        crash(_T("VLKBinaryTrie bad nodeType")); return 0;
        }
    }



inta VLKBinaryTrie::keycmp(const uint8 *floor1, const uint8 *roof1, const uint8 *floor2, const uint8 *roof2)
    {
    if (!floor1)
        return floor2 ? -1 : 0;
    else if (!floor2)
        return +1;
    else if (floor1 >= roof1)
        return floor2 >= roof2 ? 0 : -1;
    else if (floor2 >= roof2)
        return +1;
    else
        {
        uinta l1 = roof1 - floor1;
        uinta l2 = roof2 - floor2;
        uinta l = l1 < l2 ? l1 : l2;
        inta result = memcmp(floor1, floor2, l);
        return result != 0 ? result : l1 - l2;
        }
    }

bool VLKBinaryTrie::set(const uint8 *keyFloor, const uint8 *keyRoof, void *value, uinta *position, Node **n)
    {
    if (!keyFloor)
        {
        if (position) *position = AT_NULL;
        nullValue = value;
        if (flags & NULL_PRESENT)
            return YES;
        else
            {
            flags |= NULL_PRESENT;
            count++;
            keyChangeCount++;
            return NO;
            }
        }
    else if (keyFloor >= keyRoof)
        {
        if (position) *position = AT_ZERO_LEN;
        zeroLenValue = value;
        if (flags & ZERO_LEN_PRESENT)
            return YES;
        else
            {
            flags |= ZERO_LEN_PRESENT;
            count++;
            keyChangeCount++;
            return NO;
            }
        }
    else
        {
        if (position) *position = AT_ROOT;
        Run *r;
        uint8 *runRoof;
        if (trace(&keyFloor, keyRoof, &r, &runRoof))
            {
            Node *n2 = r->setEndValue(value);
            if (position) *n = n2;
            return YES;
            }
        else
            {
            Node *n2 = grow(r, runRoof, keyFloor, keyRoof, value);
            if (position) *n = n2;
            return NO;
            }
        }
    }

bool VLKBinaryTrie::get(const uint8 *keyFloor, const uint8 *keyRoof, void **value, uinta *position, Node **n)
    {
    if (position) *position = AT_BEFORE_START;
    if (!keyFloor)
        {
        if (flags & NULL_PRESENT)
            {
            if (position) *position = AT_NULL;
            *value = nullValue;
            return YES;
            }
        else
            return NO;
        }
    else if (keyFloor >= keyRoof)
        {
        if (flags & ZERO_LEN_PRESENT)
            {
            if (position) *position = AT_ZERO_LEN;
            *value = zeroLenValue;
            return YES;
            }
        else
            return NO;
        }
    else
        {
        Run *r;
        if (trace(&keyFloor, keyRoof, &r))
            {
            if (position)
                {
                *position = AT_ROOT;
                *n = r->endValueNode();
                }
            *value = r->endValue();
            return YES;
            }
        else
            return NO;
        }
    }

bool VLKBinaryTrie::setIfPresent(const uint8 *keyFloor, const uint8 *keyRoof, void *value, uinta *position, Node **n)
    {
    if (position) *position = AT_BEFORE_START;
    if (!keyFloor)
        {
        if (flags & NULL_PRESENT)
            {
            if (position) *position = AT_NULL;
            nullValue = value;
            return YES;
            }
        else
            return NO;
        }
    else if (keyFloor >= keyRoof)
        {
        if (flags & ZERO_LEN_PRESENT)
            {
            if (position) *position = AT_ZERO_LEN;
            zeroLenValue = value;
            return YES;
            }
        else
            return NO;
        }
    else
        {
        Run *r;
        if (trace(&keyFloor, keyRoof, &r))
            {
            if (position)
                {
                *position = AT_ROOT;
                *n = r->endValueNode();
                }
            r->setEndValue(value);
            return YES;
            }
        else
            return NO;
        }
    }

bool VLKBinaryTrie::getIfPresentElseSet(const uint8 *keyFloor, const uint8 *keyRoof, void **value, uinta *position, Node **n)
    {
    if (!keyFloor)
        {
        if (position) *position = AT_NULL;
        if (flags & NULL_PRESENT)
            {
            *value = nullValue;
            return YES;
            }
        else
            {
            flags |= NULL_PRESENT;
            count++;
            keyChangeCount++;
            nullValue = *value;
            return NO;
            }
        }
    else if (keyFloor >= keyRoof)
        {
        if (position) *position = AT_ZERO_LEN;
        if (flags & ZERO_LEN_PRESENT)
            {
            *value = zeroLenValue;
            return YES;
            }
        else
            {
            flags |= ZERO_LEN_PRESENT;
            count++;
            keyChangeCount++;
            zeroLenValue = *value;
            return NO;
            }
        }
    else
        {
        if (position) *position = AT_ROOT;
        Run *r;
        uint8 *runRoof;
        if (trace(&keyFloor, keyRoof, &r, &runRoof))
            {
            if (position) *n = r->endValueNode();
            *value = r->endValue();
            return YES;
            }
        else
            {
            Node *n2 = grow(r, runRoof, keyFloor, keyRoof, *value);
            if (position) *n = n2;
            return NO;
            }
        }
    }

bool VLKBinaryTrie::swap(const uint8 *keyFloor, const uint8 *keyRoof, void **value, uinta *position, Node **n)
    {
    if (!keyFloor)
        {
        if (position) *position = AT_NULL;
        if (flags & NULL_PRESENT)
            {
            void *temp = nullValue;
            nullValue = *value;
            *value = temp;
            return YES;
            }
        else
            {
            flags |= NULL_PRESENT;
            count++;
            keyChangeCount++;
            nullValue = *value;
            return NO;
            }
        }
    else if (keyFloor >= keyRoof)
        {
        if (position) *position = AT_ZERO_LEN;
        if (flags & ZERO_LEN_PRESENT)
            {
            void *temp = zeroLenValue;
            zeroLenValue = *value;
            *value = temp;
            return YES;
            }
        else
            {
            flags |= ZERO_LEN_PRESENT;
            count++;
            keyChangeCount++;
            zeroLenValue = *value;
            return NO;
            }
        }
    else
        {
        Run *r;
        uint8 *runRoof;
        if (trace(&keyFloor, keyRoof, &r, &runRoof))
            {
            void *temp;
            if (r->isTerminal)
                {
                if (position)
                    {
                    *position = AT_ROOT;
                    *n = r;
                    }
                temp = r->value;
                r->value = *value;
                }
            else
                {
                ShortEnd *se = r->link->asShortEnd();
                if (position)
                    {
                    *position = AT_ROOT;
                    *n = se;
                    }
                temp = se->value;
                se->value = *value;
                }
            *value = temp;
            return YES;
            }
        else
            {
            Node *n2 = grow(r, runRoof, keyFloor, keyRoof, *value);
            if (position)
                {
                *position = AT_ROOT;
                *n = n2;
                }
            return NO;
            }
        }
    }

bool VLKBinaryTrie::swapIfPresent(const uint8 *keyFloor, const uint8 *keyRoof, void **value, uinta *position, Node **n)
    {
    if (position) *position = AT_BEFORE_START;
    if (!keyFloor)
        {
        if (flags & NULL_PRESENT)
            {
            if (position) *position = AT_NULL;
            void *temp = nullValue;
            nullValue = *value;
            *value = temp;
            return YES;
            }
        else
            return NO;
        }
    else if (keyFloor >= keyRoof)
        {
        if (flags & ZERO_LEN_PRESENT)
            {
            if (position) *position = AT_ZERO_LEN;
            void *temp = zeroLenValue;
            zeroLenValue = *value;
            *value = temp;
            return YES;
            }
        else
            return NO;
        }
    else
        {
        Run *r;
        if (trace(&keyFloor, keyRoof, &r))
            {
            void *temp;
            if (r->isTerminal)
                {
                if (position)
                    {
                    *position = AT_ROOT;
                    *n = r;
                    }
                temp = r->value;
                r->value = *value;
                }
            else
                {
                ShortEnd *se = r->link->asShortEnd();
                if (position)
                    {
                    *position = AT_ROOT;
                    *n = se;
                    }
                temp = se->value;
                se->value = *value;
                }
            *value = temp;
            return YES;
            }
        else
            return NO;
        }
    }

bool VLKBinaryTrie::expunge(const uint8 *keyFloor, const uint8 *keyRoof, void **value)
    {
    if (!keyFloor)
        {
        if (flags & NULL_PRESENT)
            {
            if (value) *value = nullValue;
            flags &= ~NULL_PRESENT;
            count--;
            keyChangeCount++;
            return YES;
            }
        else
            return NO;
        }
    else if (keyFloor >= keyRoof)
        {
        if (flags & ZERO_LEN_PRESENT)
            {
            if (value) *value = zeroLenValue;
            flags &= ~ZERO_LEN_PRESENT;
            count--;
            keyChangeCount++;
            return YES;
            }
        else
            return NO;
        }
    else
        {
        Run *r;
        if (trace(&keyFloor, keyRoof, &r))
            {
            if (r->isTerminal)
                {
                if (value) *value = r->value;
                prune(r);
                }
            else
                {
                ShortEnd *se = r->link->asShortEnd();
                if (value) *value = se->value;
                prune(se);
                }
            return YES;
            }
        else
            return NO;
        }
    }

void VLKBinaryTrie::clear()
    {
    Node *n = root;
    root = 0;
    nullValue = zeroLenValue = 0;
    if (count) keyChangeCount++;
    count = flags = 0;
    if (n)
        {
        for ( ; ; )
            {
            for ( ; ; )
                {
                Node *nextN = n->down(0);
                if (nextN) n = nextN; else break;
                }
            for ( ; ; )
                {
                Node *nextN = n->backlink;
                free(n);
                if (nextN)
                    {
                    if (nextN->isDecision() && n == nextN->asDecision()->links[0])
                        {
                        n = nextN->asDecision()->links[1];
                        break;
                        }
                    else
                        n = nextN;
                    }
                else
                    return;
                }
            }
        }
    }

bool VLKBinaryTrie::trace(const uint8 **keyFloor, const uint8 *keyRoof, Run **runFound, uint8 **runRoofFound)
    {
    if (!root)
        {
        if (runFound) *runFound = 0;
        return NO;
        }
    uint8 *runScan, *runRoof;
    uinta delta;
    const uint8 *keyScan = *keyFloor;
    uint8 keyByte = *keyScan;
    Node *n = root;
    for ( ; ; )
        {
        switch (n->nodeType)
            {
            case DECISION:
                n = keyByte & n->bitMask ? n->asDecision()->links[1] : n->asDecision()->links[0];
                break;
            case RUN:
                runScan = n->asRun()->dataFloor();
                runRoof = n->asRun()->dataRoof;
                delta = compareBytes(runScan, runRoof, keyScan, keyRoof);
                keyScan += delta;
                runScan += delta;
                if (n->isTerminal || runScan < runRoof || keyScan >= keyRoof)
                    {
                    if (runFound) *runFound = n->asRun();
                    if (runRoofFound) *runRoofFound = runScan;
                    *keyFloor = keyScan;
                    return runScan >= runRoof && keyScan >= keyRoof && (n->isTerminal || n->asRun()->link->isShortEnd());
                    }
                keyByte = *keyScan;
                n = n->asRun()->link;
                break;
            case SHORT_END:
                n = n->asShortEnd()->link;
                break;
            default:
                crash(_T("VLKBinaryTrie bad nodeType"));
                break;
            }
        }
    return NO; // never gets here -- just to prevent eclipse IDE warning
    }

VLKBinaryTrie::Node *VLKBinaryTrie::grow(Run *r, uint8 *runRoof, const uint8 *keyFloor, const uint8 *keyRoof, void *value)
    {
    count++;
    keyChangeCount++;
    if (!root)
        return growAtRoot(keyFloor, keyRoof, value);
    else if (runRoof == r->dataFloor())
        return growAtStart(r, keyFloor, keyRoof, value);
    else if (runRoof == r->dataRoof)
        {
        if (keyRoof > keyFloor)
            return growAtEnd(r, keyFloor, keyRoof, value);
        else
            return growAtEnd(r, value);
        }
    else
        {
        if (keyRoof > keyFloor)
            return growInMiddle(r, runRoof, keyFloor, keyRoof, value);
        else
            return growInMiddle(r, runRoof, value);
        }
    }

VLKBinaryTrie::Node *VLKBinaryTrie::growAtRoot(const uint8 *keyFloor, const uint8 *keyRoof, void *value)
    {
    root = makeRun(0, keyFloor, keyRoof, YES);
    root->backlink = 0;
    root->asRun()->value = value;
    return root;
    }

VLKBinaryTrie::Node *VLKBinaryTrie::growAtStart(Run *r, const uint8 *keyFloor, const uint8 *keyRoof, void *value)
    {
    Decision *d = makeDecision(*keyFloor, *r->dataFloor());
    Node *otherBranch = r;
    for ( ; ; )
        {
        Node *up = otherBranch->backlink;
        if (!up || !up->isDecision() || d->bitMask < up->asDecision()->bitMask) break;
        otherBranch = up;
        }
    d->backlink = otherBranch->backlink;
    if (d->backlink)
        d->backlink->replaceLink(otherBranch, d);
    else
        root = d;
    Run *newRun = makeRun(r->dataFloorOffset, keyFloor, keyRoof, YES);
    newRun->value = value;
    newRun->backlink = otherBranch->backlink = d;
    if (d->bitMask & *keyFloor)
        {
        d->links[0] = otherBranch;
        d->links[1] = newRun;
        }
    else
        {
        d->links[0] = newRun;
        d->links[1] = otherBranch;
        }
    return newRun;
    }

VLKBinaryTrie::Node *VLKBinaryTrie::growInMiddle(Run *r, uint8 *runRoof, const uint8 *keyFloor, const uint8 *keyRoof, void *value)
    {
    Run *r1, *r2;
    divideRun(r, runRoof, &r1, &r2);
    Decision *d = makeDecision(*keyFloor, *r2->dataFloor());
    r1->link = d;
    d->backlink = r1;
    Run *newRun = makeRun(r2->dataFloorOffset, keyFloor, keyRoof, YES);
    newRun->value = value;
    newRun->backlink = r2->backlink = d;
    if (d->bitMask & *keyFloor)
        {
        d->links[0] = r2;
        d->links[1] = newRun;
        }
    else
        {
        d->links[0] = newRun;
        d->links[1] = r2;
        }
    return newRun;
    }

VLKBinaryTrie::Node *VLKBinaryTrie::growInMiddle(Run *r, uint8 *runRoof, void *value)
    {
    Run *r1, *r2;
    divideRun(r, runRoof, &r1, &r2);
    ShortEnd *se = makeShortEnd(value);
    r1->link = se;
    se->backlink = r1;
    se->link = r2;
    r2->backlink = se;
    return se;
    }

VLKBinaryTrie::Node *VLKBinaryTrie::growAtEnd(Run *r, const uint8 *keyFloor, const uint8 *keyRoof, void *value)
    {
    if (!r->isTerminal) crash(_T("VLKBinaryTrie OOPS run should be terminal but isn't"));
    r->isTerminal = NO;
    ShortEnd *se = makeShortEnd(r->value);
    r->link = se;
    se->backlink = r;
    Run *newRun = makeRun((uinta)r->dataRoof & OFFSET_MASK, keyFloor, keyRoof, YES);
    se->link = newRun;
    newRun->backlink = se;
    newRun->value = value;
    return newRun;
    }

VLKBinaryTrie::Node *VLKBinaryTrie::growAtEnd(Run *r, void *value)
    {
    if (r->isTerminal) crash(_T("VLKBinaryTrie OOPS run shouldn't be terminal but is"));
    ShortEnd *se = makeShortEnd(value);
    se->backlink = r;
    se->link = r->link;
    r->link = se;
    se->link->backlink = se;
    return se;
    }

void VLKBinaryTrie::prune(Node *n)
    {
    count--;
    keyChangeCount++;
    switch (n->nodeType)
        {
        case RUN:       pruneRun(n->asRun());                    break;
        case SHORT_END: pruneShortEnd(n->asShortEnd());          break;
        default:        crash(_T("VLKBinaryTrie bad nodeType")); break;
        }
    }

void VLKBinaryTrie::pruneRun(Run *r)
    {
    if (!r->isTerminal) crash(_T("VLKBinaryTrie OOPS run should be terminal but isn't"));
    if (!r->backlink)
        root = 0;
    else if (r->backlink->isDecision())
        pruneRunAfterDecision(r);
    else
        pruneRunAfterShortEnd(r);
    free(r);
    }

void VLKBinaryTrie::pruneRunAfterDecision(Run *r)
    {
    Decision *d = r->backlink->asDecision();
    Node *upper = d->backlink;
    Node *lower = d->links[d->links[0] == r ? 1 : 0];
    if (upper)
        {
        if (upper->isRun() && lower->isRun())
            mergeRuns(upper->asRun(), lower->asRun());
        else
            {
            upper->replaceLink(d, lower);
            lower->backlink = upper;
            }
        }
    else
        {
        root = lower;
        lower->backlink = 0;
        }
    free(d);
    }

void VLKBinaryTrie::pruneRunAfterShortEnd(Run *r)
    {
    ShortEnd *se = r->backlink->asShortEnd();
    Run *upperRun = se->backlink->asRun();
    upperRun->isTerminal = YES;
    upperRun->value = se->value;
    free(se);
    }

void VLKBinaryTrie::pruneShortEnd(ShortEnd *se)
    {
    if (se->link->isRun())
        mergeRuns(se->backlink->asRun(), se->link->asRun());
    else
        {
        se->link->backlink = se->backlink;
        se->backlink->asRun()->link = se->link;
        }
    free(se);
    }

void VLKBinaryTrie::divideRun(Run *r, uint8 *floor2, Run **r1, Run **r2)
    {
    uinta len1 = floor2 - r->dataFloor();
    uinta len2 = r->dataRoof - floor2;
    uinta rMaxLen = r->trailingLen + r->dataRoof - r->minDataFloor;
    uinta threshold = rMaxLen >> 2;
    if (threshold > MAX_TRAILING_LEN) threshold = MAX_TRAILING_LEN;
    threshold = rMaxLen - threshold;
    if (len1 > len2)
        {
        if (len1 > threshold || rMaxLen <= (sizeof(void*) << 1))
            divideRunKeep1st(r, floor2, r1, r2, len1, len2);
        else
            divideRunKeepNone(r, floor2, r1, r2, len1, len2);
        }
    else
        {
        if (len2 > threshold || rMaxLen <= (sizeof(void*) << 1))
            divideRunKeep2nd(r, floor2, r1, r2, len1, len2);
        else
            divideRunKeepNone(r, floor2, r1, r2, len1, len2);
        }
    }

void VLKBinaryTrie::divideRunKeep1st(Run *r, uint8 *floor2, Run **r1, Run **r2, uinta len1, uinta len2)
    {
    bool terminal = r->isTerminal;
    Run *new2 = makeRun((uinta)floor2 & OFFSET_MASK, floor2, r->dataRoof, terminal);
    r->trailingLen += len2;
    r->dataRoof = floor2;
    if (terminal)
        {
        r->isTerminal = NO;
        new2->value = r->value;
        }
    else
        {
        new2->link = r->link;
        new2->link->backlink = new2;
        }
    *r1 = r;
    *r2 = new2;
    }

void VLKBinaryTrie::divideRunKeep2nd(Run *r, uint8 *floor2, Run **r1, Run **r2, uinta len1, uinta len2)
    {
    Run *new1 = makeRun(r->dataFloorOffset, r->dataFloor(), floor2, NO);
    r->dataFloorOffset = (uinta)floor2 & OFFSET_MASK;
    memmove(r->dataFloor(), floor2, len2);
    r->trailingLen += len1;
    uint8 *newRDataRoof = r->dataFloor() + len2;
    r->trailingLen += r->dataRoof - newRDataRoof;
    r->dataRoof = newRDataRoof;
    new1->backlink = r->backlink;
    if (new1->backlink)
        new1->backlink->replaceLink(r, new1);
    else
        root = new1;
    *r1 = new1;
    *r2 = r;
    }

void VLKBinaryTrie::divideRunKeepNone(Run *r, uint8 *floor2, Run **r1, Run **r2, uinta len1, uinta len2)
    {
    Run *new1 = makeRun(r->dataFloorOffset, r->dataFloor(), floor2, NO);
    new1->backlink = r->backlink;
    if (new1->backlink)
        new1->backlink->replaceLink(r, new1);
    else
        root = new1;
    bool terminal = r->isTerminal;
    Run *new2 = makeRun((uinta)floor2 & OFFSET_MASK, floor2, r->dataRoof, terminal);
    if (terminal)
        new2->value = r->value;
    else
        {
        new2->link = r->link;
        new2->link->backlink = new2;
        }
    free(r);
    *r1 = new1;
    *r2 = new2;
    }

VLKBinaryTrie::Run *VLKBinaryTrie::mergeRuns(Run *r1, Run *r2)
    {
    uinta len1 = r1->dataRoof - r1->dataFloor();
    uinta len2 = r2->dataRoof - r2->dataFloor();
    uinta len = r1->dataFloorOffset + len1 + len2;
    uinta max1 = r1->trailingLen + r1->dataRoof - r1->minDataFloor;
    uinta max2 = r2->trailingLen + r2->dataRoof - r2->minDataFloor;
    if (max1 < max2)
        {
        if (len <= max1)
            return mergeRunDataForward(r1, r2, len2);
        else if (len <= max2)
            return mergeRunDataBack(r1, r2, len1 , len2);
        }
    else
        {
        if (len <= max2)
            return mergeRunDataBack(r1, r2, len1, len2);
        else if (len <= max1)
            return mergeRunDataForward(r1, r2, len2);
        }
    return makeMergedRun(r1, r2, len1, len2);
    }

VLKBinaryTrie::Run *VLKBinaryTrie::mergeRunDataForward(Run *r1, Run *r2, uinta len2)
    {
    memcpy(r1->dataRoof, r2->dataFloor(), len2);
    r1->trailingLen -= len2;
    r1->dataRoof += len2;
    r1->isTerminal = r2->isTerminal;
    if (r1->isTerminal)
        r1->value = r2->value;
    else
        {
        r1->link = r2->link;
        r1->link->backlink = r1;
        }
    free(r2);
    return r1;
    }

VLKBinaryTrie::Run *VLKBinaryTrie::mergeRunDataBack(Run *r1, Run *r2, uinta len1, uinta len2)
    {
    uint8 *r2MaxDataRoof = r2->dataRoof + r2->trailingLen;
    uint8 *r2Data = r2->dataFloor();
    r2->dataFloorOffset = r1->dataFloorOffset;
    memmove(r2->dataFloor() + len1, r2Data, len2);
    memcpy(r2->dataFloor(), r1->dataFloor(), len1);
    r2->dataRoof = r2->dataFloor() + len1 + len2;
    r2->trailingLen = r2MaxDataRoof - r2->dataRoof;
    r2->backlink = r1->backlink;
    if (r2->backlink)
        r2->backlink->replaceLink(r1, r2);
    else
        root = r2;
    free(r1);
    return r2;
    }

VLKBinaryTrie::Run *VLKBinaryTrie::makeMergedRun(Run *r1, Run *r2, uinta len1, uinta len2)
    {
    Run *newRun;
    if (r2->isTerminal)
        {
        newRun = makeRun(r1->dataFloorOffset, len1 + len2, YES);
        newRun->value = r2->value;
        }
    else
        {
        newRun = makeRun(r1->dataFloorOffset, len1 + len2, NO);
        newRun->link = r2->link;
        newRun->link->backlink = newRun;
        }
    memcpy(newRun->dataFloor(), r1->dataFloor(), len1);
    memcpy(newRun->dataFloor() + len1, r2->dataFloor(), len2);
    newRun->backlink = r1->backlink;
    if (newRun->backlink)
        newRun->backlink->replaceLink(r1, newRun);
    else
        root = newRun;
    free(r1);
    free(r2);
    return newRun;
    }

VLKBinaryTrie::Decision *VLKBinaryTrie::makeDecision(uint8 byteA, uint8 byteB)
    {
    Decision *d = (Decision*)malloc(sizeof(Decision));
    d->nodeType = DECISION;
    uint8 x = byteA ^ byteB;
    if (x & 0xF0)
        {
        if (x & 0xC0)
            d->bitMask = x & 0x80 ? 0x80 : 0x40;
        else
            d->bitMask = x & 0x20 ? 0x20 : 0x10;
        }
    else
        {
        if (x & 0x0C)
            d->bitMask = x & 0x08 ? 0x08 : 0x04;
        else
            d->bitMask = x & 0x02 ? 0x02 : 0x01;
        }
    return d;
    }

VLKBinaryTrie::Run *VLKBinaryTrie::makeRun(uinta offset, const uint8 *dataFloor, const uint8 *dataRoof, bool terminal)
    {
    uinta dataLen = dataRoof - dataFloor;
    Run *r = makeRun(offset, dataLen, terminal);
    memcpy(r->dataFloor(), dataFloor, dataLen);
    return r;
    }

VLKBinaryTrie::Run *VLKBinaryTrie::makeRun(uinta offset, uinta dataLen, bool terminal)
    {
    dataLen += offset;
    uinta bufferLen = alignLen(dataLen);
    Run *r = (Run*)malloc(sizeof(RunSizer) + bufferLen);
    r->nodeType = RUN;
    r->dataFloorOffset = offset;
    r->isTerminal = terminal ? YES : NO;
    r->dataRoof = r->minDataFloor + dataLen;
    r->trailingLen = bufferLen - dataLen;
    return r;
    }

VLKBinaryTrie::ShortEnd *VLKBinaryTrie::makeShortEnd(void *value)
    {
    ShortEnd *se = (ShortEnd*)malloc(sizeof(ShortEnd));
    se->nodeType = SHORT_END;
    se->value = value;
    return se;
    }



void VLKBinaryTrieIterator::reset(VLKBinaryTrie *vbt)
    {
    this->vbt = vbt;
    keyChangeCount = vbt->keyChangeCount;
    nextPosition = AT_BEFORE_START;
    top = 0;
    }

bool VLKBinaryTrieIterator::reset(VLKBinaryTrie *vbt, const uint8 **keyFloor, const uint8 *keyRoof, bool confine)
    {
    const uint8 *kf = *keyFloor;
    reset(vbt);
    bool keyFound = NO;
    nextN = 0;
    if (!kf)
        {
        if (vbt->flags & NULL_PRESENT)
            {
            nextPosition = AT_NULL;
            keyFound = YES;
            }
        else if (vbt->flags & ZERO_LEN_PRESENT)
            {
            nextPosition = AT_ZERO_LEN;
            keyFound = YES;
            }
        else if (vbt->root)
            nextN = vbt->root;
        }
    else if (kf >= keyRoof)
        {
        if (vbt->flags & ZERO_LEN_PRESENT)
            {
            nextPosition = AT_ZERO_LEN;
            keyFound = YES;
            }
        else if (vbt->root)
            nextN = vbt->root;
        }
    else
        {
        nextPosition = AT_AFTER_END;
        VLKBinaryTrie::Run *r;
        uint8 *runRoof;
        if (vbt->trace(keyFloor, keyRoof, &r, &runRoof))
            {
            nextN = r ? r->endValueNode() : 0;
            if (confine) top = r;
            keyFound = YES;
            }
        else
            {
            nextN = r;
            if (r)
                {
                if (runRoof == r->dataFloor())
                    {
                    for ( ; ; )
                        {
                        VLKBinaryTrie::Node *up = nextN->backlink;
                        if (!up) break;
                        nextN = up;
                        if (!up->isDecision()) break;
                        }
                    }
                if (confine && nextN->backlink) top = nextN->backlink;
                }
            }
        }
    if (nextN)
        {
        if (!nextN->isEndValueNode())
            for ( ; ; )
                {
                VLKBinaryTrie::Node *down = nextN->down(0);
                if (!down) break;
                nextN = down;
                if (nextN->isShortEnd()) break;
                }
        nextPosition = AT_ROOT;
        upNotDown = nextN->isRun() && nextN->isTerminal;
        }
    return keyFound;
    }

bool VLKBinaryTrieIterator::reset(VLKBinaryTrieReference *ref, bool confine)
    {
    if (!ref->isValid()) return NO;
    top = 0;
    vbt = ref->vbt;
    keyChangeCount = ref->keyChangeCount;
    nextPosition = ref->position;
    nextN = ref->n;
    if (nextN)
        {
        upNotDown = nextN->isRun() && nextN->isTerminal;
        if (confine) top = nextN->backlink;
        }
    return YES;
    }

void VLKBinaryTrieIterator::rewind()
    {
    nextPosition = AT_BEFORE_START;
    keyChangeCount = vbt->keyChangeCount;
    }

bool VLKBinaryTrieIterator::next(void **value)
    {
    if (!vbt) return NO;
    if (nextPosition == AT_BEFORE_START)
        {
        keyChangeCount = vbt->keyChangeCount;
        advanceNextPosition();
        }
    else if (keyChangeCount != vbt->keyChangeCount)
        return NO;
    position = nextPosition;
    if (position == AT_AFTER_END) return NO;
    expunged = NO;
    n = nextN;
    if (value) getValue(value);
    advanceNextPosition();
    return YES;
    }

void VLKBinaryTrieIterator::advanceNextPosition()
    {
    for ( ; ; )
        {
        switch (nextPosition)
            {
            case AT_BEFORE_START:
                nextPosition = AT_NULL;
                if (vbt->flags & NULL_PRESENT) return;
                break;
            case AT_NULL:
                nextPosition = AT_ZERO_LEN;
                if (vbt->flags & ZERO_LEN_PRESENT) return;
                break;
            case AT_ZERO_LEN:
                if (vbt->root)
                    {
                    nextPosition = AT_ROOT;
                    nextN = vbt->root;
                    if (nextN->isRun() && nextN->isTerminal)
                        {
                        upNotDown = YES;
                        return;
                        }
                    else
                        upNotDown = NO;
                    }
                else
                    {
                    nextPosition = AT_AFTER_END;
                    return;
                    }
                break;
            case AT_ROOT:
                while (upNotDown)
                    {
                    VLKBinaryTrie::Node *up = nextN->backlink;
                    if (!up || up == top)
                        {
                        nextPosition = AT_AFTER_END;
                        return;
                        }
                    if (up->isDecision() && up->asDecision()->links[0] == nextN)
                        {
                        nextN = up->asDecision()->links[1];
                        if (nextN->isRun() && nextN->isTerminal)
                            return;
                        else
                            upNotDown = NO;
                        }
                    else
                        nextN = up;
                    }
                while (!upNotDown)
                    {
                    nextN = nextN->down(0);
                    if (nextN->isShortEnd()) return;
                    if (nextN->isRun() && nextN->isTerminal)
                        {
                        upNotDown = YES;
                        return;
                        }
                    }
                break;
            default:
                break;
            }
        }
    }

bool VLKBinaryTrieIterator::get(void **value)
    {
    if (!isValid() || expunged) return NO;
    getValue(value);
    return YES;
    }

bool VLKBinaryTrieIterator::set(void *value)
    {
    if (!isValid() || expunged) return NO;
    setValue(value);
    return YES;
    }

bool VLKBinaryTrieIterator::swap(void **value)
    {
    if (!isValid() || expunged) return NO;
    void *temp;
    getValue(&temp);
    setValue(*value);
    *value = temp;
    return YES;
    }

bool VLKBinaryTrieIterator::expunge()
    {
    if (!isValid() || expunged) return NO;
    switch (position)
        {
        case AT_NULL:
            vbt->flags &= ~NULL_PRESENT;
            vbt->count--;
            vbt->keyChangeCount++;
            break;
        case AT_ZERO_LEN:
            vbt->flags &= ~ZERO_LEN_PRESENT;
            vbt->count--;
            vbt->keyChangeCount++;
            break;
        case AT_ROOT:
            if (nextPosition == AT_ROOT)
                {
                switch (n->nodeType)
                    {
                    case RUN:
                        if (n->backlink && n->backlink->isDecision())
                            {
                            VLKBinaryTrie::Decision *d = n->backlink->asDecision();
                            if (n == d->links[0] && nextN == d->links[1] && d->backlink && d->backlink->isRun() && d->links[1]->isRun())
                                {
                                VLKBinaryTrie::Node *parking = d->backlink->backlink;
                                uinta index = parking && parking->isDecision() && parking->asDecision()->links[1] == d->backlink ? 1 : 0;
                                vbt->prune(n);
                                n = 0;
                                nextN = parking ? parking->down(index) : vbt->root;
                                }
                            }
                        break;
                    case SHORT_END:
                        if (n->asShortEnd()->link == nextN)
                            {
                            VLKBinaryTrie::Node *parking = n->backlink->backlink;
                            uinta index = parking && parking->isDecision() && parking->asDecision()->links[1] == n->backlink ? 1 : 0;
                            vbt->prune(n);
                            n = 0;
                            nextN = parking ? parking->down(index) : vbt->root;
                            }
                        break;
                    }
                }
            if (n) vbt->prune(n);
            break;
        default:
            return NO;
        }
    keyChangeCount++;
    expunged = YES;
    return YES;
    }

bool VLKBinaryTrieIterator::keyIsNull()
    {
    return isValid() && !expunged && position == AT_NULL;
    }

bool VLKBinaryTrieIterator::computeKeyLen(uinta *len)
    {
    if (!isValid() || expunged) return NO;
    uinta l = 0;
    if (position == AT_ROOT)
        for (VLKBinaryTrie::Node *scan = n; scan; scan = scan->backlink)
            if (scan->isRun()) l += scan->asRun()->dataRoof - scan->asRun()->dataFloor();
    *len = l;
    return YES;
    }

bool VLKBinaryTrieIterator::regenerateKey(uint8 *buffer, uinta keyLen)
    {
    if (!isValid() || expunged) return NO;
    if (position == AT_ROOT)
        {
        uint8 *out = buffer + keyLen;
        for (VLKBinaryTrie::Node *scan = n; scan; scan = scan->backlink)
            if (scan->isRun())
                {
                uinta len = scan->asRun()->dataRoof - scan->asRun()->dataFloor();
                out -= len;
                memcpy(out, scan->asRun()->dataFloor(), len);
                }
        }
    return YES;
    }

void VLKBinaryTrieIterator::getValue(void **value)
    {
    switch (position)
        {
        case AT_NULL:     *value = vbt->nullValue;    break;
        case AT_ZERO_LEN: *value = vbt->zeroLenValue; break;
        case AT_ROOT:     n->getValue(value);         break;
        default:                                      break;
        }
    }

void VLKBinaryTrieIterator::setValue(void *value)
    {
    switch (position)
        {
        case AT_NULL:     vbt->nullValue = value;    break;
        case AT_ZERO_LEN: vbt->zeroLenValue = value; break;
        case AT_ROOT:     n->setValue(value);        break;
        default:                                     break;
        }
    }



bool VLKBinaryTrieReference::set(VLKBinaryTrie *vbt, const uint8 *keyFloor, const uint8 *keyRoof, void *value)
    {
    this->vbt = vbt;
    keyChangeCount = vbt->keyChangeCount;
    return vbt->set(keyFloor, keyRoof, value, &position, &n);
    }

bool VLKBinaryTrieReference::setIfPresent(VLKBinaryTrie *vbt, const uint8 *keyFloor, const uint8 *keyRoof, void *value)
    {
    if (vbt->setIfPresent(keyFloor, keyRoof, value, &position, &n))
        {
        keyChangeCount = vbt->keyChangeCount;
        this->vbt = vbt;
        return YES;
        }
    else
        {
        this->vbt = 0;
        return NO;
        }
    }

bool VLKBinaryTrieReference::get(VLKBinaryTrie *vbt, const uint8 *keyFloor, const uint8 *keyRoof, void **value)
    {
    if (vbt->get(keyFloor, keyRoof, value, &position, &n))
        {
        keyChangeCount = vbt->keyChangeCount;
        this->vbt = vbt;
        return YES;
        }
    else
        {
        this->vbt = 0;
        return NO;
        }
    }

bool VLKBinaryTrieReference::getIfPresentElseSet(VLKBinaryTrie *vbt, const uint8 *keyFloor, const uint8 *keyRoof, void **value)
    {
    this->vbt = vbt;
    keyChangeCount = vbt->keyChangeCount;
    return vbt->getIfPresentElseSet(keyFloor, keyRoof, value, &position, &n);
    }

bool VLKBinaryTrieReference::swap(VLKBinaryTrie *vbt, const uint8 *keyFloor, const uint8 *keyRoof, void **value)
    {
    this->vbt = vbt;
    keyChangeCount = vbt->keyChangeCount;
    return vbt->swap(keyFloor, keyRoof, value, &position, &n);
    }

bool VLKBinaryTrieReference::swapIfPresent(VLKBinaryTrie *vbt, const uint8 *keyFloor, const uint8 *keyRoof, void **value)
    {
    if (vbt->swapIfPresent(keyFloor, keyRoof, value, &position, &n))
        {
        keyChangeCount = vbt->keyChangeCount;
        this->vbt = vbt;
        return YES;
        }
    else
        {
        this->vbt = 0;
        return NO;
        }
    }

bool VLKBinaryTrieReference::positionTo(VLKBinaryTrieIterator *iterator)
    {
    if (!iterator->isValid()) return NO;
    switch (iterator->position)
        {
        case AT_BEFORE_START: case AT_AFTER_END: return NO;
        }
    vbt = iterator->vbt;
    keyChangeCount = iterator->keyChangeCount;
    position = iterator->position;
    n = iterator->n;
    return YES;
    }

bool VLKBinaryTrieReference::get(void **value)
    {
    if (!isValid()) return NO;
    getValue(value);
    return YES;
    }

bool VLKBinaryTrieReference::set(void *value)
    {
    if (!isValid()) return NO;
    setValue(value);
    return YES;
    }

bool VLKBinaryTrieReference::swap(void **value)
    {
    if (!isValid()) return NO;
    void *temp;
    getValue(&temp);
    setValue(*value);
    *value = temp;
    return YES;
    }

bool VLKBinaryTrieReference::expunge()
    {
    if (!isValid()) return NO;
    switch (position)
        {
        case AT_NULL:
            vbt->flags &= ~NULL_PRESENT;
            vbt->count--;
            vbt->keyChangeCount++;
            break;
        case AT_ZERO_LEN:
            vbt->flags &= ~ZERO_LEN_PRESENT;
            vbt->count--;
            vbt->keyChangeCount++;
            break;
        case AT_ROOT:
            vbt->prune(n);
            break;
        default:
            return NO;
        }
    vbt = 0;
    return YES;
    }

void VLKBinaryTrieReference::getValue(void **value)
    {
    switch (position)
        {
        case AT_NULL:     *value = vbt->nullValue;    break;
        case AT_ZERO_LEN: *value = vbt->zeroLenValue; break;
        case AT_ROOT:     n->getValue(value);         break;
        default:                                      break;
        }
    }

void VLKBinaryTrieReference::setValue(void *value)
    {
    switch (position)
        {
        case AT_NULL:     vbt->nullValue = value;    break;
        case AT_ZERO_LEN: vbt->zeroLenValue = value; break;
        case AT_ROOT:     n->setValue(value);        break;
        default:                                     break;
        }
    }



#ifdef TEST_VLK_BINARY_TRIE

void VLKBinaryTrie::dump()
    {
    if (!count) zprintf(_T("Empty VLKBinaryTrie\n"));
    if (flags & NULL_PRESENT) zprintf(_T("NULL@%yu\n"), nullValue);
    if (flags & ZERO_LEN_PRESENT) zprintf(_T("ZERO_LEN@%yu\n"), zeroLenValue);
    if (root) dumpNode(root);
    }

void VLKBinaryTrie::dumpNode(Node *n)
    {
    dumpNode(prefixFloor, prefixFloor, n);
    }

void VLKBinaryTrie::dumpNode(TCHAR *decisionRoof, TCHAR *currentRoof, Node *n)
    {
    if (n->isDecision())
        {
        while (decisionRoof < currentRoof) *decisionRoof++ = _T(' ');
        *decisionRoof++ = _T(' ');
        TCHAR *marker = decisionRoof;
        *decisionRoof++ = _T('|');
        currentRoof += nodeTextOut(n);
        while (decisionRoof < currentRoof) *decisionRoof++ = _T(' ');
        dumpNode(decisionRoof, currentRoof, n->asDecision()->links[0]);
        *marker = _T('>');
        zprintf(_T("%.*s"), decisionRoof - prefixFloor, prefixFloor);
        *marker = _T(' ');
        dumpNode(decisionRoof, currentRoof, n->asDecision()->links[1]);
        }
    else
        {
        currentRoof += nodeTextOut(n);
        if (!n->isTerminal) dumpNode(decisionRoof, currentRoof, n->down(0));
        }
    }

uinta VLKBinaryTrie::nodeTextOut(Node *n)
    {
    Run *r = 0;
    int bitPos = 99;
    switch(n->nodeType)
        {
        case DECISION:
            switch (n->asDecision()->bitMask)
                {
                case 0x01: bitPos = 0; break;
                case 0x02: bitPos = 1; break;
                case 0x04: bitPos = 2; break;
                case 0x08: bitPos = 3; break;
                case 0x10: bitPos = 4; break;
                case 0x20: bitPos = 5; break;
                case 0x40: bitPos = 6; break;
                case 0x80: bitPos = 7; break;
                }
            return zprintf(_T(" D%d"), bitPos);
        case RUN:
            r = n->asRun();
            if (r->isTerminal)
                return zprintf(_T(" T\"%.*s\"@%yd\n"), r->dataRoof - r->dataFloor(), r->dataFloor(), r->value);
            else
                return zprintf(_T(" R\"%.*s\""), r->dataRoof - r->dataFloor(), r->dataFloor());
        case SHORT_END:
            return zprintf(_T(" S@%yd"), n->asShortEnd()->value);
        default:
            return zprintf(_T(" ?????"));
        }
    }

#endif
