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

struct game_sound_output_buffer
{
    int16_t *samples;
    int sampleCount;
    int samplesPerSecond;
};

internal void GameUpdateAndRender(game_offscreen_buffer *buffer, int blueOffset, int greenOffset, game_sound_output_buffer *soundBuffer);

/*
 * Services that the platform layer provides to the game
 */

#define HANDMADE_H
#endif