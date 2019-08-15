/**
 * file:    log.h
 * created: 2018-09-06
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2018 - all rights reserved
 */

enum LogType {
    LOG_TYPE_INFO,
    LOG_TYPE_WARNING,
    LOG_TYPE_ERROR,

    LOG_TYPE_UNIMPLEMENTED,
    LOG_TYPE_ASSERT,

    LOG_TYPE_DEFAULT = LOG_TYPE_INFO
};

#if LEARY_ENABLE_LOGGING

#define LOG(...)            log(__FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

#define LOG_WARNING(...)    log(__FILE__, __LINE__, __FUNCTION__, LOG_TYPE_WARNING, __VA_ARGS__)
#define LOG_ERROR(...)      log(__FILE__, __LINE__, __FUNCTION__, LOG_TYPE_ERROR, __VA_ARGS__)
#define LOG_INFO(...)       log(__FILE__, __LINE__, __FUNCTION__, LOG_TYPE_INFO, __VA_ARGS__)
#define LOG_ASSERT(...)     log(__FILE__, __LINE__, __FUNCTION__, LOG_TYPE_ASSERT, __VA_ARGS__)

// TODO(jesper): support arguments
#define LOG_UNIMPLEMENTED() log(__FILE__, __LINE__, __FUNCTION__, LOG_TYPE_UNIMPLEMENTED, "fixme: stub!")

#else

#define LOG(...)            do {} while(0)
#define LOG_WARNING(...)    do {} while(0)
#define LOG_ERROR(...)      do {} while(0)
#define LOG_INFO(...)       do {} while(0)
#define LOG_UNIMPLEMENTED() do {} while(0)
#define LOG_ASSERT(...)     do {} while(0)

#endif // LEARY_ENABLE_LOGGING

void log(
    const char *src_file,
    u32 src_line,
    const char *src_function,
    LogType type,
    const char *msg_format,
    ...);

void log(
    const char *src_file,
    u32 src_line,
    const char *src_function,
    const char *msg_format,
    ...);
