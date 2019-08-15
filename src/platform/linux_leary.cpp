/**
 * file:    linux_leary.cpp
 * created: 2017-03-18
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017-2018 - all rights reserved
 */

#define SND_PCM_OPEN(name) int name(snd_pcm_t **pcm, const char *name, snd_pcm_stream_t stream, int mode)
typedef SND_PCM_OPEN(snd_pcm_open_fptr);

#define SND_PCM_HW_PARAMS_MALLOC(name) int name(snd_pcm_hw_params_t **ptr)
typedef SND_PCM_HW_PARAMS_MALLOC(snd_pcm_hw_params_malloc_fptr);

#define SND_PCM_HW_PARAMS_ANY(name) int name(snd_pcm_t *pcm, snd_pcm_hw_params_t *params)
typedef SND_PCM_HW_PARAMS_ANY(snd_pcm_hw_params_any_fptr);

#define SND_PCM_HW_PARAMS_SET_ACCESS(name) int name(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_access_t _access)
typedef SND_PCM_HW_PARAMS_SET_ACCESS(snd_pcm_hw_params_set_access_fptr);

#define SND_PCM_HW_PARAMS_SET_FORMAT(name) int name(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_format_t val)
typedef SND_PCM_HW_PARAMS_SET_FORMAT(snd_pcm_hw_params_set_format_fptr);

#define SND_PCM_HW_PARAMS_SET_RATE_NEAR(name) int name(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir)
typedef SND_PCM_HW_PARAMS_SET_RATE_NEAR(snd_pcm_hw_params_set_rate_near_fptr);

#define SND_PCM_HW_PARAMS_SET_RATE(name) int name(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val, int dir)
typedef SND_PCM_HW_PARAMS_SET_RATE(snd_pcm_hw_params_set_rate_fptr);

#define SND_PCM_HW_PARAMS_SET_CHANNELS(name) int name(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val)
typedef SND_PCM_HW_PARAMS_SET_CHANNELS(snd_pcm_hw_params_set_channels_fptr);

#define SND_PCM_HW_PARAMS(name) int name(snd_pcm_t *pcm, snd_pcm_hw_params_t *params)
typedef SND_PCM_HW_PARAMS(snd_pcm_hw_params_fptr);

#define SND_PCM_HW_PARAMS_FREE(name) void name(snd_pcm_hw_params_t *obj)
typedef SND_PCM_HW_PARAMS_FREE(snd_pcm_hw_params_free_fptr);

#define SND_PCM_HW_PARAMS_GET_PERIOD_SIZE(name) int name(const snd_pcm_hw_params_t *params, snd_pcm_uframes_t *frames, int *dir)
typedef SND_PCM_HW_PARAMS_GET_PERIOD_SIZE(snd_pcm_hw_params_get_period_size_fptr);

#define SND_PCM_PREPARE(name) int name(snd_pcm_t *pcm);
typedef SND_PCM_PREPARE(snd_pcm_prepare_fptr);

#define SND_PCM_WRITEI(name) snd_pcm_sframes_t name(snd_pcm_t *pcm, const void *buffer, snd_pcm_uframes_t size)
typedef SND_PCM_WRITEI(snd_pcm_writei_fptr);

#define SND_PCM_HW_PARAMS_SET_BUFFER_SIZE(name) int name(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_uframes_t val)
typedef SND_PCM_HW_PARAMS_SET_BUFFER_SIZE(snd_pcm_hw_params_set_buffer_size_fptr);

#define SND_PCM_HW_PARAMS_SET_PERIODS(name) int name(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val, int dir)
typedef SND_PCM_HW_PARAMS_SET_PERIODS(snd_pcm_hw_params_set_periods_fptr);

PlatformState   *g_platform;
Settings         g_settings;

struct MouseState
{
    Vector2 position = {};
    Vector2 drag_start = {};
    bool pressed = false;
    bool raw_mouse = false;
};

MouseState g_mouse = {};

Allocator *g_heap;
Allocator *g_frame;
Allocator *g_debug_frame;
Allocator *g_persistent;
Allocator *g_stack;
Allocator *g_system_alloc;

void init_mutex(Mutex *m)
{
    m->native = {};
    pthread_mutex_init(&m->native, nullptr);
}

void lock_mutex(Mutex *m)
{
    pthread_mutex_lock(&m->native);
}

void unlock_mutex(Mutex *m)
{
    pthread_mutex_unlock(&m->native);
}


snd_pcm_t *g_alsa_pcm = nullptr;
void *g_alsa_buffer = nullptr;

