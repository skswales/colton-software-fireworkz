# MakeResp

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

# Copyright ? 2013-2022 Stuart Swales

CCflags = -c -throwback -DRELEASED  

# Final targets:
@.MakeResp: @.o.MakeResp
 link -aif -o $@ $? c:o.Stubs

# Static dependencies:
@.o.MakeResp:   @.c.MakeResp
 @CDir o
 cc -o $@ $(CCflags) $?
