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

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "player.h"

#define DEFAULT_OUTPUT_DEVICE "pal35"

static const char *stn1[] = {
    "s"
};

static const char *stn2[] = {
    "le"
};

static void help(int exitcode) __attribute__((noreturn));

static void help(int exitcode)
{
    printf("Usage: xply [options] <file>\n\n");
    printf("file: Input file name\n");
    printf("Options:\n");
    printf("  -ao <name>: Changes the audio output device (default is " DEFAULT_OUTPUT_DEVICE ").\n");

    exit(exitcode);
}

int main(int argc, char *argv[])
{
    char *input_file = NULL;
    char *output_device = DEFAULT_OUTPUT_DEVICE;

    char **to_be_set = NULL;

    for (int i = 1; i < argc; i++)
    {
        if (to_be_set != NULL)
        {
            *to_be_set = argv[i];
            to_be_set = NULL;
            continue;
        }

        if (argv[i][0] == '-')
        {
            if (!strcmp(argv[i], "-ao"))
                to_be_set = &output_device;
            else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-help") || !strcmp(argv[i], "-h") || !strcmp(argv[i], "-?"))
                help(0);
            else
                fprintf(stderr, "%s: Skipping unknown option \"%s\".\n", argv[0], argv[i]);
        }
        else
        {
            if (input_file == NULL)
                input_file = argv[i];
            else
                fprintf(stderr, "%s: Skipping unexpected parameter \"%s\".\n", argv[0], argv[i]);
        }
    }

    if (to_be_set != NULL)
    {
        fprintf(stderr, "%s: Parameter expected after \"%s\".\n", argv[0], argv[argc - 1]);
        return 1;
    }

    if (input_file == NULL)
    {
        fprintf(stderr, "%s: Input file required.\n", argv[0]);
        help(1);
    }


    const struct output_device *ao = find_output_device(output_device);

    if (ao == NULL)
    {
        fprintf(stderr, "%s: No such output device \"%s\".\n", argv[0], output_device);
        return 1;
    }


    FILE *ifp = fopen(input_file, "r");

    if (ifp == NULL)
    {
        fprintf(stderr, "%s: Could not open \"%s\": %s\n", argv[0], input_file, strerror(errno));
        return 1;
    }


    struct file_type *dec = get_file_type(ifp);

    if (dec == NULL)
    {
        fprintf(stderr, "%s: Unknown file type.\n", argv[0]);
        return 1;
    }


    printf("Selected output device: %s\n", ao->name);
    printf("Selected audio codec: %s\n", dec->decoder->name);
    printf("Audio: %i Hz, %i ch, %s%i%s, %.1f kbit/s\n", dec->sample_rate, dec->channels, stn1[dec->sample_type], (int)dec->sample_size * 8, stn2[dec->sample_type], dec->bitrate);

    printf("==========================================================================\n");

    if (!ao->prepare(dec->sample_rate, dec->channels, dec->sample_type, dec->sample_size))
    {
        fprintf(stderr, "AO initialization failed.\n");
        return 1;
    }

    void *buffer = malloc(BUFFER_SIZE);

    size_t now;

    do
    {
        size_t pwas = dec->position;

        fflush(stdout);

        now = dec->decoder->decode(dec, BUFFER_SIZE / (dec->sample_size * dec->channels), buffer);
        ao->play(buffer, BUFFER_SIZE / (dec->sample_size * dec->channels));

        printf("Playback: %i:%02i/%i:%02i\r", (int)(pwas / 60), (int)pwas % 60, (int)(dec->length / 60), (int)dec->length % 60);

        ao->wait_until_buffer_below(BUFFER_SIZE / (2 * dec->sample_size * dec->channels));
    }
    while (now >= (BUFFER_SIZE / (dec->sample_size * dec->channels)));

    printf("\n");

    return 0;
}

