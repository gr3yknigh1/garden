# nocheck

> Unit-testing library.

## Example

Simple example:

```c
//
// basic.c
//
#include <nocheck/nocheck.h>

void
TC_Basic(NOC_TestCase *c)
{
    int x = 69;

    NOC_TASSERT(c, x > 0);

    NOC_TEXPECT(c, x - 1 > 2);
    NOC_TEXPECT(c, x < 0);
    NOC_TEXPECT(c, x != 69);
}

int
main(void)
{
    NOC_TestSuite *suite = NOC_TestSuiteMake("basic");
    NOC_TestSuiteAddCase(suite, "Basic", TC_Basic);

    int exitCode = NOC_TestSuiteExecute(suite);
    NOC_TestSuiteDestroy(suite);
    return exitCode;
}
```

Output:

```
E: CASE('basic'@'Basic'): FAILED
E:     path/to/your/tests/basic.c:11 'x < 0' - failed
E:     path/to/your/tests/basic.c:12 'x != 69' - failed
```

Check `example/` folder for more.
