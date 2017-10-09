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
#ifndef VLK_BINARY_TRIE_H_
#define VLK_BINARY_TRIE_H_



class VLKBinaryTrieIterator;
class VLKBinaryTrieReference;



class VLKBinaryTrie
    {
public:
    static inta keycmp(const uint8 *floor1, const uint8 *roof1, const uint8 *floor2, const uint8 *roof2);
    VLKBinaryTrie()                                                                     { memset(this, 0, sizeof(*this));                             }
    ~VLKBinaryTrie()                                                                    { clear();                                                    }
    bool set(const uint8 *keyFloor, const uint8 *keyRoof, void *value)                  { return set(keyFloor, keyRoof, value, 0, 0);                 }
    bool setIfPresent(const uint8 *keyFloor, const uint8 *keyRoof, void *value)         { return setIfPresent(keyFloor, keyRoof, value, 0, 0);        }
    bool get(const uint8 *keyFloor, const uint8 *keyRoof, void **value)                 { return get(keyFloor, keyRoof, value, 0, 0);                 }
    bool getIfPresentElseSet(const uint8 *keyFloor, const uint8 *keyRoof, void **value) { return getIfPresentElseSet(keyFloor, keyRoof, value, 0, 0); }
    bool swap(const uint8 *keyFloor, const uint8 *keyRoof, void **value)                { return swap(keyFloor, keyRoof, value, 0, 0);                }
    bool swapIfPresent(const uint8 *keyFloor, const uint8 *keyRoof, void **value)       { return swapIfPresent(keyFloor, keyRoof, value, 0, 0);       }
    bool expunge(const uint8 *keyFloor, const uint8 *keyRoof, void **value = 0);
    bool contains(const uint8 *keyFloor, const uint8 *keyRoof)                          { return trace(&keyFloor, keyRoof);                           }
    void clear();
    uinta size()                                                                        { return count;                                               }

#ifdef TEST_VLK_BINARY_TRIE
    void dump();
#endif

private:
    friend class VLKBinaryTrieIterator;
    friend class VLKBinaryTrieReference;

    struct Node;
    struct Decision;
    struct RunSizer;
    struct Run;
    struct ShortEnd;

    Node *root;
    void *nullValue;
    void *zeroLenValue;
    uinta flags;
    uinta count;
    uinta keyChangeCount;

    bool set(const uint8 *keyFloor, const uint8 *keyRoof, void *value, uinta *position, Node **n);
    bool setIfPresent(const uint8 *keyFloor, const uint8 *keyRoof, void *value, uinta *position, Node **n);
    bool get(const uint8 *keyFloor, const uint8 *keyRoof, void **value, uinta *position, Node **n);
    bool getIfPresentElseSet(const uint8 *keyFloor, const uint8 *keyRoof, void **value, uinta *position, Node **n);
    bool swap(const uint8 *keyFloor, const uint8 *keyRoof, void **value, uinta *position, Node **n);
    bool swapIfPresent(const uint8 *keyFloor, const uint8 *keyRoof, void **value, uinta *position, Node **n);
    bool trace(const uint8 **keyFloor, const uint8 *keyRoof, Run **runFound = 0, uint8 **runRoofFound = 0);
    Node *grow(Run *r, uint8 *runRoof, const uint8 *keyFloor, const uint8 *keyRoof, void *value);
    Node *growAtRoot(const uint8 *keyFloor, const uint8 *keyRoof, void *value);
    Node *growAtStart(Run *r, const uint8 *keyFloor, const uint8 *keyRoof, void *value);
    Node *growInMiddle(Run *r, uint8 *runRoof, const uint8 *keyFloor, const uint8 *keyRoof, void *value);
    Node *growInMiddle(Run *r, uint8 *runRoof, void *value);
    Node *growAtEnd(Run *r, const uint8 *keyFloor, const uint8 *keyRoof, void *value);
    Node *growAtEnd(Run *r, void *value);
    void prune(Node *n);
    void pruneRun(Run *r);
    void pruneRunAfterDecision(Run *r);
    void pruneRunAfterShortEnd(Run *r);
    void pruneShortEnd(ShortEnd *se);
    void divideRun(Run *r, uint8 *floor2, Run **r1, Run **r2);
    void divideRunKeep1st(Run *r, uint8 *floor2, Run **r1, Run **r2, uinta len1, uinta len2);
    void divideRunKeep2nd(Run *r, uint8 *floor2, Run **r1, Run **r2, uinta len1, uinta len2);
    void divideRunKeepNone(Run *r, uint8 *floor2, Run **r1, Run **r2, uinta len1, uinta len2);
    Run *mergeRuns(Run *r1, Run *r2);
    Run *mergeRunDataForward(Run *r1, Run *r2, uinta len2);
    Run *mergeRunDataBack(Run *r1, Run *r2, uinta len1, uinta len2);
    Run *makeMergedRun(Run *r1, Run *r2, uinta len1, uinta len2);
    Decision *makeDecision(uint8 byteA, uint8 byteB);
    Run *makeRun(uinta offset, const uint8 *dataFloor, const uint8 *dataRoof, bool terminal);
    Run *makeRun(uinta offset, uinta dataLen, bool terminal);
    ShortEnd *makeShortEnd(void *value);

#ifdef TEST_VLK_BINARY_TRIE
    TCHAR prefixFloor[0x1000];
    void dumpNode(Node *n);
    void dumpNode(TCHAR *decisionRoof, TCHAR *currentRoof, Node *n);
    uinta nodeTextOut(Node *n);
#endif
    };



