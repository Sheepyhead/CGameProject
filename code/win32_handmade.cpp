#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <Xinput.h>
#include <dsound.h>
#include <math.h>

#define internal static
#define local_persist static
#define global_variable static

#define pi32 3.14159265359f

#include "handmade.cpp"

typedef int32_t bool32_t;

struct win32_offscreen_buffer
{
    BITMAPINFO info;
    void *memory;
    int width;
    int height;
    int pitch;
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
global_variable LPDIRECTSOUNDBUFFER globalSecondaryBuffer;

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

            DSBUFFERDESC bufferDescription = {};
            bufferDescription.dwBufferBytes = bufferSize;
            bufferDescription.lpwfxFormat = &waveFormat;
            bufferDescription.dwSize = sizeof(bufferDescription);
            bufferDescription.dwFlags = 0;

            if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription, &globalSecondaryBuffer, 0)))
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

internal void win32ResizeDIBSection(win32_offscreen_buffer *buffer, int width, int height)
{

    if (buffer->memory)
    {
        VirtualFree(buffer->memory, 0, MEM_RELEASE);
    }

    buffer->width = width;
    buffer->height = height;
    int bytesPerPixel = 4;

    buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
    buffer->info.bmiHeader.biWidth = buffer->width;
    buffer->info.bmiHeader.biHeight = -buffer->height; // negative for top-down
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = 32;
    buffer->info.bmiHeader.biCompression = BI_RGB;

    int bitmapMemorySize = bytesPerPixel * buffer->width * buffer->height;
    buffer->memory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
    buffer->pitch = width * bytesPerPixel;
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
        bool32_t isDown = ((lParam & (1 << 31)) == 0);

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

struct win32_sound_output
{
    int samplesPerSecond;
    int toneFrequency;
    int16_t toneVolume;
    uint32_t runningSampleIndex;
    int wavePeriod;
    int bytesPerSample;
    int secondaryBufferSize;
    int latencySampleCount;
};

void win32ClearSoundBuffer(win32_sound_output *soundOutput)
{
    VOID *region1;
    DWORD region1Size;
    VOID *region2;
    DWORD region2Size;

    if (SUCCEEDED(globalSecondaryBuffer->Lock(0, soundOutput->secondaryBufferSize,
                                              &region1, &region1Size,
                                              &region2, &region2Size,
                                              0)))
    {
        uint8_t *destSample = (uint8_t *)region1;
        for (DWORD byteIndex = 0; byteIndex < region1Size; ++byteIndex)
        {
            *destSample++ = 0;
        }

        destSample = (uint8_t *)region2;
        for (DWORD byteIndex = 0; byteIndex < region2Size; ++byteIndex)
        {
            *destSample++ = 0;
        }

        globalSecondaryBuffer->Unlock(region1, region1Size, region2, region2Size);
    }
}

void win32FillSoundBuffer(win32_sound_output *soundOutput, DWORD byteToLock, DWORD bytesToWrite, game_sound_output_buffer *sourceBuffer)
{
    VOID *region1;
    DWORD region1Size;
    VOID *region2;
    DWORD region2Size;

    if (SUCCEEDED(globalSecondaryBuffer->Lock(
            byteToLock,
            bytesToWrite,
            &region1, &region1Size,
            &region2, &region2Size,
            0)))
    {
        int16_t *destSample = (int16_t *)region1;
        int16_t *sourceSample = sourceBuffer->samples;
        DWORD region1SampleCount = region1Size / soundOutput->bytesPerSample;
        for (DWORD sampleIndex = 0; sampleIndex < region1SampleCount; ++sampleIndex)
        {
            *destSample++ = *sourceSample++;
            *destSample++ = *sourceSample++;

            ++soundOutput->runningSampleIndex;
        }

        destSample = (int16_t *)region2;
        DWORD region2SampleCount = region2Size / soundOutput->bytesPerSample;
        for (DWORD sampleIndex = 0; sampleIndex < region2SampleCount; ++sampleIndex)
        {
            *destSample++ = *sourceSample++;
            *destSample++ = *sourceSample++;

            ++soundOutput->runningSampleIndex;
        }

        globalSecondaryBuffer->Unlock(region1, region1Size, region2, region2Size);
    }
}

#if HANDMADE_WIN32
int WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR commandLine, int showCode)
{
#else
int main(int argc, char **argv)
#endif
    LARGE_INTEGER performanceFrequencyResult;
    QueryPerformanceFrequency(&performanceFrequencyResult);
    uint64_t performanceFrequency = performanceFrequencyResult.QuadPart;

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
            //graphics
            int xOffset = 0;
            int yOffset = 0;

            win32_sound_output soundOutput = {};

            soundOutput.samplesPerSecond = 48000;
            soundOutput.toneFrequency = 256;
            soundOutput.toneVolume = 3000;
            soundOutput.wavePeriod = soundOutput.samplesPerSecond / soundOutput.toneFrequency;
            soundOutput.bytesPerSample = sizeof(int16_t) * 2;
            soundOutput.secondaryBufferSize = soundOutput.samplesPerSecond * soundOutput.bytesPerSample;
            soundOutput.latencySampleCount = soundOutput.samplesPerSecond / 15;

            win32InitDSound(window, soundOutput.samplesPerSecond, soundOutput.secondaryBufferSize);
            win32ClearSoundBuffer(&soundOutput);
            globalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

            LARGE_INTEGER lastCounter;
            QueryPerformanceCounter(&lastCounter);

            int64_t lastCycleCount = __rdtsc();

            _running = true;
            while (_running) // Game loop start
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

                DWORD byteToLock = 0;
                DWORD targetCursor = 0;
                DWORD bytesToWrite = 0;
                DWORD playCursor = 0;
                DWORD writeCursor = 0;
                bool32_t soundIsValid = false;
                if (SUCCEEDED(globalSecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor)))
                {
                    byteToLock = ((soundOutput.runningSampleIndex * soundOutput.bytesPerSample) % soundOutput.secondaryBufferSize);
                    targetCursor = ((playCursor + (soundOutput.latencySampleCount * soundOutput.bytesPerSample)) % soundOutput.secondaryBufferSize);
                    if (byteToLock > targetCursor)
                    {
                        bytesToWrite = soundOutput.secondaryBufferSize - byteToLock;
                        bytesToWrite += targetCursor;
                    }
                    else
                    {
                        bytesToWrite = targetCursor - byteToLock;
                    }

                    soundIsValid = true;
                }

                int16_t samples[48000 * 2];
                game_sound_output_buffer soundBuffer = {};
                soundBuffer.samplesPerSecond = soundOutput.samplesPerSecond;
                soundBuffer.sampleCount = bytesToWrite / soundOutput.bytesPerSample;
                soundBuffer.samples = samples;

                game_offscreen_buffer buffer = {};
                buffer.height = _globalBackbuffer.height;
                buffer.memory = _globalBackbuffer.memory;
                buffer.width = _globalBackbuffer.width;
                buffer.pitch = _globalBackbuffer.pitch;
                GameUpdateAndRender(&buffer, xOffset, yOffset, &soundBuffer);

                HDC deviceContext = GetDC(window);

                // DirectSound output test

                if (soundIsValid)
                {

                    win32FillSoundBuffer(&soundOutput, byteToLock, bytesToWrite, &soundBuffer);
                }
                win32_window_dimension dimension = win32GetWindowDimension(window);

                win32DisplayBufferInWindow(deviceContext, dimension.width, dimension.height, &_globalBackbuffer, 0, 0, dimension.width, dimension.height);
                ReleaseDC(window, deviceContext);

                ++xOffset;

                LARGE_INTEGER endCounter;
                QueryPerformanceCounter(&endCounter);

                uint64_t endCycleCount = __rdtsc();

                uint64_t cyclesElapsed = endCycleCount - lastCycleCount;
                int64_t counterElapsed = endCounter.QuadPart - lastCounter.QuadPart;
                float msPerFrame = ((1000.0f * (float)counterElapsed) / (float)performanceFrequency);
                float fps = 1000.0f / (float)msPerFrame;
                float megaCyclesPerFrame = ((float)cyclesElapsed / (1000.0f * 1000.0f));
                /*
                char buffer[256];
                sprintf(buffer, "%.02fms,  %.02ff/s,  %.02fMc/f\n", msPerFrame, fps, megaCyclesPerFrame);
                OutputDebugString(buffer);
*/
                lastCounter = endCounter;
                lastCycleCount = endCycleCount;
            } // Game loop end
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