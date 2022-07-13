/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/* author: Matthias Meißner (geige.matze@gmail.com) */

#include "muh_nit.h"

MUH_NIT_CASE(multiplication_works)
{
    MUH_ASSERT("error: 1*1 != 1", 1 * 1 == 1);
    MUH_END_CASE();
}

MUH_NIT_CASE(addition_works)
{
    MUH_ASSERT("error: 1+1 != 2", 1 + 1 == 2);
    MUH_END_CASE();
}

int main(int argc, const char **args)
{
    muh_nit_case cases[] = MUH_CASES(
        &multiplication_works,
        &addition_works);

    muh_test_result res = muh_nit_run(cases);
    return muh_nit_evaluate(res);
}
