/**
 * file:    win32_leary.cpp
 * created: 2017-03-27
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017-2018 - all rights reserved
 */

PlatformState   *g_platform;
Settings         g_settings;

Allocator *g_heap;
Allocator *g_frame;
Allocator *g_debug_frame;
Allocator *g_persistent;
Allocator *g_stack;
Allocator *g_system_alloc;

struct CatalogThreadData {
    FolderPathView folder;
    catalog_callback_t *callback;
};

struct MouseState {
    f32 x, y;
    f32 dx, dy;
    bool in_window = false;
};

extern "C"
DWORD catalog_thread_process(void *data)
{
    CatalogThreadData *ctd = (CatalogThreadData*)data;

    HANDLE fh = CreateFile(
        ctd->folder.absolute.bytes,
        GENERIC_READ | FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
        NULL);

    ASSERT(fh != INVALID_HANDLE_VALUE);

    bool eslash = ctd->folder[ctd->folder.absolute.size - 1] == '\\';

    DWORD buffer[2048];
    DWORD bytes = 0;

    while (ReadDirectoryChangesW(fh,
                                 buffer, sizeof buffer,
                                 true, FILE_NOTIFY_CHANGE_LAST_WRITE,
                                 &bytes, NULL, NULL) != FALSE)
    {
        DWORD *ptr = buffer;

        FILE_NOTIFY_INFORMATION *fni;
        do {
            fni = (FILE_NOTIFY_INFORMATION*)ptr;
            ptr += fni->NextEntryOffset;

            if (fni->Action == FILE_ACTION_MODIFIED) {
                ASSERT(fni->FileNameLength <= I32_MAX);

                FilePath p;
                if (eslash) {
                    // TODO(jesper): remove g_system_alloc
                    p = create_file_path(
                        g_system_alloc, {
                        ctd->folder.absolute,
                        string_from_utf16((u16*)fni->FileName, wcslen(fni->FileName)) });
                } else {
                    // TODO(jesper): remove g_system_alloc
                    p = create_file_path(
                        g_system_alloc, {
                        ctd->folder.absolute, "\\",
                        string_from_utf16((u16*)fni->FileName, wcslen(fni->FileName)) });
                }

                ctd->callback(p);
            }
        } while (fni->NextEntryOffset > 0);
    }

    CloseHandle(fh);
    return 0;
}

void create_catalog_thread(Array<FolderPath> folders, catalog_callback_t *callback)
{
    for (auto f : folders) {
        auto data = ialloc<CatalogThreadData>(g_persistent);
        data->folder   = f;
        data->callback = callback;

        HANDLE th = CreateThread(NULL,
                                 8 * 1024,
                                 &catalog_thread_process, data,
                                 0, NULL);
        ASSERT(th != NULL);
    }
}

void platform_quit()
{
    FilePath settings_path = resolve_file_path(
        GamePath_preferences,
        "settings.conf",
        g_stack);;

    serialize_save_conf(
        settings_path,
        Settings_members,
        ARRAY_SIZE(Settings_members),
        &g_settings);

    _exit(EXIT_SUCCESS);
}

void platform_toggle_raw_mouse()
{
    g_platform->raw_mouse = !g_platform->raw_mouse;
    LOG("toggle raw mouse: %d", g_platform->raw_mouse);

    if (g_platform->raw_mouse) {
        RAWINPUTDEVICE rid[1];
        rid[0].usUsagePage = 0x01;
        rid[0].usUsage = 0x02;
        rid[0].dwFlags = RIDEV_INPUTSINK | RIDEV_CAPTUREMOUSE | RIDEV_NOLEGACY;
        rid[0].hwndTarget = g_platform->native.hwnd;

        bool result = RegisterRawInputDevices(rid, 1, sizeof rid[0]);
        ASSERT(result != false);
        (void)result;

        while(ShowCursor(false) > 0) {}
    } else {
        RAWINPUTDEVICE rid[1];
        rid[0].usUsagePage = 0x01;
        rid[0].usUsage = 0x02;
        rid[0].dwFlags = RIDEV_REMOVE;
        rid[0].hwndTarget = 0;

        bool result = RegisterRawInputDevices(rid, 1, sizeof rid[0]);
        ASSERT(result != false);
        (void)result;

        while(ShowCursor(true) > 0) {}
    }
}

void platform_set_raw_mouse(bool enable)
{
    if ((enable && !g_platform->raw_mouse) ||
        (!enable && g_platform->raw_mouse))
    {
        platform_toggle_raw_mouse();
    }
}

MouseState g_mouse = {};

