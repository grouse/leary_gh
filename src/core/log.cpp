/**
 * file:    log.cpp
 * created: 2018-09-06
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2018 - all rights reserved
 */

#define LOG_BUFFER_SIZE 2048

StringView string_from_log_type(LogType type)
{
    switch (type) {
    case LOG_TYPE_INFO:
        return "info";
    case LOG_TYPE_ERROR:
        return "error";
    case LOG_TYPE_WARNING:
        return "warning";
    default:
        return "";
    }
}

void log(
    const char *src_file,
    u32 src_line,
    const char *src_function,
    LogType type,
    const char *msg_format,
    ...)
{
    // TODO(jesper): src_file now contains the full absolute path. We should
    // format that a little bit better, at least for the console output - might
    // be useful to keep the full path, or at least longer path, in the file
    // logging when we get to that

    StringView type_str = string_from_log_type(type);

    // TODO(jesper): look into more performant and type safe alternatives to
    // vsnprintf
    va_list args;
    char message[LOG_BUFFER_SIZE];
    char buffer[LOG_BUFFER_SIZE];

    va_start(args, msg_format);
    i32 length = vsnprintf(message, LOG_BUFFER_SIZE, msg_format, args);
    va_end(args);
    ASSERT(length < LOG_BUFFER_SIZE);

    length = snprintf(
        buffer,
        LOG_BUFFER_SIZE,
        "%s:%d: %s: [%s] %s\n",
        src_file,
        src_line,
        type_str.bytes,
        src_function,
        message);
    ASSERT(length < LOG_BUFFER_SIZE);

    // TODO(jesper): output to log file
    platform_output_debug_string(buffer);
}

void log(
    const char *src_file,
    u32 src_line,
    const char *src_function,
    const char *msg_format,
    ...)
{
    // TODO(jesper): src_file now contains the full absolute path. We should
    // format that a little bit better, at least for the console output - might
    // be useful to keep the full path, or at least longer path, in the file
    // logging when we get to that

    StringView type_str = string_from_log_type(LOG_TYPE_DEFAULT);

    // TODO(jesper): look into more performant and type safe alternatives to
    // vsnprintf
    va_list args;
    char message[LOG_BUFFER_SIZE];
    char buffer[LOG_BUFFER_SIZE];

    va_start(args, msg_format);
    i32 length = vsnprintf(message, LOG_BUFFER_SIZE, msg_format, args);
    va_end(args);
    ASSERT(length < LOG_BUFFER_SIZE);

    length = snprintf(
        buffer,
        LOG_BUFFER_SIZE,
        "%s:%d: %s: [%s] %s\n",
        src_file,
        src_line,
        type_str.bytes,
        src_function,
        message);
    ASSERT(length < LOG_BUFFER_SIZE);

    // TODO(jesper): output to log file
    platform_output_debug_string(buffer);
}

