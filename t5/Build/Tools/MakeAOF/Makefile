# MakeAOF

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Copyright (C) 1995-1998 Colton Software Limited
# Copyright (C) 1998-2015 R W Colton

CCflags = -c -throwback -DRELEASED  

# Final targets:
@.MakeAOF: @.o.MakeAOF
 link -aif -o $@ $? c:o.Stubs

# Static dependencies:
@.o.MakeAOF:   @.c.MakeAOF
 @CDir o
 cc -o $@ $(CCflags) $?