Vector2 get_mouse_position()
{
    return { g_mouse.x, g_mouse.y };
}

LRESULT CALLBACK
window_proc(HWND   hwnd,
            UINT   message,
            WPARAM wparam,
            LPARAM lparam)
{
    switch (message) {
    case WM_CLOSE:
    case WM_QUIT:
        game_quit();
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        FillRect(hdc, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW+1));

        EndPaint(hwnd, &ps);
    } break;
    case WM_KEYDOWN: {
        InputEvent event;
        event.type         = InputType_key_press;
        event.key.code     = win32_keycode(wparam);
        event.key.repeated = lparam & 0x40000000;

        game_input(event);
    } break;
    case WM_KEYUP: {
        InputEvent event;
        event.type         = InputType_key_release;
        event.key.code     = win32_keycode(wparam);
        event.key.repeated = false;

        game_input(event);
    } break;
    case WM_MOUSEMOVE: {
        if (g_platform->raw_mouse) {
            break;
        }

        i32 x = lparam & 0xffff;
        i32 y = (lparam >> 16) & 0xffff;

        if (!g_mouse.in_window) {
            g_mouse.in_window = true;
            g_mouse.dx = 0.0f;
            g_mouse.dy = 0.0f;
            g_mouse.x = (f32)x;
            g_mouse.y = (f32)y;
            break;
        }

        g_mouse.dx = (f32)(x - g_mouse.x);
        g_mouse.dy = (f32)(y - g_mouse.y);
        g_mouse.x  = (f32)x;
        g_mouse.y  = (f32)y;

        InputEvent event;
        event.type = InputType_mouse_move;
        event.mouse.x  = g_mouse.x;
        event.mouse.y  = g_mouse.y;
        event.mouse.dx = g_mouse.dx;
        event.mouse.dy = g_mouse.dy;

        game_input(event);
    } break;
    case WM_LBUTTONDOWN: {
        if (g_platform->raw_mouse) {
            break;
        }

        InputEvent event;
        event.type = InputType_mouse_press;
        event.mouse.x = (f32)(lparam & 0xffff);
        event.mouse.y = (f32)((lparam >> 16) & 0xffff);

        game_input(event);
    } break;
    case WM_RBUTTONDOWN: {
        if (g_platform->raw_mouse) {
            break;
        }

        InputEvent event;
        event.type = InputType_right_mouse_press;
        event.mouse.x = (f32)(lparam & 0xffff);
        event.mouse.y = (f32)((lparam >> 16) & 0xffff);

        game_input(event);
    } break;
    case WM_RBUTTONUP: {
        if (g_platform->raw_mouse) {
            break;
        }

        InputEvent event;
        event.type = InputType_right_mouse_release;
        event.mouse.x = (f32)(lparam & 0xffff);
        event.mouse.y = (f32)((lparam >> 16) & 0xffff);

        game_input(event);
    } break;
    case WM_MBUTTONDOWN: {
        if (g_platform->raw_mouse) {
            break;
        }

        InputEvent event;
        event.type = InputType_middle_mouse_press;
        event.mouse.x = (f32)(lparam & 0xffff);
        event.mouse.y = (f32)((lparam >> 16) & 0xffff);

        game_input(event);
    } break;
    case WM_MBUTTONUP: {
        if (g_platform->raw_mouse) {
            break;
        }

        InputEvent event;
        event.type = InputType_middle_mouse_release;
        event.mouse.x = (f32)(lparam & 0xffff);
        event.mouse.y = (f32)((lparam >> 16) & 0xffff);

        game_input(event);
    } break;
    case WM_MOUSEWHEEL: {
        if (g_platform->raw_mouse) {
            break;
        }

        InputEvent event;
        event.type = InputType_mouse_wheel;
        event.mouse_wheel = (f32)GET_WHEEL_DELTA_WPARAM(wparam);
        game_input(event);
    } break;
    case WM_MOUSELEAVE:
        g_mouse.in_window = false;
        break;
    case WM_INPUT: {
        u32 size;
        GetRawInputData((HRAWINPUT)lparam, RID_INPUT, NULL,
                        &size, sizeof(RAWINPUTHEADER));

        u8 *data = new u8[size];
        defer { delete[] data; };

        ASSERT(data != nullptr);

        UINT result = GetRawInputData((HRAWINPUT)lparam, RID_INPUT, data,
                                      &size, sizeof(RAWINPUTHEADER));
        if (result != size) {
            LOG_ERROR("incorrect size from GetRawInputData. Expected: %u, received %u",
                      size, result);
            ASSERT(false);
            break;
        }

        RAWINPUT *raw = (RAWINPUT*)data;

        switch (raw->header.dwType) {
        case RIM_TYPEMOUSE: {
            if (!g_platform->raw_mouse) {
                break;
            }

            if (raw->data.mouse.usFlags == MOUSE_MOVE_RELATIVE) {
                g_mouse.dx = (f32)raw->data.mouse.lLastX;
                g_mouse.dy = (f32)raw->data.mouse.lLastY;
                g_mouse.x  += (f32)raw->data.mouse.lLastX;
                g_mouse.y  += (f32)raw->data.mouse.lLastY;
            } else if (raw->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) {
                g_mouse.dx = (f32)raw->data.mouse.lLastX - g_mouse.x;
                g_mouse.dy = (f32)raw->data.mouse.lLastY - g_mouse.y;
                g_mouse.x  = (f32)raw->data.mouse.lLastX;
                g_mouse.y  = (f32)raw->data.mouse.lLastY;
            } else {
                LOG_ERROR("unsupported flags");
                ASSERT(false);
            }

            InputEvent event;
            event.type = InputType_mouse_move;
            event.mouse.dx = g_mouse.dx;
            event.mouse.dy = g_mouse.dy;
            event.mouse.x  = g_mouse.x;
            event.mouse.y  = g_mouse.y;

            game_input(event);

            DefWindowProc(hwnd, message, wparam, lparam);
        } break;
        default:
            LOG("unhandled raw input device type: %d", raw->header.dwType);
            break;
        }
    } break;
    default:
        //LOG(Log_info, "unhandled event: %d", message);
        return DefWindowProc(hwnd, message, wparam, lparam);
    }

    return 0;
}

