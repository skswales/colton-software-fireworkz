/* mathxcpp.cpp */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2019-2022 Stuart Swales */

/* Additional math routines */

#include "common/gflags.h"

#include <cstdlib>
#include <cmath>

#if defined(USE_RNG_STD_MT19937)

static bool
uniform_distribution_seeded = false;

#pragma warning(disable:4548) // expression before comma has no effect; expected expression with side-effect
#pragma warning(disable:4571) // Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
#pragma warning(disable:4625) // 'class': copy constructor was implicitly defined as deleted
#pragma warning(disable:4626) // 'class': assignment operator was implicitly defined as deleted
#pragma warning(disable:4820) // 'class': 'n' bytes padding added after data member 'm'
#pragma warning(disable:5026) // 'class': move constructor was implicitly defined as deleted
#pragma warning(disable:5027) // 'class': move assignment operator was implicitly defined as deleted

#include <random>

/* Mersenne Twister Engine */

static std::normal_distribution<> random_number_normal_distribution(0.0, 1.0); // Produces random floating-point values normally distributed with mean zero and standard deviation one

static std::uniform_real_distribution<> random_number_uniform_distribution(0.0, 1.0); // Produces random floating-point values uniformly distributed on the interval [0, 1)

static std::mt19937_64 random_number_generator;

_Check_return_
extern "C" double
normal_distribution(void)
{
    return(random_number_normal_distribution(random_number_generator));
}

/* Generates a random number on [0,1) */ 

_Check_return_
extern "C" double
uniform_distribution(void)
{
    return(random_number_uniform_distribution(random_number_generator));
}

extern "C" void
uniform_distribution_seed(
    _In_    const unsigned int seed)
{
    uniform_distribution_seeded = true;

    random_number_generator.seed(seed);
}

#include <time.h>

/*ncr*/
extern "C" bool
uniform_distribution_test_seeded(
    _In_    const bool fEnsure)
{
    if(!fEnsure)
        return(uniform_distribution_seeded);

    if(!uniform_distribution_seeded)
    {
        const unsigned int seed = (unsigned int) time(NULL);
        uniform_distribution_seed(seed);
    }

    return(true);
}

#endif /* USE_RNG_STD_MT19937 */

/* end of mathxcpp.c */
