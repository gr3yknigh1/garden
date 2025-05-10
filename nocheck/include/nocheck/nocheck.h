#if !defined(NOCHECK_NOCHECK_H_INCLUDED)
/*
 * FILE          modules/nocheck/include/nocheck/nocheck.h
 *
 * AUTHORS
 *               Ilya Akkuzin <gr3yknigh1@gmail.com>
 *
 * NOTICE        (c) Copyright 2024 by Ilya Akkuzin. All rights reserved.
 */
#define NOCHECK_NOCHECK_H_INCLUDED

#if !defined(NOC_MALLOC)
#include <stdlib.h>
#define NOC_MALLOC(SZ) malloc((SZ))
#endif // NOC_MALLOC

#if !defined(NOC_STRINGIFY)
#define NOC_STRINGIFY(S) #S
#endif // NOC_STRINGIFY

#if !defined(NOC_LOG_INFO)
#include <stdio.h>
#define NOC_LOG_INFO(MSG, ...) printf("I: " MSG "\n", __VA_ARGS__)
#endif // NOC_LOG_INFO

#if !defined(NOC_LOG_ERROR)
#include <stdio.h>
#define NOC_LOG_ERROR(MSG, ...) printf("E: " MSG "\n", __VA_ARGS__)
#endif // NOC_LOG_ERROR

#if !defined(NOC_TRUE)
#define NOC_TRUE 1
#endif // NOC_TRUE

#if !defined(NOC_FALSE)
#define NOC_FALSE 0
#endif // NOC_FALSE

//
// @brief Allocates memory for specified type. Essensialy shorthand for NOC_MALLOC(sizeof(TYPE))
// @param TYPE Type memory for which should be allocated
// @see NOC_MALLOC
//
#if !defined(NOC_SALLOC)
#define NOC_SALLOC(TYPE) NOC_MALLOC(sizeof(TYPE))
#endif // NOC_SALLOC

#if !defined(NOC_EXIT_SUCCESS)
#define NOC_EXIT_SUCCESS 0
#endif // NOC_EXIT_SUCCESS

#if !defined(NOC_EXIT_FAILURE)
#define NOC_EXIT_FAILURE 1
#endif // NOC_EXIT_FAILURE

#if !defined(NOC_NULL)
#define NOC_NULL ((void *)0)
#endif // NOC_NULL

typedef struct NOC_TestCase NOC_TestCase; // Forward-declaring for `NOC_TestSuite`.

typedef void NOC_TestCaseProcType(NOC_TestCase *testCase);

typedef struct {
    const char *expression;
    const char *filePath;
    int lineNumber;
} NOC_TestCheckInfo;

typedef enum {
    NOC_TEST_CHECK_TYPE_ASSERT,
    NOC_TEST_CHECK_TYPE_EXPECT,
} NOC_TestCheckType;

typedef struct NOC_TestCheck {
    NOC_TestCheckInfo info;
    NOC_TestCheckType type;
    struct NOC_TestCheck *next;
} NOC_TestCheck;

typedef struct NOC_TestSuite {
    const char *name;
    NOC_TestCase *cases;
} NOC_TestSuite;

typedef enum { NOC_TEST_CASE_STATUS_OK, NOC_TEST_CASE_STATUS_FAILED } NOC_TestCaseStatus;

typedef struct NOC_TestCase {
    NOC_TestSuite *suite;
    const char *name;
    NOC_TestCaseProcType *proc;
    NOC_TestCaseStatus status;

    struct {
        NOC_TestCheck *head;
        NOC_TestCheck *tail;
    } checks;

    struct NOC_TestCase *next;
} NOC_TestCase;

void
NOC_TestCaseAddCheckFailure(
    NOC_TestCase *testCase, NOC_TestCheckType type, const char *expression, const char *filePath, int lineNumber)
{
    NOC_TestCheck *check = NOC_SALLOC(NOC_TestCheck);
    check->type = type;
    check->info.expression = expression;
    check->info.filePath = filePath;
    check->info.lineNumber = lineNumber;
    check->next = NOC_NULL;

    if (testCase->checks.tail == NOC_NULL) {
        testCase->checks.head = check;
        testCase->checks.tail = check;
    } else {
        testCase->checks.tail->next = check;
        testCase->checks.tail = check;
    }
}