void init_alsa()
{
    u32 sample_rate = 48000;
    u32 buffer_duration_sec = 2;
    u32 num_channels = 2;
    u32 num_samples = sample_rate * num_channels * buffer_duration_sec;
    u32 sample_size = sizeof(i16) * 2;
    u32 buffer_size = num_samples * sample_size;

    snd_pcm_uframes_t period_size;

    void *lib = dlopen("libasound.so", RTLD_NOW | RTLD_GLOBAL);
    if (lib == nullptr) {
        ASSERT(false);
        return;
    }

    auto snd_pcm_open = (snd_pcm_open_fptr*)dlsym(lib, "snd_pcm_open");
    auto snd_pcm_prepare = (snd_pcm_prepare_fptr*)dlsym(lib, "snd_pcm_prepare");
    auto snd_pcm_writei = (snd_pcm_writei_fptr*)dlsym(lib, "snd_pcm_writei");
    auto snd_pcm_hw_params_malloc = (snd_pcm_hw_params_malloc_fptr*)dlsym(lib, "snd_pcm_hw_params_malloc");
    auto snd_pcm_hw_params_any = (snd_pcm_hw_params_any_fptr*)dlsym(lib, "snd_pcm_hw_params_any");
    auto snd_pcm_hw_params_set_access = (snd_pcm_hw_params_set_access_fptr*)dlsym(lib, "snd_pcm_hw_params_set_access");
    auto snd_pcm_hw_params_set_format = (snd_pcm_hw_params_set_format_fptr*)dlsym(lib, "snd_pcm_hw_params_set_format");
    auto snd_pcm_hw_params_set_rate_near = (snd_pcm_hw_params_set_rate_near_fptr*)dlsym(lib, "snd_pcm_hw_params_set_rate_near");
    auto snd_pcm_hw_params_set_rate = (snd_pcm_hw_params_set_rate_fptr*)dlsym(lib, "snd_pcm_hw_params_set_rate");
    auto snd_pcm_hw_params_set_channels = (snd_pcm_hw_params_set_channels_fptr*)dlsym(lib, "snd_pcm_hw_params_set_channels");
    auto snd_pcm_hw_params = (snd_pcm_hw_params_fptr*)dlsym(lib, "snd_pcm_hw_params");
    auto snd_pcm_hw_params_free = (snd_pcm_hw_params_free_fptr*)dlsym(lib, "snd_pcm_hw_params_free");
    auto snd_pcm_hw_params_set_buffer_size = (snd_pcm_hw_params_set_buffer_size_fptr*)dlsym(lib, "snd_pcm_hw_params_set_buffer_size");
    auto snd_pcm_hw_params_set_periods = (snd_pcm_hw_params_set_periods_fptr*)dlsym(lib, "snd_pcm_hw_params_set_periods");
    auto snd_pcm_hw_params_get_period_size = (snd_pcm_hw_params_get_period_size_fptr*)dlsym(lib, "snd_pcm_hw_params_get_period_size");

    ASSERT(snd_pcm_open != nullptr);
    ASSERT(snd_pcm_prepare != nullptr);
    ASSERT(snd_pcm_writei != nullptr);
    ASSERT(snd_pcm_hw_params_malloc != nullptr);
    ASSERT(snd_pcm_hw_params_any != nullptr);
    ASSERT(snd_pcm_hw_params_set_access != nullptr);
    ASSERT(snd_pcm_hw_params_set_format != nullptr);
    ASSERT(snd_pcm_hw_params_set_rate_near != nullptr);
    ASSERT(snd_pcm_hw_params_set_rate != nullptr);
    ASSERT(snd_pcm_hw_params_set_channels != nullptr);
    ASSERT(snd_pcm_hw_params != nullptr);
    ASSERT(snd_pcm_hw_params_free != nullptr);
    ASSERT(snd_pcm_hw_params_set_buffer_size != nullptr);
    ASSERT(snd_pcm_hw_params_set_periods != nullptr);
    ASSERT(snd_pcm_hw_params_get_period_size != nullptr);

    int err;
    snd_pcm_hw_params_t *hw_params = nullptr;

    err = snd_pcm_open(&g_alsa_pcm, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) {
        ASSERT(false);
        return;
    }

    err = snd_pcm_hw_params_malloc(&hw_params);
    if (err < 0) {
        ASSERT(false);
        return;
    }

    err = snd_pcm_hw_params_any(g_alsa_pcm, hw_params);
    if (err < 0) {
        ASSERT(false);
        return;
    }

    err = snd_pcm_hw_params_set_access(g_alsa_pcm, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0) {
        ASSERT(false);
        return;
    }

    err = snd_pcm_hw_params_set_format(g_alsa_pcm, hw_params, SND_PCM_FORMAT_S16_LE);
    if (err < 0) {
        ASSERT(false);
        return;
    }

    err = snd_pcm_hw_params_set_rate(g_alsa_pcm, hw_params, sample_rate, 0);
    if (err < 0) {
        ASSERT(false);
        return;
    }

    err = snd_pcm_hw_params_set_channels(g_alsa_pcm, hw_params, num_channels);
    if (err < 0) {
        ASSERT(false);
        return;
    }

#if 0
    err = snd_pcm_hw_params_set_periods(g_alsa_pcm, hw_params, num_samples, 0);
    if (err < 0) {
        ASSERT(false);
        return;
    }
#endif

#if 0
    err = snd_pcm_hw_params_get_period_size(hw_params, &period_size, 0);
    if (err < 0) {
        ASSERT(false);
        return;
    }
#endif

#if 0
    err = snd_pcm_hw_params_set_buffer_size(g_alsa_pcm, hw_params, num_samples);
    if (err < 0) {
        ASSERT(false);
        return;
    }
#endif

    err = snd_pcm_hw_params(g_alsa_pcm, hw_params);
    if (err < 0) {
        ASSERT(false);
        return;
    }

    snd_pcm_hw_params_free(hw_params);

    err = snd_pcm_prepare(g_alsa_pcm);
    if (err < 0) {
        ASSERT(false);
        return;
    }

    g_alsa_buffer = malloc(buffer_size);
    struct Sample {
        i16 left;
        i16 right;
    };

    Sample *samples = (Sample*)g_alsa_buffer;
    i32 counter = 0;

    i32 hz = 261;
    i32 period = sample_rate/ hz;
    i16 value = 2000;

    for (u32 i = 0; i < num_samples; i++) {
        if (counter > period) {
            value = -value;
            counter = 0;
        }
        counter++;

        samples[i].left  = value;
        samples[i].right = value;
    }

    snd_pcm_writei(g_alsa_pcm, g_alsa_buffer, num_samples);
}