#define XAUDIO2_CREATE(name) HRESULT name(IXAudio2 **ppXAudio2, UINT32 Flags, XAUDIO2_PROCESSOR XAudio2Processor)
typedef XAUDIO2_CREATE(XAudio2Create_t);

#define COINITIALIZEEX(name) HRESULT name(LPVOID pvReserved, DWORD dwCoInit)
typedef COINITIALIZEEX(CoInitializeEx_t);

#define COCREATEINSTANCE(name) HRESULT name(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv)
typedef COCREATEINSTANCE(CoCreateInstance_t);

char                *g_xaudio2_buffer = nullptr;
IXAudio2SourceVoice *g_xaudio2_voice  = nullptr;

void init_xaudio2()
{
    HMODULE ole = LoadLibrary("Ole32.dll");

    CoInitializeEx_t *CoInitializeEx = (CoInitializeEx_t*)GetProcAddress(ole, "CoInitializeEx");
    if (CoInitializeEx != nullptr) {
        HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (FAILED(hr)) {
            ASSERT(false);
            return;
        }
    }

    // TODO(jesper): xaudio2_8 fallback
    HMODULE lib = LoadLibrary("xaudio2_9.dll");
    if (lib != nullptr) {
        XAudio2Create_t *XAudio2Create = (XAudio2Create_t*)GetProcAddress(lib, "XAudio2Create");
        ASSERT(XAudio2Create != nullptr);

        HRESULT hr;
        IXAudio2* xaudio2 = nullptr;
        hr = XAudio2Create(&xaudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);

        if (FAILED(hr)) {
            ASSERT(false);
            return;
        }

        IXAudio2MasteringVoice* voice = nullptr;
        hr = xaudio2->CreateMasteringVoice(&voice);

        if (FAILED(hr)) {
            ASSERT(false);
            return;
        }

        WAVEFORMATEX format = {};
        format.wFormatTag = WAVE_FORMAT_PCM;
        format.nChannels = 2;
        format.nSamplesPerSec = 48000;
        format.wBitsPerSample = 16;
        format.cbSize = 0;
        format.nBlockAlign = (format.nChannels * format.wBitsPerSample) / 8;
        format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;

        g_xaudio2_buffer = (char*)malloc(format.nAvgBytesPerSec * 2);

        struct Sample {
            i16 left;
            i16 right;
        };

        Sample *samples = (Sample*)g_xaudio2_buffer;

        i32 hz = 261;
        i32 square_period = 48000 / hz;
        i32 square_counter = 0;
        i16 sample_value = 16000;
        for (u32 i = 0; i < format.nSamplesPerSec * 2; i++) {
            if (square_counter > square_period) {
                sample_value   = -sample_value;
                square_counter = 0;
            }
            square_counter++;

            samples[i].left  = sample_value;
            samples[i].right = sample_value;
        }

        XAUDIO2_BUFFER buffer = {};
        buffer.Flags = 0;
        buffer.AudioBytes = format.nAvgBytesPerSec * 2;
        buffer.pAudioData = (BYTE*)g_xaudio2_buffer;
        buffer.PlayBegin = 0;
        buffer.PlayLength = 0;
        buffer.LoopBegin = 0;
        buffer.LoopLength = 0;
        buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
        buffer.pContext = nullptr;

        hr = xaudio2->CreateSourceVoice(&g_xaudio2_voice, &format);
        if (FAILED(hr)) {
            ASSERT(false);
            return;
        }

        hr = g_xaudio2_voice->SubmitSourceBuffer(&buffer);
        if (FAILED(hr)) {
            ASSERT(false);
            return;
        }

        hr = g_xaudio2_voice->Start(0);
        if (FAILED(hr)) {
            ASSERT(false);
            return;
        }
    }
}

