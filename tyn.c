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

#include <init.h>
#include <rpc.h>
#include <sleep.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <syscall.h>
#include <sys/types.h>

#include "cdi/audio.h"

#include "player.h"

struct mixer_conf {
    int sample_rate, channel_count;
    cdi_audio_sample_format_t sample_format;
} __attribute__((packed));

static size_t frame_size;
static pid_t mixer;
static uint32_t shm_id;
static void* shm_buf;

static bool prepare(int sample_rate, int channels, enum sample_type sample_type,
    int sample_size)
{
    mixer = init_service_get("mixer");

    if (!mixer) {
        return false;
    }

    struct mixer_conf mc = {
        .sample_rate = sample_rate,
        .channel_count = channels
    };

    switch (sample_size) {
        case 1: mc.sample_format = CDI_AUDIO_8SI;
                break;
        case 2: mc.sample_format = CDI_AUDIO_16SI;
                break;
        case 4: mc.sample_format = CDI_AUDIO_32SI;
                break;
        default:
            return false;
    }

    if (sample_type != ST_SIGNED_INTEGER_LE) {
        return false;
    }

    if (!rpc_get_int(mixer, "MIXCONF", sizeof(mc), (char*) &mc)) {
        return false;
    }

    frame_size = channels * sample_size;

    size_t size = BUFFER_SIZE;
    shm_id = rpc_get_dword(mixer, "GETSHM", sizeof(size), (char*) &size);

    if (shm_id) {
        shm_buf = open_shared_memory(shm_id);
    }

    if (shm_buf == NULL) {
        mc.channel_count = 0;
        rpc_get_int(mixer, "MIXCONF", sizeof(mc), (char*) &mc);
        return false;
    }

    return true;
}

static void unload(void)
{
    close_shared_memory(shm_id);

    struct mixer_conf mc = {
        .sample_rate = 0,
        .channel_count = 0
    };

    rpc_get_int(mixer, "MIXCONF", sizeof(mc), (char*) &mc);
}

static void play(void* buffer, size_t frames)
{
    memcpy(shm_buf, buffer, frames * frame_size);
    rpc_get_int(mixer, "PLAY", sizeof(shm_id), (char*) &shm_id);
}

static void wait4buf(size_t frames)
{
    // kaputt, aber tut hoffentlich
    int dummy = 0;
    while (rpc_get_int(mixer, "POS", sizeof(dummy), (char*) &dummy) >
        (frames * frame_size + 2 * BUFFER_SIZE))
    {
        msleep(10);
    }
}

static const struct output_device tyn = {
    .name = "tyndur",
    .description = "Sound output on tyndur.",
    .prepare = &prepare,
    .unload = &unload,
    .play = &play,
    .wait_until_buffer_below = &wait4buf
};

DEFINE_AO(&tyn)

