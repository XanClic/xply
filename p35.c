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

#include <stdbool.h>
#include <stddef.h>
#include <vfs.h>

#include <drv/audio.h>

#include "player.h"

static pipe mixer;
static size_t frame_size;

static bool prepare(int sample_rate, int channels, enum sample_type sample_type __attribute__((unused)), int sample_size)
{
    mixer = create_pipe("(mixer)/", O_WRONLY);

    if (mixer == NULL)
        return false;

    if (!pipe_set_flag(mixer, O_AUDIO_CHANNELS, channels))
        return false;

    if (!pipe_set_flag(mixer, O_AUDIO_SAMPLERATE, sample_rate))
        return false;

    frame_size = channels * sample_size;

    return true;
}

static void unload(void)
{
    destroy_pipe(mixer, 0);
}

static void play(void *buffer, size_t frames)
{
    stream_send(mixer, buffer, frames * frame_size, 0);
}

static void wait4buf(size_t frames)
{
    // kaputt, aber tut hoffentlich
    while (pipe_get_flag(mixer, O_AUDIO_BUFFER_CONTENT) > (frames * frame_size + BUFFER_SIZE))
        msleep(10);
}

static const struct output_device p35 = {
    .name = "p35",
    .description = "Sound output on paloxena3.5 via (mixer).",
    .prepare = &prepare,
    .unload = &unload,
    .play = &play,
    .wait_until_buffer_below = &wait4buf
};

DEFINE_AO(&p35)

