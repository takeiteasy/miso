//
//  log.h
//  colony
//
//  Created by George Watson on 19/02/2023.
//

#ifndef log_h
#define log_h
#include <assert.h>

#if defined(DEBUG)
#define LOG(FMT, ...) fprintf(stdout, "[LOG] " FMT "\n", __VA_ARGS__);
#else
#define LOG(...)
#endif

#define ASSERT(CND, FMT, ...)                                  \
    do {                                                       \
        if (!(CND)) {                                          \
            fprintf(stderr, __BASE_FILE__ ":%2d\n", __LINE__); \
            fprintf(stderr, "Assertion `%s` failed\n", #CND);  \
            fprintf(stderr, "\t" FMT "\n", ##__VA_ARGS__);     \
            abort();                                           \
        }                                                      \
    } while(0)

#endif /* log_h */
