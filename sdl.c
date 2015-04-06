/******************************************************************************
 * Copyright (c) 2010 Hanna Reitz                                             *
 *                                                                            *
 * Permission is hereby granted,  free of charge,  to any  person obtaining a *
 * copy of this software and associated documentation files (the "Software"), *
 * to deal in the Software without restriction,  including without limitation *
 * the rights to use,  copy, modify, merge, publish,  distribute, sublicense, *
 * and/or sell copies  of the  Software,  and to permit  persons to whom  the *
 * Software is furnished to do so, subject to the following conditions:       *
 *                                                                            *
 * The above copyright notice and this permission notice shall be included in *
 * all copies or substantial portions of the Software.                        *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING  BUT NOT  LIMITED TO THE WARRANTIES OF MERCHANTABILITY, *
 * FITNESS FOR A PARTICULAR  PURPOSE AND  NONINFRINGEMENT.  IN NO EVENT SHALL *
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
 * LIABILITY,  WHETHER IN AN ACTION OF CONTRACT,  TORT OR OTHERWISE,  ARISING *
 * FROM,  OUT OF  OR IN CONNECTION  WITH THE  SOFTWARE  OR THE  USE OR  OTHER *
 * DEALINGS IN THE SOFTWARE.                                                  *
 ******************************************************************************/

#define _DEFAULT_SOURCE

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <SDL.h>

#include "player.h"

static int cpb = 0;
static size_t buffer_offset[2] = { 0 }, buffer_remaining[2] = { 0 };
static uint8_t *sdl_buf[2];
static size_t frame_size;

static void sdl_callback(void *userdata __attribute__((unused)), Uint8 *stream, int len)
{
    if (!buffer_remaining[cpb])
    {
        if (!buffer_remaining[!cpb])
            return;

        cpb ^= 1;
    }

    size_t this_size = (buffer_remaining[cpb] > (size_t)len) ? (size_t)len : buffer_remaining[cpb];

    memcpy(stream, sdl_buf[cpb] + buffer_offset[cpb], this_size);

    buffer_offset[cpb] += this_size;
    buffer_remaining[cpb] -= this_size;

    len -= this_size;

    if ((len > 0) && buffer_remaining[!cpb])
    {
        cpb ^= 1;

        size_t new_size = (buffer_remaining[cpb] > (size_t)len) ? (size_t)len : buffer_remaining[cpb];

        memcpy(stream + this_size, sdl_buf[cpb] + buffer_offset[cpb], new_size);

        buffer_offset[cpb] += new_size;
        buffer_remaining[cpb] -= new_size;
    }
}

static bool prepare(int sample_rate, int channels, enum sample_type sample_type, int sample_size)
{
    SDL_AudioSpec aspec, obtained;

    frame_size = channels * sample_size;

    aspec.freq = sample_rate;

    switch (sample_type)
    {
        case ST_SIGNED_INTEGER_LE:
            if (sample_size == 2)
                aspec.format = AUDIO_S16LSB;
            else
                aspec.format = AUDIO_S8; // FIXME
            break;
    }

    aspec.samples = 4096 / sample_size;
    aspec.callback = &sdl_callback;

    if (SDL_Init(SDL_INIT_AUDIO))
        return false;

    if (SDL_OpenAudio(&aspec, &obtained) < 0)
        return false;

    sdl_buf[0] = calloc(1, BUFFER_SIZE);
    sdl_buf[1] = calloc(1, BUFFER_SIZE);

    SDL_PauseAudio(0);

    return true;
}

static void play(void *buffer, size_t frames)
{
    memcpy(sdl_buf[!cpb], buffer, frames * frame_size);
    buffer_offset[!cpb] = 0;
    buffer_remaining[!cpb] = frames * frame_size;
}

static void wait4buf(size_t frames)
{
    frames *= frame_size;

    while (buffer_remaining[0] + buffer_remaining[1] >= frames)
        usleep(10000);
}

static const struct output_device sdl = {
    .name = "sdl",
    .description = "Outputs sound via SDL_Audio.",
    .prepare = &prepare,
    .play = &play,
    .wait_until_buffer_below = &wait4buf
};

DEFINE_AO(&sdl)