class VLKBinaryTrieIterator
    {
public:
    VLKBinaryTrieIterator() { vbt = 0; keyChangeCount = position = nextPosition = 0; top = n = nextN = 0; upNotDown = expunged = NO; }
    bool isValid() { return vbt && vbt->keyChangeCount == keyChangeCount; }
    void reset(VLKBinaryTrie *vbt);
    bool reset(VLKBinaryTrie *vbt, const uint8 **keyFloorPtr, const uint8 *keyRoof, bool confine);
    bool reset(VLKBinaryTrieReference *ref, bool confine);
    void rewind();
    bool next(void **value = 0);
    bool get(void **value);
    bool set(void *value);
    bool swap(void **value);
    bool expunge();
    bool keyIsNull();
    bool computeKeyLen(uinta *len);
    bool regenerateKey(uint8 *buffer, uinta keyLen);

private:
    friend class VLKBinaryTrieReference;

    VLKBinaryTrie *vbt;
    uinta keyChangeCount;
    uinta position;
    VLKBinaryTrie::Node *top;
    VLKBinaryTrie::Node *n;
    uinta nextPosition;
    VLKBinaryTrie::Node *nextN;
    bool8 upNotDown;
    bool8 expunged;

    void advanceNextPosition();
    void getValue(void **value);
    void setValue(void *value);
    };



class VLKBinaryTrieReference
    {
public:
    VLKBinaryTrieReference() { vbt = 0; keyChangeCount = position = 0; n = 0; }
    bool isValid() { return vbt && vbt->keyChangeCount == keyChangeCount; }
    bool set(VLKBinaryTrie *vbt, const uint8 *keyFloor, const uint8 *keyRoof, void *value);
    bool setIfPresent(VLKBinaryTrie *vbt, const uint8 *keyFloor, const uint8 *keyRoof, void *value);
    bool get(VLKBinaryTrie *vbt, const uint8 *keyFloor, const uint8 *keyRoof, void **value);
    bool getIfPresentElseSet(VLKBinaryTrie *vbt, const uint8 *keyFloor, const uint8 *keyRoof, void **value);
    bool swap(VLKBinaryTrie *vbt, const uint8 *keyFloor, const uint8 *keyRoof, void **value);
    bool swapIfPresent(VLKBinaryTrie *vbt, const uint8 *keyFloor, const uint8 *keyRoof, void **value);
    bool positionTo(VLKBinaryTrieIterator *iterator);
    bool get(void **value);
    bool set(void *value);
    bool swap(void **value);
    bool expunge();

private:
    friend class VLKBinaryTrieIterator;

    VLKBinaryTrie *vbt;
    uinta keyChangeCount;
    uinta position;
    VLKBinaryTrie::Node *n;

    void getValue(void **value);
    void setValue(void *value);
    };



#endif /* VLK_BINARY_TRIE_H_ */
