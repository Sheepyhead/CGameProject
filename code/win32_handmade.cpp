#include <windows.h>
#include <stdint.h>
#include <Xinput.h>
#include <dsound.h>

#define internal static
#define local_persist static
#define global_variable static

typedef int32_t bool32_t;

struct win32_offscreen_buffer
{
    BITMAPINFO info;
    void *memory;
    int width;
    int height;
    int pitch;
    int bytesPerPixel;
};

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}

global_variable x_input_get_state *DynamicXInputGetState = XInputGetStateStub;
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_set_state *DynamicXInputSetState = XInputSetStateStub;

#define XInputGetState DynamicXInputGetState
#define XInputSetState DynamicXInputSetState

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuideDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal void win32LoadXInput()
{
    //TODO tpj: Test this on Windows 8
    HMODULE xInputLibrary = LoadLibrary("xinput1_4.dll");
    if (!xInputLibrary)
    {
        HMODULE xInputLibrary = LoadLibrary("xinput1_3.dll");
    }

    if (xInputLibrary)
    {
        XInputGetState = (x_input_get_state *)GetProcAddress(xInputLibrary, "XInputGetState");
        XInputSetState = (x_input_set_state *)GetProcAddress(xInputLibrary, "XInputSetState");
    }
}

global_variable bool32_t _running;
global_variable win32_offscreen_buffer _globalBackbuffer;

internal void win32InitDSound(HWND window, int32_t samplesPerSecond, int32_t bufferSize)
{
    HMODULE dSoundLibrary = LoadLibrary("dsound.dll");

    if (dSoundLibrary)
    {
        direct_sound_create *directSoundCreate = (direct_sound_create *)GetProcAddress(dSoundLibrary, "DirectSoundCreate");
        LPDIRECTSOUND directSound;
        if (directSoundCreate && SUCCEEDED(directSoundCreate(0, &directSound, 0)))
        {
            WAVEFORMATEX waveFormat = {};
            waveFormat.wFormatTag = WAVE_FORMAT_PCM;
            waveFormat.nChannels = 2;
            waveFormat.nSamplesPerSec = samplesPerSecond;
            waveFormat.wBitsPerSample = 16;
            waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
            waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
            waveFormat.cbSize = 0;
            if (SUCCEEDED(directSound->SetCooperativeLevel(window, DSSCL_PRIORITY)))
            {
                LPDIRECTSOUNDBUFFER primaryBuffer;
                DSBUFFERDESC bufferDescription = {};
                bufferDescription.dwSize = sizeof(bufferDescription);
                bufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

                if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription, &primaryBuffer, 0)))
                {
                    if (SUCCEEDED(primaryBuffer->SetFormat(&waveFormat)))
                    {
                        OutputDebugString("Primary buffer format was set.\n");
                    }
                    else
                    {
                    }
                }
                else
                {
                }
            }
            else
            {
            }
            LPDIRECTSOUNDBUFFER secondaryBuffer;
            DSBUFFERDESC bufferDescription = {};
            bufferDescription.dwBufferBytes = bufferSize;
            bufferDescription.lpwfxFormat = &waveFormat;
            bufferDescription.dwSize = sizeof(bufferDescription);
            bufferDescription.dwFlags = 0;

            if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription, &secondaryBuffer, 0)))
            {
                OutputDebugString("Secondary buffer created.\n");
            }
            else
            {
            }
        }
        else
        {
        }
    }
}

struct win32_window_dimension
{
    int width;
    int height;
};
internal win32_window_dimension win32GetWindowDimension(HWND window)
{

    win32_window_dimension result;

    RECT clientRect;
    GetClientRect(window, &clientRect);
    result.height = clientRect.bottom - clientRect.top;
    result.width = clientRect.right - clientRect.left;

    return result;
}

internal void renderWeirdGradient(win32_offscreen_buffer *buffer, int xOffset, int yOffset)
{
    int width = buffer->width;
    int height = buffer->height;
    uint8_t *row = (uint8_t *)buffer->memory;
    for (int y = 0; y < buffer->height; y++)
    {
        uint32_t *pixel = (uint32_t *)row;
        for (int x = 0; x < buffer->width; x++)
        {
            uint8_t blue = (x + xOffset);
            uint8_t green = (y + yOffset);
            uint8_t red = blue * green;

            *pixel++ = ((((red << 8) | green) << 8) | blue);
        }
        row += buffer->pitch;
    }
}

internal void win32ResizeDIBSection(win32_offscreen_buffer *buffer, int width, int height)
{

    if (buffer->memory)
    {
        VirtualFree(buffer->memory, 0, MEM_RELEASE);
    }

    buffer->width = width;
    buffer->height = height;
    buffer->bytesPerPixel = 4;

    buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
    buffer->info.bmiHeader.biWidth = buffer->width;
    buffer->info.bmiHeader.biHeight = -buffer->height; // negative for top-down
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = 32;
    buffer->info.bmiHeader.biCompression = BI_RGB;

    int bitmapMemorySize = buffer->bytesPerPixel * buffer->width * buffer->height;
    buffer->memory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

    renderWeirdGradient(buffer, 128, 128);

    buffer->pitch = width * buffer->bytesPerPixel;
}

internal void win32DisplayBufferInWindow(HDC deviceContext, int windowWidth, int windowHeight, win32_offscreen_buffer *buffer, int x, int y, int width, int height)
{
    StretchDIBits(deviceContext, 0, 0, windowWidth, windowHeight, 0, 0, buffer->width, buffer->height, buffer->memory, &buffer->info, DIB_RGB_COLORS, SRCCOPY);
}

