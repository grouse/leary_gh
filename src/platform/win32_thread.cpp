/**
 * file:    thread.cpp
 * created: 2018-01-02
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2018 - all rights reserved
 */

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

