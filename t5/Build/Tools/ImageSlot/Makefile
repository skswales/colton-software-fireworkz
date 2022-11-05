# Makefile for ImageSlot utility

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

# Copyright © 1991-1998 Colton Software Limited
# Copyright © 1998-2015 R W Colton

ImageSlot: o.ImageSlot
 link -Utility -o $@ $?

o.ImageSlot: s.ImageSlot
 @CDir o
 objasm -o $@ --arm --arm_only -throwback $?

