# MakeMess

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

# Copyright ? 1990-1998 Colton Software Limited
# Copyright ? 1998-2015 R W Colton

CCflags = -c -throwback -DRELEASED  

# Final targets:
@.MakeMess: @.o.MakeMess
 link -aif -o $@ $? c:o.Stubs

# Static dependencies:
@.o.MakeMess:   @.c.MakeMess
 @CDir o
 cc -o $@ $(CCflags) $?
