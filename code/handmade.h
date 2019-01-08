#if !defined(HANDMADE_H)

/*
 * Services that the game provides to the platform layer
 */

struct game_offscreen_buffer
{
    void *memory;
    int width;
    int height;
    int pitch;
};

internal void GameUpdateAndRender(game_offscreen_buffer *buffer, int blueOffset, int greenOffset);

internal void renderWeirdGradient(game_offscreen_buffer *buffer, int xOffset, int yOffset);
/*
 * Services that the platform layer provides to the game
 */

#define HANDMADE_H
#endif