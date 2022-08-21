/* MakeResp.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2013-2018 Stuart Swales */

#include <stdlib.h>
#include <stdio.h>

int main(int argc, char * argv[])
{
    int i;

    for(i = 1; i < argc; ++i)
    {
        fputs(argv[i], stdout);
        fputs("\n", stdout);
    }

    return(EXIT_SUCCESS);
}

/* end of MakeResp.c */
