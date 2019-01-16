#include "handmade.h"

internal void GameOutputSound(game_sound_output_buffer *soundBuffer)
{
    local_persist float tSine;
    int16_t toneVolume = 3000;
    int toneFrequency = 256;

    int wavePeriod = soundBuffer->samplesPerSecond / toneFrequency;

    int16_t *soundOut = soundBuffer->samples;
    for (int sampleIndex = 0; sampleIndex < soundBuffer->sampleCount; ++sampleIndex)
    {
        float sineValue = sinf(tSine);
        int16_t sampleValue = (int16_t)(sineValue * toneVolume);
        *soundOut++ = sampleValue;
        *soundOut++ = sampleValue;

        tSine += 2.0f * pi32 * 1.0f / (float)wavePeriod;
    }
}

internal void renderWeirdGradient(game_offscreen_buffer *buffer, int xOffset, int yOffset)
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

internal void GameUpdateAndRender(game_offscreen_buffer *buffer, int blueOffset, int greenOffset, game_sound_output_buffer *soundBuffer)
{
    GameOutputSound(soundBuffer);
    renderWeirdGradient(buffer, blueOffset, greenOffset);
}