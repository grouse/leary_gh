/**
 * file:    linux_leary.cpp
 * created: 2017-03-18
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#include "core/types.h"

#include "platform.h"

#include "platform/linux_debug.cpp"
#include "platform/linux_file.cpp"
#include "platform/linux_input.cpp"

#include <sys/inotify.h>
#include <pthread.h>

PlatformState   *g_platform;
Settings        g_settings;
HeapAllocator   *g_heap;
LinearAllocator *g_frame;
LinearAllocator *g_debug_frame;
LinearAllocator *g_persistent;
StackAllocator  *g_stack;

#include "platform/linux_vulkan.cpp"

#include "leary.cpp"

#include "generated/type_info.h"

void init_mutex(Mutex *m)
{
    (void)m;
}

void lock_mutex(Mutex *m)
{
    pthread_mutex_lock(&m->native);
}

void unlock_mutex(Mutex *m)
{
    pthread_mutex_unlock(&m->native);
}

void platform_toggle_raw_mouse()
{
    Display *dpy = g_platform->native.display;
    Window  &wnd = g_platform->native.window;

    g_platform->raw_mouse = !g_platform->raw_mouse;
    DEBUG_LOG("raw mouse mode set to: %d", g_platform->raw_mouse);

    if (g_platform->raw_mouse) {
        XGrabPointer(dpy, wnd, false,
                     (KeyPressMask | KeyReleaseMask) & 0,
                     GrabModeAsync, GrabModeAsync,
                     wnd, g_platform->native.hidden_cursor, CurrentTime);
    } else {
        XUngrabPointer(dpy, CurrentTime);

        i32 num_screens = XScreenCount(dpy);

        for (i32 i = 0; i < num_screens; i++) {
            Window root = XRootWindow(dpy, i);
            Window child;

            u32 mask;
            i32 root_x, root_y, win_x, win_y;
            bool result = XQueryPointer(dpy, wnd,
                                        &root, &child,
                                        &root_x, &root_y, &win_x, &win_y,
                                        &mask);
            if (result) {
                g_platform->native.mouse.x = win_x;
                g_platform->native.mouse.y = win_y;
                break;
            }
        }
    }

    XFlush(dpy);
}

void platform_set_raw_mouse(bool enable)
{
    if ((enable && !g_platform->raw_mouse) ||
        (!enable && g_platform->raw_mouse))
    {
        platform_toggle_raw_mouse();
    }
}

void platform_quit()
{
    XUngrabPointer(g_platform->native.display, CurrentTime);

    char *settings_path = resolve_path(GamePath_preferences, "settings.conf", g_stack);
    serialize_save_conf(settings_path, Settings_members,
                        ARRAY_SIZE(Settings_members), &g_settings);

    // TODO(jesper): do we need to unload the .so ?
    exit(EXIT_SUCCESS);
}

#define INOTIFY_EVENT_SIZE (sizeof(struct inotify_event))
#define INOTIFY_BUF_SIZE   (1024 * (INOTIFY_EVENT_SIZE + 16))

struct CatalogThreadData {
    const char         *folder;
    catalog_callback_t *callback;
};

void* catalog_thread_process(void *data)
{
    // TODO(jesper): entering the realm of threads, we need thread safe
    // DEBUG_LOG now!!!!
    CatalogThreadData *ctd = (CatalogThreadData*)data;

    char buffer[INOTIFY_BUF_SIZE];

    int fd = inotify_init();
    assert(fd >= 0);

    isize flen = strlen(ctd->folder);

    // NOTE(jesper): we're using IN_MOVED_FROM here because it seems when krita
    // exports its images it writes it to a temporary file and then moves it.
    // This is really quite weird, and I might contact krita about it.
    int wd = inotify_add_watch(fd, ctd->folder, IN_CLOSE_WRITE | IN_MOVED_FROM);
    while (true) {
        int length = read(fd, buffer, INOTIFY_BUF_SIZE);

        assert(length >= 0);
        int i = 0;
        while (i < length) {
            struct inotify_event *event = (struct inotify_event*)&buffer[i];
            if (event->len) {
                // TODO(jesper): supported created and deleted file events
                if (!(event->mask & IN_ISDIR)) {
                    Path p;
                    bool eslash  = ctd->folder[flen-1] == '/';
                    if (!eslash) {
                        isize length = flen + event->len + 2;
                        // TODO(jesper): replace with thread safe allocator
                        p.absolute = { length, (char*)malloc(length) };
                        strcpy(p.absolute.bytes, ctd->folder);
                        p.absolute[flen]   = '/';
                        p.absolute[flen+1] = '\0';
                    } else {
                        isize length = flen + event->len + 1;
                        // TODO(jesper): replace with thread safe allocator
                        p.absolute   = { length, (char*)malloc(length) };
                        strcpy(p.absolute.bytes, ctd->folder);
                    }

                    strcat(p.absolute.bytes, event->name);
                    if (!eslash) {
                        p.filename = { event->len, p.absolute.bytes + flen + 1 };
                    } else {
                        p.filename = { event->len, p.absolute.bytes + flen };
                    }

                    isize ext = 0;
                    for (i32 i = 0; i < p.filename.length; i++) {
                        if (p.filename[i] == '.') {
                            ext = i;
                        }
                    }
                    if (ext != 0) {
                        p.extension = { event->len - ext, p.filename.bytes + ext + 1 };
                    } else {
                        p.extension = { 0, p.filename.bytes + event->len };
                    }

                    ctd->callback(p);
                }
            }

            i += INOTIFY_EVENT_SIZE + event->len;
        }
    }

    inotify_rm_watch( fd, wd );
    close( fd );

    return nullptr;
}

void create_catalog_thread(const char *folder, catalog_callback_t *callback)
{
    auto data      = g_persistent->talloc<CatalogThreadData>();
    data->folder   = folder;
    data->callback = callback;

    pthread_t *thread = g_heap->talloc<pthread_t>();
    int result = pthread_create(thread, NULL, &catalog_thread_process, data);
    assert(result == 0);
}

DL_EXPORT
PLATFORM_INIT_FUNC(platform_init)
{
    g_platform = platform;
    g_settings = {};

    NativePlatformState *native = &g_platform->native;

    isize frame_size       = 64  * 1024 * 1024;
    isize debug_frame_size = 64  * 1024 * 1024;
    isize persistent_size  = 256 * 1024 * 1024;
    isize heap_size        = 256 * 1024 * 1024;
    isize stack_size       = 16  * 1024 * 1024;

    // TODO(jesper): allocate these using appropriate syscalls
    void *frame_mem       = malloc(frame_size);
    void *debug_frame_mem = malloc(debug_frame_size);
    void *persistent_mem  = malloc(persistent_size);
    void *heap_mem        = malloc(heap_size);
    void *stack_mem       = malloc(stack_size);

    g_heap        = new HeapAllocator  (heap_mem,        heap_size);
    g_frame       = new LinearAllocator(frame_mem,       frame_size);
    g_debug_frame = new LinearAllocator(debug_frame_mem, debug_frame_size);
    g_persistent  = new LinearAllocator(persistent_mem,  persistent_size);
    g_stack       = new StackAllocator (stack_mem,       stack_size);

    init_paths(g_persistent);

    char *settings_path = resolve_path(GamePath_preferences, "settings.conf", g_frame);
    serialize_load_conf(settings_path, Settings_members,
                        ARRAY_SIZE(Settings_members), &g_settings);

    native->display = XOpenDisplay(nullptr);
    i32 screen      = DefaultScreen(native->display);
    native->window  = XCreateSimpleWindow(native->display,
                                          DefaultRootWindow(native->display),
                                          0, 0,
                                          g_settings.video.resolution.width,
                                          g_settings.video.resolution.height,
                                          2, BlackPixel(native->display, screen),
                                          BlackPixel(native->display, screen));

    XSelectInput(native->display, native->window,
                 KeyPressMask | KeyReleaseMask | StructureNotifyMask |
                 PointerMotionMask | ButtonPressMask |
                 ButtonReleaseMask | EnterWindowMask);
    XMapWindow(native->display, native->window);

    native->WM_DELETE_WINDOW = XInternAtom(native->display, "WM_DELETE_WINDOW", false);
    XSetWMProtocols(native->display, native->window, &native->WM_DELETE_WINDOW, 1);

    i32 xkb_major = XkbMajorVersion;
    i32 xkb_minor = XkbMinorVersion;

    if (XkbQueryExtension(native->display, NULL, NULL, NULL, &xkb_major, &xkb_minor)) {
        native->xkb = XkbGetMap(native->display, XkbAllClientInfoMask, XkbUseCoreKbd);
        DEBUG_LOG("Initialised XKB version %d.%d", xkb_major, xkb_minor);
    }

    {
        i32 event, error;
        if (!XQueryExtension(native->display, "XInputExtension",
                             &native->xinput2,
                             &event, &error))
        {
            assert(false && "XInput2 extension not found, this is required");
            platform_quit();
        }

        u8 mask[3] = { 0, 0, 0 };

        XIEventMask emask;
        emask.deviceid = XIAllMasterDevices;
        emask.mask_len = sizeof(mask);
        emask.mask     = mask;

        XISetMask(mask, XI_RawMotion);
        //XISetMask(mask, XI_RawButtonPress);
        //XISetMask(mask, XI_RawButtonRelease);

        XISelectEvents(native->display, DefaultRootWindow(native->display),
                       &emask, 1);
        XFlush(native->display);
    }

    { // create hidden cursor used when raw mouse is enabled
        XColor xcolor;
        char csr_bits[] = { 0x00 };

        Pixmap csr = XCreateBitmapFromData(native->display, native->window,
                                           csr_bits, 1, 1);
        native->hidden_cursor = XCreatePixmapCursor(native->display,
                                                   csr, csr, &xcolor, &xcolor,
                                                   1, 1);
    }

    game_init();

    i32 num_screens = XScreenCount(native->display);
    for (i32 i = 0; i < num_screens; i++) {
        Window root = XRootWindow(native->display, i);
        Window child;

        u32 mask;
        i32 root_x, root_y, win_x, win_y;
        bool result = XQueryPointer(native->display, native->window,
                                    &root, &child,
                                    &root_x, &root_y,
                                    &win_x, &win_y,
                                    &mask);
        if (result) {
            native->mouse.x = win_x;
            native->mouse.y = win_y;
            break;
        }
    }
}

DL_EXPORT
PLATFORM_PRE_RELOAD_FUNC(platform_pre_reload)
{
    platform->reload_state.game = game_pre_reload();

    platform->reload_state.frame       = g_frame;
    platform->reload_state.debug_frame = g_debug_frame;
    platform->reload_state.stack       = g_stack;
    platform->reload_state.heap        = g_heap;
    platform->reload_state.persistent  = g_persistent;
    platform->reload_state.stack       = g_stack;
}

DL_EXPORT
PLATFORM_RELOAD_FUNC(platform_reload)
{
    g_frame       = platform->reload_state.frame;
    g_debug_frame = platform->reload_state.debug_frame;
    g_heap        = platform->reload_state.heap;
    g_persistent  = platform->reload_state.persistent;
    g_stack       = platform->reload_state.stack;
    g_platform    = platform;

    game_reload(platform->reload_state.game);
}

DL_EXPORT
PLATFORM_UPDATE_FUNC(platform_update)
{
    profile_start_frame();

    NativePlatformState *native = &g_platform->native;

    PROFILE_START(linux_input);
    XEvent xevent;
    while (XPending(native->display) > 0) {
        XNextEvent(native->display, &xevent);

        switch (xevent.type) {
        case KeyPress: {
            InputEvent event;
            event.type         = InputType_key_press;
            event.key.code     = linux_keycode(xevent.xkey.keycode);
            event.key.repeated = false;

            game_input(event);
        } break;
        case KeyRelease: {
            InputEvent event;
            event.type         = InputType_key_release;
            event.key.code     = linux_keycode(xevent.xkey.keycode);
            event.key.repeated = false;

            if (XEventsQueued(native->display, QueuedAfterReading)) {
                XEvent next;
                XPeekEvent(native->display, &next);

                if (next.type == KeyPress &&
                    next.xkey.time == xevent.xkey.time &&
                    next.xkey.keycode == xevent.xkey.keycode)
                {
                    event.key.repeated = true;
                }
            }

            game_input(event);
        } break;
        case ButtonPress: {
            InputEvent event;
            event.type         = InputType_mouse_press;
            event.mouse.x      = xevent.xbutton.x;
            event.mouse.y      = xevent.xbutton.y;
            event.mouse.button = xevent.xbutton.button;
            game_input(event);
        } break;
        case MotionNotify: {
#if 0
            if (platform->raw_mouse) {
                break;
            }

            InputEvent event;
            event.type = InputType_mouse_move;
            event.mouse.x = xevent.xmotion.x;
            event.mouse.y = xevent.xmotion.y;

            event.mouse.dx = xevent.xmotion.x - native->mouse.x;
            event.mouse.dy = xevent.xmotion.y - native->mouse.y;

            native->mouse.x = xevent.xmotion.x;
            native->mouse.y = xevent.xmotion.y;

            game_input(platform, event);
#endif
        } break;
        case EnterNotify: {
            if (xevent.xcrossing.focus == true &&
                xevent.xcrossing.window == native->window &&
                xevent.xcrossing.display == native->display)
            {
                native->mouse.x = xevent.xcrossing.x;
                native->mouse.y = xevent.xcrossing.y;
            }
        } break;
        case ClientMessage: {
            if ((Atom)xevent.xclient.data.l[0] == native->WM_DELETE_WINDOW) {
                game_quit();
            }
        } break;
        case GenericEvent: {
            if (xevent.xcookie.extension == native->xinput2 &&
                XGetEventData(native->display, &xevent.xcookie))
            {
                switch (xevent.xcookie.evtype) {
                case XI_RawMotion: {
                    if (!platform->raw_mouse) {
                        break;
                    }

                    static Time prev_time = 0;
                    static f64  prev_deltas[2];

                    XIRawEvent *revent = (XIRawEvent*)xevent.xcookie.data;

                    f64 deltas[2];

                    u8  *mask    = revent->valuators.mask;
                    i32 mask_len = revent->valuators.mask_len;
                    f64 *ivalues = revent->raw_values;

                    i32 top = MIN(mask_len * 8, 16);
                    for (i32 i = 0, j = 0; i < top && j < 2; i++,j++) {
                        if (XIMaskIsSet(mask, i)) {
                            deltas[j] = *ivalues++;
                        }
                    }

                    if (revent->time == prev_time &&
                        deltas[0] == prev_deltas[0] &&
                        deltas[1] == prev_deltas[1])
                    {
                        // NOTE(jesper): discard duplicate events,
                        // apparently can happen?
                        break;
                    }

                    prev_deltas[0] = deltas[0];
                    prev_deltas[1] = deltas[1];

                    InputEvent event = {};
                    event.type     = InputType_mouse_move;
                    event.mouse.dx = deltas[0];
                    event.mouse.dy = deltas[1];

                    game_input(event);
                } break;
                default:
                    DEBUG_LOG("unhandled xinput2 event: %d",
                              xevent.xcookie.evtype);
                    break;
                }
            } else {
                DEBUG_LOG("unhandled generic event");
            }
        } break;
        default: {
            DEBUG_LOG("unhandled xevent type: %d", xevent.type);
        } break;
        }
    }
    PROFILE_END(linux_input);

    game_update_and_render(dt);
    profile_end_frame();
}
