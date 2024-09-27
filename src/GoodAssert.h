#pragma once

#include <cstdarg>
#include <ctime>

namespace Assert
{
    void ReportFailure(const char * condition, const char * file, int line, const char * msg, ...);
}

#define GOOD_ASSERT_ENABLE

#ifdef GOOD_ASSERT_ENABLE

    #define GOOD_ASSERT(cond, msg, ...) \
        do \
        { \
            if (!(cond)) \
            { \
                Assert::ReportFailure(#cond, __FILE__, __LINE__, (msg), ##__VA_ARGS__); \
            } \
        } while(0)

#else
    #define GOOD_ASSERT(cond, msg, ...) 
#endif
