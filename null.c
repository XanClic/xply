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

#include "player.h"

#define unused __attribute__((unused))

static bool prepare(int sample_rate unused, int channels unused, enum sample_type sample_type unused, int sample_size unused)
{
    return true;
}

static void play(void *buffer unused, size_t frames unused)
{
}

static void wait4buf(size_t frames unused)
{
}

static const struct output_device null = {
    .name = "null",
    .description = "Discards all output data.",
    .prepare = &prepare,
    .play = &play,
    .wait_until_buffer_below = &wait4buf
};

DEFINE_AO(&null)

