# cstr and muh_nit
Single header C libraries.

## muh_nit.h
Unit testing environment in a single header.

Example usage:
```C
#include "muh_nit.h"

// a basic test case
MUH_NIT_CASE(math_works)
{
  MUH_ASSERT("oh no, math is broken!", 2 + 2 == 4);
}

// multiple tests can be run with a table
MUH_NIT_FIXTURE(addition_table, TABLE(int, int, int),
                { 1, 1, 2 },
                { 2, 2, 4 } )
                
MUH_NIT_CASE(addition_test, FIXTURE(addition_table))
{
  MUH_FIXTURE_BIND(addition_table, a, b, res);
  MUH_ASSERT("how can addition not work?", a + b == res);
}

// tests can be disabled
MUH_NIT_CASE(broken_test, SKIP)
{
  MUH_FAIL("for some reason, this test always fails");
}

int main (int argc, char **argv)
{
  // register tests
  muh_nit_case cases[] = MUH_CASES(
    math_works,
    addition_test,
    broken_test);

  // process arguments
  muh_setup(argc, argv, cases);
  
  // run tests
  muh_nit_run(cases);
  
  // print summary / error messages and return error code
  return muh_nit_evaluate(cases);
}
```

## cstr.h
Single header string manipulation in C,
compatible with C++.

Run tests with make:
```
make test
```