void platform_toggle_raw_mouse()
{
    Display *dpy = g_platform->native.display;
    Window  &wnd = g_platform->native.window;

    g_mouse.raw_mouse = !g_mouse.raw_mouse;
    LOG("raw mouse mode set to: %d", g_mouse.raw_mouse);

    if (g_mouse.raw_mouse) {
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
                g_mouse.position = { (f32)win_x, (f32)win_y };
                break;
            }
        }
    }

    XFlush(dpy);
}

void platform_set_raw_mouse(bool enable)
{
    if ((enable && !g_mouse.raw_mouse) ||
        (!enable && g_mouse.raw_mouse))
    {
        platform_toggle_raw_mouse();
    }
}

Vector2 get_mouse_position()
{
    return g_mouse.position;
}

void platform_quit()
{
    XUngrabPointer(g_platform->native.display, CurrentTime);

    FilePath settings_path = resolve_file_path(
        GamePath_preferences,
        "settings.conf",
        g_stack);;

    serialize_save_conf(
        settings_path,
        Settings_members,
        ARRAY_SIZE(Settings_members),
        &g_settings);

    // TODO(jesper): do we need to unload the .so ?
    exit(EXIT_SUCCESS);
}

#define INOTIFY_EVENT_SIZE (sizeof(struct inotify_event))
#define INOTIFY_BUF_SIZE   (1024 * (INOTIFY_EVENT_SIZE + 16))

struct CatalogThreadData {
    Array<FolderPath> folders;
    catalog_callback_t *callback;
};

void* catalog_thread_process(void *data)
{
    // TODO(jesper): entering the realm of threads, we need thread safe
    // LOG now!!!!
    CatalogThreadData *ctd = (CatalogThreadData*)data;

    char buffer[INOTIFY_BUF_SIZE];

    int fd = inotify_init();
    assert(fd >= 0);


    // TODO(jesper): surely there's a better way than to use a hashmap for this?
    RHHashMap<i32, FolderPath> watches;
    init_map(&watches, g_heap);

    for (i32 i = 0; i < ctd->folders.count; i++) {
        // NOTE(jesper): we're using IN_MOVED_FROM here because it seems when krita
        // exports its images it writes it to a temporary file and then moves it.
        // This is really quite weird, and I might contact krita about it.
        int wd = inotify_add_watch(
            fd,
            ctd->folders[i].absolute.bytes,
            IN_CLOSE_WRITE | IN_MOVED_FROM);

        map_add(&watches, wd, ctd->folders[i]);
    }

    while (true) {
        int length = read(fd, buffer, INOTIFY_BUF_SIZE);

        assert(length >= 0);
        int i = 0;
        while (i < length) {
            struct inotify_event *event = (struct inotify_event*)&buffer[i];
            if (event->len) {
                // TODO(jesper): supported created and deleted file events
                if (!(event->mask & IN_ISDIR)) {

                    FolderPath *ret = map_find(&watches, event->wd);
                    if (ret == nullptr) {
                        LOG(Log_error,
                            "unable to find folder for watch descriptor: %d",
                            event->wd);
                        continue;
                    }

                    FolderPath folder = *ret;

                    bool eslash = folder[folder.absolute.size-1] == '/';

                    FilePath p = eslash
                        ? create_file_path(g_system_alloc, folder, event->name)
                        : create_file_path(g_system_alloc, folder, '/', event->name);

                    ctd->callback(p);
                }
            }

            i += INOTIFY_EVENT_SIZE + event->len;
        }
    }

#if 0
    // TODO(jesper): iterate over the hash table and remove the watches
    inotify_rm_watch(fd, wd );
#endif
    close(fd);

    return nullptr;
}