internal LRESULT CALLBACK win32MainWindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;
    switch (message)
    {
    case WM_SIZE:
    {
        OutputDebugString("WM_SIZE\n");
    }
    break;

    case WM_DESTROY:
    {
        _running = false;
        OutputDebugString("WM_DESTROY\n");
    }
    break;

    case WM_CLOSE:
    {
        _running = false;
        OutputDebugString("WM_CLOSE\n");
    }
    break;

    case WM_ACTIVATEAPP:
    {
        OutputDebugString("WM_ACTIVATEAPP\n");
    }
    break;

    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
        uint32_t vkCode = wParam;
        bool32_t wasDown = ((lParam & (1 << 30)) != 0);
        bool32_t isDown = ((lParam & (1 << 31)) == 0); //TODO: Verify that this is correct

        if (wasDown != isDown)
        {
            if (vkCode == 'W')
            {
            }
            else if (vkCode == 'A')
            {
            }
            else if (vkCode == 'S')
            {
            }
            else if (vkCode == 'D')
            {
            }
            else if (vkCode == 'Q')
            {
            }
            else if (vkCode == 'E')
            {
            }
            else if (vkCode == VK_UP)
            {
            }
            else if (vkCode == VK_LEFT)
            {
            }
            else if (vkCode == VK_DOWN)
            {
            }
            else if (vkCode == VK_RIGHT)
            {
            }
            else if (vkCode == VK_ESCAPE)
            {
            }
            else if (vkCode == VK_SPACE)
            {
            }
        }
        bool32_t altKeyWasDown = ((lParam & (1 << 29)) != 0);
        if ((vkCode == VK_F4) && altKeyWasDown)
        {
            _running = false;
        }
    }
    break;

    case WM_PAINT:
    {
        PAINTSTRUCT paint;
        HDC deviceContext = BeginPaint(window, &paint);
        int x = paint.rcPaint.left;
        int y = paint.rcPaint.top;
        int height = paint.rcPaint.bottom - paint.rcPaint.top;
        int width = paint.rcPaint.right - paint.rcPaint.left;

        win32_window_dimension dimension = win32GetWindowDimension(window);

        win32DisplayBufferInWindow(deviceContext, dimension.width, dimension.height, &_globalBackbuffer, x, y, width, height);

        EndPaint(window, &paint);
    }
    break;

    default:
    {
        result = DefWindowProc(window, message, wParam, lParam);
    }
    break;
    }

    return result;
}

int WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR commandLine, int showCode)
{
    win32LoadXInput();
    WNDCLASS windowClass = {};

    win32ResizeDIBSection(&_globalBackbuffer, 1280, 720);

    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = win32MainWindowCallback;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = "HandmadeHeroWindowClass";
    if (RegisterClass(&windowClass))
    {
        HWND window = CreateWindowEx(
            0,
            windowClass.lpszClassName,
            "Handmade Hero",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            instance,
            0);
        if (window)
        {
            int xOffset = 0;
            int yOffset = 0;

            win32InitDSound(window, 48000, 48000 * sizeof(int16_t) * 2);

            _running = true;
            while (_running)
            {
                MSG message;
                while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
                {
                    if (message.message == WM_QUIT)
                        _running = false;

                    TranslateMessage(&message);
                    DispatchMessage(&message);
                }

                for (int controllerIndex = 0; controllerIndex < XUSER_MAX_COUNT; controllerIndex++)
                {
                    XINPUT_STATE controllerState;
                    if (XInputGetState(controllerIndex, &controllerState) == ERROR_SUCCESS)
                    {
                        // Controller is plugged in
                        XINPUT_GAMEPAD *pad = &controllerState.Gamepad;

                        bool32_t up = (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                        bool32_t down = (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                        bool32_t left = (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                        bool32_t right = (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                        bool32_t start = (pad->wButtons & XINPUT_GAMEPAD_START);
                        bool32_t back = (pad->wButtons & XINPUT_GAMEPAD_BACK);
                        bool32_t leftShoulder = (pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                        bool32_t rightShoulder = (pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
                        bool32_t aButton = (pad->wButtons & XINPUT_GAMEPAD_A);
                        bool32_t bButton = (pad->wButtons & XINPUT_GAMEPAD_B);
                        bool32_t xButton = (pad->wButtons & XINPUT_GAMEPAD_X);
                        bool32_t yButton = (pad->wButtons & XINPUT_GAMEPAD_Y);

                        int16_t stickX = pad->sThumbLX;
                        int16_t stickY = pad->sThumbLY;

                        if (aButton)
                            --yOffset;
                        else
                            ++yOffset;
                    }
                    else
                    {
                        // Controller is not available
                    }
                }

                XINPUT_VIBRATION vibration;
                vibration.wLeftMotorSpeed = 60000;
                vibration.wRightMotorSpeed = 60000;
                XInputSetState(0, &vibration);

                renderWeirdGradient(&_globalBackbuffer, xOffset, yOffset);
                HDC deviceContext = GetDC(window);

                win32_window_dimension dimension = win32GetWindowDimension(window);

                win32DisplayBufferInWindow(deviceContext, dimension.width, dimension.height, &_globalBackbuffer, 0, 0, dimension.width, dimension.height);
                ReleaseDC(window, deviceContext);

                ++xOffset;
            }
        }
        else
        {
            // TODO: LOGGING
        }
    }
    else
    {
        // TODO: LOGGING
    }

    return (0);
}