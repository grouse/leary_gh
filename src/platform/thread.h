/**
 * file:    thread.h
 * created: 2018-01-02
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2018 - all rights reserved
 */

#ifndef LEARY_PLATFORM_THREAD_H
#define LEARY_PLATFORM_THREAD_H

#if defined(__linux__)
typedef pthread_mutex_t NativeMutex;
#elif defined(_WIN32)
typedef HANDLE NativeMutex;
#else
#error "unsupported platform"
#endif

struct Mutex {
    NativeMutex native;
};

// -- functions
void init_mutex(Mutex *m);
void lock_mutex(Mutex *m);
void unlock_mutex(Mutex *m);

#endif // LEARY_PLATFORM_THREAD_H
