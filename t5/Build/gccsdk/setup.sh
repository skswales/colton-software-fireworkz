#!/bin/sh

#rem Build\gccsdk\setup.sh

#rem This Source Code Form is subject to the terms of the Mozilla Public
#rem License, v. 2.0. If a copy of the MPL was not distributed with this
#rem file, You can obtain one at http://mozilla.org/MPL/2.0/.

#rem Copyright (C) 2013-2019 Stuart Swales

#rem Execute from top-level t5 directory (move as t5/external/setup-gccsdk.sh soon)

COLTSOFT_CS_NONFREE=../../../coltsoft/$FIREWORKZ_TBT/cs-nonfree

[ ! -d ./external/Microsoft/Excel97SDK/INCLUDE ] && mkdir       ./external/Microsoft/Excel97SDK/INCLUDE
cp $COLTSOFT_CS_NONFREE/Microsoft/Excel_97_SDK/INCLUDE/XLCALL.H ./external/Microsoft/Excel97SDK/INCLUDE/xlcall.h

#rem Patched copy of BTTNCUR from 'Inside OLE 2' for Fireworkz

BTTNCURP_FILE=./external/Microsoft/InsideOLE2/BTTNCURP/BTTNCUR.C
[ -f $BTTNCUR_FILE ] && rm $BTTNCURP_FILE
BTTNCURP_FILE=./external/Microsoft/InsideOLE2/BTTNCURP/BTTNCUR.H
[ -f $BTTNCUR_FILE ] && rm $BTTNCURP_FILE
BTTNCURP_FILE=./external/Microsoft/InsideOLE2/BTTNCURP/BTTNCURI.H
[ -f $BTTNCUR_FILE ] && rm $BTTNCURP_FILE

BTTNCUR_SOURCE=./external/Microsoft/InsideOLE2/BTTNCUR
if [ -d $COLTSOFT_CS_NONFREE/Microsoft/InsideOLE2/BTTNCUR ];
then
  BTTNCUR_SOURCE=$COLTSOFT_CS_NONFREE/Microsoft/InsideOLE2/BTTNCUR
fi

patch -b --verbose $BTTNCUR_SOURCE/BTTNCUR.C -o ./external/Microsoft/InsideOLE2/BTTNCURP/BTTNCUR.C -i ./external/Microsoft/InsideOLE2/BTTNCURP/BTTNCUR.C.patch

patch -b --verbose $BTTNCUR_SOURCE/BTTNCUR.H -o ./external/Microsoft/InsideOLE2/BTTNCURP/BTTNCUR.H -i ./external/Microsoft/InsideOLE2/BTTNCURP/BTTNCUR.H.patch

cp $BTTNCUR_SOURCE/BTTNCURI.H ./external/Microsoft/InsideOLE2/BTTNCURP