void create_catalog_thread(Array<FolderPath> folders, catalog_callback_t *callback)
{
    auto data      = ialloc<CatalogThreadData>(g_persistent);
    data->folders  = folders;
    data->callback = callback;

    pthread_t *thread = ialloc<pthread_t>(g_heap);
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

    g_platform->allocators.heap        = heap_allocator(heap_mem, heap_size);
    g_platform->allocators.debug_frame = linear_allocator(debug_frame_mem, debug_frame_size);
    g_platform->allocators.frame       = linear_allocator(frame_mem, frame_size);
    g_platform->allocators.persistent  = linear_allocator(persistent_mem, persistent_size);
    g_platform->allocators.stack       = stack_allocator(stack_mem, stack_size);
    g_platform->allocators.system      = system_allocator();

    g_heap         = &g_platform->allocators.heap;
    g_debug_frame  = &g_platform->allocators.debug_frame;
    g_frame        = &g_platform->allocators.frame;
    g_persistent   = &g_platform->allocators.persistent;
    g_stack        = &g_platform->allocators.stack;
    g_system_alloc = &g_platform->allocators.system;

    init_paths(g_persistent);
    init_alsa();

    FilePath settings_path = resolve_file_path(
        GamePath_preferences,
        "settings.conf",
        g_frame);

    serialize_load_conf(
        settings_path,
        Settings_members,
        ARRAY_SIZE(Settings_members),
        &g_settings);

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
        LOG("Initialised XKB version %d.%d", xkb_major, xkb_minor);
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
            g_mouse.position = { (f32)win_x, (f32)win_y };
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
    (void)platform;

    profiler_begin_frame();
    game_begin_frame();

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

            g_mouse.pressed = true;
            g_mouse.drag_start.x = (f32)xevent.xbutton.x;
            g_mouse.drag_start.y = (f32)xevent.xbutton.y;

        } break;
        case ButtonRelease: {
            InputEvent event;
            event.type         = InputType_mouse_release;
            event.mouse.x      = xevent.xbutton.x;
            event.mouse.y      = xevent.xbutton.y;
            event.mouse.button = xevent.xbutton.button;
            game_input(event);

            g_mouse.pressed = false;
        } break;
        case MotionNotify: {
            if (g_mouse.raw_mouse) {
                break;
            }

            g_mouse.position = { (f32)xevent.xmotion.x, (f32)xevent.xmotion.y };

            if (g_mouse.pressed) {
                InputEvent event;

                event.type = InputType_mouse_drag;
                event.mouse_drag.start   = g_mouse.drag_start;
                event.mouse_drag.current = g_mouse.position;

                game_input(event);
            }
#if 0
            InputEvent event;
            event.type = InputType_mouse_move;
            event.mouse.x = xevent.xmotion.x;
            event.mouse.y = xevent.xmotion.y;

            event.mouse.dx = xevent.xmotion.x - native->mouse.x;
            event.mouse.dy = xevent.xmotion.y - native->mouse.y;

            game_input(event);
#endif
        } break;
        case EnterNotify: {
            if (xevent.xcrossing.focus == true &&
                xevent.xcrossing.window == native->window &&
                xevent.xcrossing.display == native->display)
            {
                g_mouse.position = {
                    (f32)xevent.xcrossing.x,
                    (f32)xevent.xcrossing.y
                };
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
                    if (!g_mouse.raw_mouse) {
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
                    LOG("unhandled xinput2 event: %d", xevent.xcookie.evtype);
                    break;
                }
            } else {
                LOG("unhandled generic event");
            }
        } break;
        default: {
            LOG("unhandled xevent type: %d", xevent.type);
        } break;
        }
    }
    PROFILE_END(linux_input);

    game_update_and_render(dt);
}
