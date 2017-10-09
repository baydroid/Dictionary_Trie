#   Copyright (c) 2017 transmission.aquitaine@yahoo.com
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http:#www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

GPP_OPTS  = -O3 -D_REENTRANT -fPIC -c -Wall -Werror -Wwrite-strings
LINK_OPTS = -fPIC
#GPP_OPTS  = -g -D_REENTRANT -DTEST_VLK_BINARY_TRIE -fPIC -c -Wall -Werror -Wwrite-strings
#LINK_OPTS = -fPIC -g

all : ../bin/test_vlk_binary_trie ../bin/byte_compare

clean :
	rm -vf addr_width.h
	rm -vf ../o/*
	rm -vf ../bin/*

../o/make_addr_width.o : make_addr_width.cpp
	g++ $(GPP_OPTS) $< -o  $@

../bin/make_addr_width : ../o/make_addr_width.o
	g++ $(LINK_OPTS) -o $@ ../o/make_addr_width.o

addr_width.h : ../bin/make_addr_width
	../bin/make_addr_width

../o/byte_compare.o : byte_compare.cpp addr_width.h basic_types.h
	g++ $(GPP_OPTS) $< -o $@

../o/vlk_binary_trie.o : vlk_binary_trie.cpp addr_width.h basic_types.h crash.h vlk_binary_trie.h zprintf_etc.h
	g++ $(GPP_OPTS) $< -o $@

../o/test_vlk_binary_trie.o : test_vlk_binary_trie.cpp addr_width.h basic_types.h vlk_binary_trie.h zprintf_etc.h
	g++ $(GPP_OPTS) $< -o $@

../o/reporting_malloc_free.o : reporting_malloc_free.c addr_width.h basic_types.h zprintf_etc.h
	g++ $(GPP_OPTS) $< -o $@

../o/zprintf_etc.o : zprintf_etc.c zprintf_etc.h
	g++ $(GPP_OPTS) $< -o $@

../bin/test_vlk_binary_trie : ../o/test_vlk_binary_trie.o ../o/vlk_binary_trie.o ../o/zprintf_etc.o
	g++ $(LINK_OPTS) -o ../bin/test_vlk_binary_trie ../o/test_vlk_binary_trie.o ../o/vlk_binary_trie.o ../o/zprintf_etc.o

../bin/byte_compare : ../o/byte_compare.o ../o/zprintf_etc.o
	g++ $(LINK_OPTS) -o ../bin/byte_compare ../o/byte_compare.o ../o/zprintf_etc.o
