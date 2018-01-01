#include "thread.h"

void init_mutex(Mutex *m)
{
    m->native = CreateMutex(NULL, FALSE, NULL);
}

void lock_mutex(Mutex *m)
{
    WaitForSingleObject(m->native, INFINITE);
}

void unlock_mutex(Mutex *m)
{
    ReleaseMutex(m->native);
}

