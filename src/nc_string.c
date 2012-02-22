/*
 * twemproxy - A fast and lightweight proxy for memcached protocol.
 * Copyright (C) 2011 Twitter, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <string.h>

#include <nc_core.h>

/*
 * String (struct string) is a sequence of unsigned char objects terminated
 * by the null character '\0'. The length of the string is pre-computed and
 * made available explicitly as an additional field. This means that we don't
 * have to walk the entire character sequence until the null terminating
 * character everytime that the length of the String is requested
 *
 * The only way to create a String is to initialize it using, string_init()
 * and duplicate an existing String - string_duplicate() or copy an existing
 * raw sequence of character bytes - string_copy(). Such String's must be
 * freed using string_deinit()
 *
 * We can also create String as reference to raw string - string_set_raw()
 * or to text string - string_set_text() or string(). Such String don't have
 * to be freed.
 */

void
string_init(struct string *str)
{
    str->len = 0;
    str->data = NULL;
}

void
string_deinit(struct string *str)
{
    ASSERT((str->len == 0 && str->data == NULL) ||
           (str->len != 0 && str->data != NULL));

    if (str->data != NULL) {
        nc_free(str->data);
        string_init(str);
    }
}

bool
string_empty(const struct string *str)
{
    ASSERT((str->len == 0 && str->data == NULL) ||
           (str->len != 0 && str->data != NULL));
    return str->len == 0 ? true : false;
}

rstatus_t
string_duplicate(struct string *dst, const struct string *src)
{
    ASSERT(dst->len == 0 && dst->data == NULL);
    ASSERT(src->len != 0 && src->data != NULL);

    dst->data = nc_strndup(src->data, src->len + 1);
    if (dst->data == NULL) {
        return NC_ENOMEM;
    }

    dst->len = src->len;
    dst->data[dst->len] = '\0';

    return NC_OK;
}

rstatus_t
string_copy(struct string *dst, const uint8_t *src, uint32_t srclen)
{
    ASSERT(dst->len == 0 && dst->data == NULL);
    ASSERT(src != NULL && srclen != 0);

    dst->data = nc_strndup(src, srclen + 1);
    if (dst->data == NULL) {
        return NC_ENOMEM;
    }

    dst->len = srclen;
    dst->data[dst->len] = '\0';

    return NC_OK;
}

int
string_compare(const struct string *s1, const struct string *s2)
{
    if (s1->len != s2->len) {
        return s1->len - s2->len > 0 ? 1 : -1;
    }

    return nc_strncmp(s1->data, s2->data, s1->len);
}
