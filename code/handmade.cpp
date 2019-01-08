#include "handmade.h"

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

internal void GameUpdateAndRender(game_offscreen_buffer *buffer, int blueOffset, int greenOffset)
{
    renderWeirdGradient(buffer, blueOffset, greenOffset);
}