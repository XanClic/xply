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

#ifndef PLAYER_H
#define PLAYER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

enum sample_type
{
    ST_SIGNED_INTEGER_LE = 0
};

struct output_device
{
    const char *name;
    const char *description;

    bool (*prepare)(int sample_rate, int channels, enum sample_type sample_type, int sample_size);
    void (*unload)(void);
    void (*play)(void *buffer, size_t frames);
    void (*wait_until_buffer_below)(size_t frames);
};

struct file_type;

struct decoder
{
    const char *name;

    bool (*check_file)(FILE *fp, struct file_type **ft);
    size_t (*decode)(struct file_type *ft, size_t frames, void *buffer);
};

struct file_type
{
    const struct decoder *decoder;

    int sample_rate;
    int channels;
    size_t sample_size;
    enum sample_type sample_type;
    double bitrate;

    size_t position;
    size_t length;
};


#ifndef __GLUE
#define __GLUE(a, b) a ## b
#endif

#define __DEFINE_AO(ao_struct, cnt) static const struct output_device *__attribute__((section("ao"), used)) __GLUE(__ao_, cnt) = ao_struct;
#define DEFINE_AO(ao_struct) __DEFINE_AO(ao_struct, __COUNTER__)

#define __DEFINE_DC(dc_struct, cnt) static const struct decoder *__attribute__((section("dc"), used)) __GLUE(__dec_, cnt) = dc_struct;
#define DEFINE_DC(dc_struct) __DEFINE_DC(dc_struct, __COUNTER__)


#define BUFFER_SIZE 32768


const struct output_device *find_output_device(const char *name);
struct file_type *get_file_type(FILE *fp);

#endif

