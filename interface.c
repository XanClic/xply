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
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "player.h"

extern const void __start_ao, __stop_ao;
extern const void __start_dc, __stop_dc;

const struct output_device *find_output_device(const char *name)
{
    const struct output_device **ao = (const struct output_device **)&__start_ao;
    bool just_list = false;

    if (!strcmp(name, "list"))
    {
        puts("Available output devices:");
        just_list = true;
    }

    while ((uintptr_t)ao < (uintptr_t)&__stop_ao)
    {
        if (just_list)
            printf("  - %s: %s\n", (*ao)->name, (*ao)->description);
        else if (!strcmp((*ao)->name, name))
            return *ao;

        ao++;
    }

    return NULL;
}

struct file_type *get_file_type(FILE *fp)
{
    const struct decoder **dc = (const struct decoder **)&__start_dc;

    struct file_type *ft = NULL;

    while ((uintptr_t)dc < (uintptr_t)&__stop_dc)
    {
        if ((*dc)->check_file(fp, &ft))
        {
            ft->decoder = *dc;
            return ft;
        }

        dc++;
    }

    return NULL;
}