struct WASAPI {
    IAudioClient *client = nullptr;
    IAudioRenderClient *render_client = nullptr;
    IAudioClock *audio_clock = nullptr;
    i32 samples_per_second = 0;
    i32 num_channels = 0;
    i16 bits_per_sample = 16;
    i32 buffer_size_samples = 0;
    i32 bytes_per_sample = 0;
    i32 latency_samples;
};

WASAPI g_wasapi = {};

WASAPI init_wasapi(i32 samples_per_second, i32 num_channels)
{
    WASAPI wasapi = {};

    HRESULT hr;

    HMODULE ole = LoadLibrary("Ole32.dll");
    if (ole == nullptr) {
        ASSERT(false);
        return {};
    }

    auto CoInitializeEx = (CoInitializeEx_t*)GetProcAddress(ole, "CoInitializeEx");
    ASSERT(CoInitializeEx != nullptr);

    hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    ASSERT(SUCCEEDED(hr));

    auto CoCreateInstance = (CoCreateInstance_t*)GetProcAddress(ole, "CoCreateInstance");
    ASSERT(CoCreateInstance != nullptr);

    IMMDeviceEnumerator *enumerator = nullptr;
    hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator),
        nullptr,
        CLSCTX_ALL,
        IID_PPV_ARGS(&enumerator));
    ASSERT(SUCCEEDED(hr));

    IMMDevice *device = nullptr;
    hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
    ASSERT(SUCCEEDED(hr));

    hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&wasapi.client);
    ASSERT(SUCCEEDED(hr));


    wasapi.buffer_size_samples = samples_per_second;
    wasapi.num_channels = num_channels;
    wasapi.samples_per_second = samples_per_second;
    wasapi.bytes_per_sample = (wasapi.num_channels * wasapi.bits_per_sample) / 8;

    // TODO(jesper): tweak and ensure this works when frametime spikes and at
    // different frequencies. Probably want something more dynamic.
    f32 audio_latency_ms = 20.0f;
    wasapi.latency_samples = (i32)(wasapi.samples_per_second * audio_latency_ms / 1000.0f);

    WAVEFORMATEXTENSIBLE wave_format = {};
    wave_format.Format.cbSize = sizeof wave_format;
    wave_format.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    wave_format.Format.wBitsPerSample = wasapi.bits_per_sample;
    wave_format.Format.nChannels = (WORD)wasapi.num_channels;
    wave_format.Format.nSamplesPerSec = wasapi.samples_per_second;
    wave_format.Format.nBlockAlign = (WORD)wasapi.bytes_per_sample;
    wave_format.Format.nAvgBytesPerSec = wave_format.Format.nSamplesPerSec * wave_format.Format.nBlockAlign;
    wave_format.Samples.wValidBitsPerSample = wasapi.bits_per_sample;
    wave_format.dwChannelMask = KSAUDIO_SPEAKER_STEREO;
    wave_format.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

    REFERENCE_TIME buffer_duration = 10000000ULL * wasapi.buffer_size_samples / wasapi.samples_per_second;
    hr = wasapi.client->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_NOPERSIST,
        buffer_duration,
        0,
        &wave_format.Format,
        nullptr);
    ASSERT(SUCCEEDED(hr));

    hr = wasapi.client->GetService(IID_PPV_ARGS(&wasapi.render_client));
    ASSERT(SUCCEEDED(hr));

    hr = wasapi.client->GetService(IID_PPV_ARGS(&wasapi.audio_clock));
    ASSERT(SUCCEEDED(hr));

    u32 num_samples;
    wasapi.client->GetBufferSize(&num_samples);
    ASSERT((u32)wasapi.buffer_size_samples <= num_samples);

    wasapi.client->Start();

    return wasapi;
}