NOC_TestSuite *
NOC_TestSuiteMake(const char *name)
{
    NOC_TestSuite *s = NOC_SALLOC(NOC_TestSuite);
    s->name = name;
    s->cases = NOC_NULL;
    return s;
}

NOC_TestCase *
NOC_TestSuiteAddCase(NOC_TestSuite *suite, const char *name, NOC_TestCaseProcType *proc)
{
    NOC_TestCase *newCase = NOC_SALLOC(NOC_TestCase);
    newCase->suite = suite;
    newCase->name = name;
    newCase->proc = proc;
    newCase->status = NOC_TEST_CASE_STATUS_OK;

    newCase->checks.head = NOC_NULL;
    newCase->checks.tail = NOC_NULL;

    newCase->next = NOC_NULL;

    if (suite->cases != 0) {
        NOC_TestCase *current = suite->cases;
        while (current->next != 0) {
            current = current->next;
        }
        current->next = newCase;
    } else {
        suite->cases = newCase;
    }

    return newCase;
}

int
NOC_TestSuiteExecute(NOC_TestSuite *suite)
{
    if (suite->cases == NOC_NULL) {
        NOC_LOG_INFO("SUITE('%s'): No cases was added", suite->name);
        return NOC_EXIT_SUCCESS;
    }
    int exitCode = NOC_EXIT_SUCCESS;

    NOC_TestCase *currentCase = suite->cases;

    while (currentCase != NOC_NULL) {
        currentCase->proc(currentCase);

        if (currentCase->status == NOC_TEST_CASE_STATUS_FAILED) {
            exitCode = NOC_EXIT_FAILURE;

            NOC_LOG_ERROR("CASE('%s'@'%s'): FAILED", suite->name, currentCase->name);

            NOC_TestCheck *currentCheck = currentCase->checks.head;

            while (currentCheck != NOC_NULL) {
                NOC_TestCheckInfo *checkInfo = &currentCheck->info;
                NOC_LOG_ERROR(
                    "    %s:%i '%s' - failed", checkInfo->filePath, checkInfo->lineNumber, checkInfo->expression);
                currentCheck = currentCheck->next;
            }
        } else if (currentCase->status == NOC_TEST_CASE_STATUS_OK) {
            NOC_LOG_INFO("CASE('%s'@'%s'): OK", suite->name, currentCase->name);
        } else {
            // NOC_DO_CRASH("Unreachable piece of code.");
        }

        currentCase = currentCase->next;
    }

    return exitCode;
}

void
NOC_TestSuiteDestroy(NOC_TestSuite *suite) // XXX
{
}

#define NOC_TASSERT(CASE, EXPRESSION) \
    do { \
        if (!(EXPRESSION)) { \
            (CASE)->status = NOC_TEST_CASE_STATUS_FAILED; \
            NOC_TestCaseAddCheckFailure( \
                (CASE), NOC_TEST_CHECK_TYPE_ASSERT, NOC_STRINGIFY(EXPRESSION), __FILE__, __LINE__); \
            return; \
        } \
    } while (0)

#define NOC_TEXPECT(CASE, EXPRESSION) \
    do { \
        if (!(EXPRESSION)) { \
            (CASE)->status = NOC_TEST_CASE_STATUS_FAILED; \
            NOC_TestCaseAddCheckFailure( \
                (CASE), NOC_TEST_CHECK_TYPE_EXPECT, NOC_STRINGIFY(EXPRESSION), __FILE__, __LINE__); \
        } else { \
            if ((CASE)->status != NOC_TEST_CASE_STATUS_FAILED) { \
                (CASE)->status = NOC_TEST_CASE_STATUS_OK; \
            } \
        } \
    } while (0)

#define NOC_TASSERT_EQ(CASE, A, B) NOC_TASSERT(CASE, (A) == (B))
#define NOC_TASSERT_GT(CASE, A, B) NOC_TASSERT(CASE, (A) > (B))
#define NOC_TASSERT_LT(CASE, A, B) NOC_TASSERT(CASE, (A) < (B))

#endif // NOCHECK_NOCHECK_H_INCLUDED
