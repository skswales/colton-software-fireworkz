#!/bin/bash

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Copyright (C) 2016 Stuart Swales

export LANG=C

cd `dirname $0`
mkdir Temp > /dev/null 2>&1
cd Temp
# Filter Colton Sofware word lists
grep -v "z$" ../CwA_src.txt | cut -f1 > CwAs
grep -v "z$" ../CwC_src.txt | cut -f1 > CwCs

grep -v "s$" ../CwA_src.txt | cut -f1 > CwAz
grep -v "s$" ../CwC_src.txt | cut -f1 > CwCz

SCOWL=/cygdrive/N/coltsoft/$FIREWORKZ_TBT/cs-free/SCOWL
# Split the header off
awk '{print >out}; /---/{out="wl-GBs"}' out=/dev/null \
 $SCOWL/wordlist-strip-70-0-GBs.txt
awk '{print >out}; /---/{out="wl-GBz"}' out=/dev/null \
 $SCOWL/wordlist-strip-70-0-GBz.txt

# Split into Non/Caps files, without any 's suffixed words
grep -v "^[A-Z]" wl-GBs | grep -v -f ../ignore-words.txt | grep -v "'s$" > wlA-GBs
grep    "^[A-Z]" wl-GBs | grep -v -f ../ignore-words.txt | grep -v "'s$" > wlC-GBs

grep -v "^[A-Z]" wl-GBz | grep -v -f ../ignore-words.txt | grep -v "'s$" > wlA-GBz
grep    "^[A-Z]" wl-GBz | grep -v -f ../ignore-words.txt | grep -v "'s$" > wlC-GBz

# Merge lists (for Fireworkz)
# cat CwAs wlA-GBs | sort | uniq > merged-wlA-GBs
# cat CwCs wlC-GBs | sort | uniq > merged-wlC-GBs

# cat CwAz wlA-GBz | sort | uniq > merged-wlA-GBz
# cat CwCz wlC-GBz | sort | uniq > merged-wlC-GBz

cat CwAs CwAz wlA-GBs wlA-GBz | \
sort | \
uniq > merged-wlA-GB
cat CwCs CwCz wlC-GBs wlC-GBz | \
sort | \
uniq > merged-wlC-GB

# Merge lists (for PipeDream)
cat CwAs wlA-GBs CwCs wlC-GBs | \
sort --ignore-case | \
uniq --ignore-case > merged-wl-GBs

cat CwAz wlA-GBz CwCz wlC-GBz | \
sort --ignore-case | \
uniq --ignore-case > merged-wl-GBz

ls -ltr
