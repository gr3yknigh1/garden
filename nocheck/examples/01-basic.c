/*
 * FILE          modules/nocheck/examples/01-basic.c
 *
 * AUTHORS
 *               Ilya Akkuzin <gr3yknigh1@gmail.com>
 *
 * NOTICE        (c) Copyright 2024 by Ilya Akkuzin. All rights reserved.
 */
#include <nocheck/nocheck.h>

void
TC_Basic(NOC_TestCase *c)
{
    NOC_TASSERT(c, 1);

    NOC_TEXPECT(c, 69 % 2 != 0);
    NOC_TEXPECT(c, 2 == 69);
    NOC_TEXPECT(c, 1 != 0);

    NOC_TASSERT(c, 0);
    NOC_TASSERT(c, 1);
}

void
TC_Basic2(NOC_TestCase *c)
{
    NOC_TEXPECT(c, 1 == 1);
}

int
main(void)
{
    NOC_TestSuite *suite = NOC_TestSuiteMake("01-basic");
    NOC_TestSuiteAddCase(suite, "Basic", TC_Basic);
    NOC_TestSuiteAddCase(suite, "Basic2", TC_Basic2);

    int exitCode = NOC_TestSuiteExecute(suite);
    NOC_TestSuiteDestroy(suite);
    return exitCode;
}