DLL_EXPORT
PLATFORM_INIT_FUNC(platform_init)
{
    g_platform = platform;
    g_settings = {};

    auto native = &g_platform->native;
    native->hinstance = instance;

    isize frame_size       = 64  * 1024 * 1024;
    isize debug_frame_size = 64  * 1024 * 1024;
    isize persistent_size  = 256 * 1024 * 1024;
    isize heap_size        = 256 * 1024 * 1024;
    isize stack_size       = 16  * 1024 * 1024;

    // TODO(jesper): alloc these using appropriate syscalls
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
    g_wasapi = init_wasapi(48000, 2);

    FilePath settings_path = resolve_file_path(
        GamePath_preferences,
        "settings.conf",
        g_frame);

    serialize_load_conf(
        settings_path,
        Settings_members,
        ARRAY_SIZE(Settings_members),
        &g_settings);

    WNDCLASS wc = {};
    wc.lpfnWndProc   = window_proc;
    wc.hInstance     = instance;
    wc.lpszClassName = "leary";

    RegisterClass(&wc);

    native->hwnd = CreateWindow(
        "leary", "leary",
        WS_TILED | WS_VISIBLE | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU,
        0, 0,
        g_settings.video.resolution.width,
        g_settings.video.resolution.height,
        nullptr, nullptr, native->hinstance, nullptr);

    if (native->hwnd == NULL) {
        char* msg = win32_system_error_message(GetLastError());
        LOG_ERROR("failed to create window: %s", msg);
        game_quit();
    }

    game_init();
}

DLL_EXPORT
PLATFORM_PRE_RELOAD_FUNC(platform_pre_reload)
{
    platform->reload_state.game = game_pre_reload();

    platform->reload_state.frame        = g_frame;
    platform->reload_state.debug_frame  = g_debug_frame;
    platform->reload_state.stack        = g_stack;
    platform->reload_state.heap         = g_heap;
    platform->reload_state.persistent   = g_persistent;
    platform->reload_state.system_alloc = g_system_alloc;
}

DLL_EXPORT
PLATFORM_RELOAD_FUNC(platform_reload)
{
    g_frame        = platform->reload_state.frame;
    g_debug_frame  = platform->reload_state.debug_frame;
    g_heap         = platform->reload_state.heap;
    g_persistent   = platform->reload_state.persistent;
    g_stack        = platform->reload_state.stack;
    g_system_alloc = platform->reload_state.system_alloc;
    g_platform     = platform;

    game_reload(platform->reload_state.game);
}

DLL_EXPORT
PLATFORM_UPDATE_FUNC(platform_update)
{
    (void)platform;

    game_begin_frame();

    PROFILE_START(win32_input);
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    PROFILE_END(win32_input);

    i32 samples_to_write = 0;
    u32 padding = 0;

    HRESULT hr;
    hr = g_wasapi.client->GetCurrentPadding(&padding);
    if (SUCCEEDED(hr)) {
        // NOTE(jesper): padding gets us the number of samples of audio that
        // we've already written and is ready to be read. The samples to write
        // this frame is thus max_audio_latency - padding. This does rely on
        // max_audio_latency > max_frame_latency, and should probably be made
        // more dynamic to account for different systems.
        i32 available_samples = g_wasapi.buffer_size_samples - padding;
        samples_to_write = g_wasapi.latency_samples - padding;
        if (samples_to_write > available_samples) {
            samples_to_write = available_samples;
        }
    } else {
        ASSERT(false);
        // TODO(jesper): logging
    }

    if (samples_to_write > 0) {
        i32 *output = ialloc_array<i32>(g_frame, samples_to_write * g_wasapi.num_channels);
        game_output_sound(output, samples_to_write);

        u8 *data = nullptr;
        hr = g_wasapi.render_client->GetBuffer(samples_to_write, &data);
        if (SUCCEEDED(hr)) {
            i16 *samples = (i16*)data;
            for (i32 i = 0; i < samples_to_write * g_wasapi.num_channels; i++) {
                i16 clamped = (i16)clamp(output[i], I16_MIN, I16_MAX);
                samples[i] = clamped;
            }
            g_wasapi.render_client->ReleaseBuffer(samples_to_write, 0);
        } else {
            ASSERT(false);
            // TODO(jesper): logging
        }
    }

    game_update_and_render(dt);
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD fwd, LPVOID reserved)
{
    (void)instance;
    (void)fwd;
    (void)reserved;

    return TRUE;
}
