#ifndef LEARY_THREAD_H
#define LEARY_THREAD_H

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

#endif // LEARY_THREAD_H
