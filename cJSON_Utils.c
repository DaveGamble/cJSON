/*
  Copyright (c) 2009-2017 Dave Gamble and cJSON contributors

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#pragma GCC visibility push(default)
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#pragma GCC visibility pop

#include "cJSON_Utils.h"

static unsigned char* cJSONUtils_strdup(const unsigned char* str)
{
    size_t len = 0;
    unsigned char *copy = NULL;

    len = strlen((const char*)str) + 1;
    if (!(copy = (unsigned char*)malloc(len)))
    {
        return NULL;
    }
    memcpy(copy, str, len);

    return copy;
}

static int cJSONUtils_strcasecmp(const unsigned char *s1, const unsigned char *s2)
{
    if (!s1)
    {
        return (s1 == s2) ? 0 : 1; /* both NULL? */
    }
    if (!s2)
    {
        return 1;
    }
    for(; tolower(*s1) == tolower(*s2); (void)++s1, ++s2)
    {
        if(*s1 == 0)
        {
            return 0;
        }
    }

    return tolower(*s1) - tolower(*s2);
}

/* JSON Pointer implementation: */
static int cJSONUtils_Pstrcasecmp(const unsigned char *a, const unsigned char *e)
{
    if (!a || !e)
    {
        return (a == e) ? 0 : 1; /* both NULL? */
    }
    for (; *a && *e && (*e != '/'); (void)a++, e++) /* compare until next '/' */
    {
        if (*e == '~')
        {
            /* check for escaped '~' (~0) and '/' (~1) */
            if (!((e[1] == '0') && (*a == '~')) && !((e[1] == '1') && (*a == '/')))
            {
                /* invalid escape sequence or wrong character in *a */
                return 1;
            }
            else
            {
                e++;
            }
        }
        else if (tolower(*a) != tolower(*e))
        {
            return 1;
        }
    }
    if (((*e != 0) && (*e != '/')) != (*a != 0))
    {
        /* one string has ended, the other not */
        return 1;
    }

    return 0;
}

static size_t cJSONUtils_PointerEncodedstrlen(const unsigned char *s)
{
    size_t l = 0;
    for (; *s; (void)s++, l++)
    {
        if ((*s == '~') || (*s == '/'))
        {
            l++;
        }
    }

    return l;
}

static void cJSONUtils_PointerEncodedstrcpy(unsigned char *d, const unsigned char *s)
{
    for (; *s; s++)
    {
        if (*s == '/')
        {
            *d++ = '~';
            *d++ = '1';
        }
        else if (*s == '~')
        {
            *d++ = '~';
            *d++ = '0';
        }
        else
        {
            *d++ = *s;
        }
    }

    *d = '\0';
}

CJSON_PUBLIC(char *) cJSONUtils_FindPointerFromObjectTo(cJSON *object, cJSON *target)
{
    size_t c = 0;
    cJSON *obj = 0;

    if (object == target)
    {
        /* found */
        return (char*)cJSONUtils_strdup((const unsigned char*)"");
    }

    /* recursively search all children of the object */
    for (obj = object->child; obj; (void)(obj = obj->next), c++)
    {
        unsigned char *found = (unsigned char*)cJSONUtils_FindPointerFromObjectTo(obj, target);
        if (found)
        {
            if (cJSON_IsArray(object))
            {
                /* reserve enough memory for a 64 bit integer + '/' and '\0' */
                unsigned char *ret = (unsigned char*)malloc(strlen((char*)found) + 23);
                /* check if conversion to unsigned long is valid
                 * This should be eliminated at compile time by dead code elimination
                 * if size_t is an alias of unsigned long, or if it is bigger */
                if (c > ULONG_MAX)
                {
                    free(found);
                    return NULL;
                }
                sprintf((char*)ret, "/%lu%s", (unsigned long)c, found); /* /<array_index><path> */
                free(found);

                return (char*)ret;
            }
            else if (cJSON_IsObject(object))
            {
                unsigned char *ret = (unsigned char*)malloc(strlen((char*)found) + cJSONUtils_PointerEncodedstrlen((unsigned char*)obj->string) + 2);
                *ret = '/';
                cJSONUtils_PointerEncodedstrcpy(ret + 1, (unsigned char*)obj->string);
                strcat((char*)ret, (char*)found);
                free(found);

                return (char*)ret;
            }

            /* reached leaf of the tree, found nothing */
            free(found);
            return NULL;
        }
    }

    /* not found */
    return NULL;
}

CJSON_PUBLIC(cJSON *) cJSONUtils_GetPointer(cJSON *object, const char *pointer)
{
    /* follow path of the pointer */
    while ((*pointer++ == '/') && object)
    {
        if (cJSON_IsArray(object))
        {
            size_t which = 0;
            /* parse array index */
            while ((*pointer >= '0') && (*pointer <= '9'))
            {
                which = (10 * which) + (size_t)(*pointer++ - '0');
            }
            if (*pointer && (*pointer != '/'))
            {
                /* not end of string or new path token */
                return NULL;
            }
            if (which > INT_MAX)
            {
                return NULL;
            }
            object = cJSON_GetArrayItem(object, (int)which);
        }
        else if (cJSON_IsObject(object))
        {
            object = object->child;
            /* GetObjectItem. */
            while (object && cJSONUtils_Pstrcasecmp((unsigned char*)object->string, (const unsigned char*)pointer))
            {
                object = object->next;
            }
            /* skip to the next path token or end of string */
            while (*pointer && (*pointer != '/'))
            {
                pointer++;
            }
        }
        else
        {
            return NULL;
        }
    }

    return object;
}

/* JSON Patch implementation. */
static void cJSONUtils_InplaceDecodePointerString(unsigned char *string)
{
    unsigned char *s2 = string;

    if (string == NULL) {
        return;
    }

    for (; *string; (void)s2++, string++)
    {
        *s2 = (unsigned char) ((*string != '~')
            ? (*string)
            : ((*(++string) == '0')
                    ? '~'
                    : '/'));
    }

    *s2 = '\0';
}

static cJSON *cJSONUtils_PatchDetach(cJSON *object, const unsigned char *path)
{
    unsigned char *parentptr = NULL;
    unsigned char *childptr = NULL;
    cJSON *parent = NULL;
    cJSON *ret = NULL;

    /* copy path and split it in parent and child */
    parentptr = cJSONUtils_strdup(path);
    if (parentptr == NULL) {
        return NULL;
    }

    childptr = (unsigned char*)strrchr((char*)parentptr, '/'); /* last '/' */
    if (childptr == NULL)
    {
        free(parentptr);
        return NULL;
    }
    /* split strings */
    *childptr++ = '\0';

    parent = cJSONUtils_GetPointer(object, (char*)parentptr);
    cJSONUtils_InplaceDecodePointerString(childptr);

    if (!parent)
    {
        /* Couldn't find object to remove child from. */
        ret = NULL;
    }
    else if (cJSON_IsArray(parent))
    {
        ret = cJSON_DetachItemFromArray(parent, atoi((char*)childptr));
    }
    else if (cJSON_IsObject(parent))
    {
        ret = cJSON_DetachItemFromObject(parent, (char*)childptr);
    }
    free(parentptr);

    /* return the detachted item */
    return ret;
}

static int cJSONUtils_Compare(cJSON *a, cJSON *b)
{
    if ((a == NULL) || (b == NULL) || ((a->type & 0xFF) != (b->type & 0xFF)))
    {
        /* mismatched type. */
        return -1;
    }
    switch (a->type & 0xFF)
    {
        case cJSON_Number:
            /* numeric mismatch. */
            return ((a->valueint != b->valueint) || (a->valuedouble != b->valuedouble)) ? -2 : 0;
        case cJSON_String:
            /* string mismatch. */
            return (strcmp(a->valuestring, b->valuestring) != 0) ? -3 : 0;
        case cJSON_Array:
            for ((void)(a = a->child), b = b->child; a && b; (void)(a = a->next), b = b->next)
            {
                int err = cJSONUtils_Compare(a, b);
                if (err)
                {
                    return err;
                }
            }
            /* array size mismatch? (one of both children is not NULL) */
            return (a || b) ? -4 : 0;
        case cJSON_Object:
            cJSONUtils_SortObject(a);
            cJSONUtils_SortObject(b);
            a = a->child;
            b = b->child;
            while (a && b)
            {
                int err = 0;
                /* compare object keys */
                if (cJSONUtils_strcasecmp((unsigned char*)a->string, (unsigned char*)b->string))
                {
                    /* missing member */
                    return -6;
                }
                err = cJSONUtils_Compare(a, b);
                if (err)
                {
                    return err;
                }
                a = a->next;
                b = b->next;
            }
            /* object length mismatch (one of both children is not null) */
            return (a || b) ? -5 : 0;

        default:
            break;
    }
    /* null, true or false */
    return 0;
}

static int cJSONUtils_ApplyPatch(cJSON *object, cJSON *patch)
{
    cJSON *op = NULL;
    cJSON *path = NULL;
    cJSON *value = NULL;
    cJSON *parent = NULL;
    int opcode = 0;
    unsigned char *parentptr = NULL;
    unsigned char *childptr = NULL;

    op = cJSON_GetObjectItem(patch, "op");
    path = cJSON_GetObjectItem(patch, "path");
    if (!op || !path)
    {
        /* malformed patch. */
        return 2;
    }

    /* decode operation */
    if (!strcmp(op->valuestring, "add"))
    {
        opcode = 0;
    }
    else if (!strcmp(op->valuestring, "remove"))
    {
        opcode = 1;
    }
    else if (!strcmp(op->valuestring, "replace"))
    {
        opcode = 2;
    }
    else if (!strcmp(op->valuestring, "move"))
    {
        opcode = 3;
    }
    else if (!strcmp(op->valuestring, "copy"))
    {
        opcode = 4;
    }
    else if (!strcmp(op->valuestring, "test"))
    {
        /* compare value: {...} with the given path */
        return cJSONUtils_Compare(cJSONUtils_GetPointer(object, path->valuestring), cJSON_GetObjectItem(patch, "value"));
    }
    else
    {
        /* unknown opcode. */
        return 3;
    }

    /* Remove/Replace */
    if ((opcode == 1) || (opcode == 2))
    {
        /* Get rid of old. */
        cJSON_Delete(cJSONUtils_PatchDetach(object, (unsigned char*)path->valuestring));
        if (opcode == 1)
        {
            /* For Remove, this is job done. */
            return 0;
        }
    }

    /* Copy/Move uses "from". */
    if ((opcode == 3) || (opcode == 4))
    {
        cJSON *from = cJSON_GetObjectItem(patch, "from");
        if (!from)
        {
            /* missing "from" for copy/move. */
            return 4;
        }

        if (opcode == 3)
        {
            /* move */
            value = cJSONUtils_PatchDetach(object, (unsigned char*)from->valuestring);
        }
        if (opcode == 4)
        {
            /* copy */
            value = cJSONUtils_GetPointer(object, from->valuestring);
        }
        if (!value)
        {
            /* missing "from" for copy/move. */
            return 5;
        }
        if (opcode == 4)
        {
            value = cJSON_Duplicate(value, 1);
        }
        if (!value)
        {
            /* out of memory for copy/move. */
            return 6;
        }
    }
    else /* Add/Replace uses "value". */
    {
        value = cJSON_GetObjectItem(patch, "value");
        if (!value)
        {
            /* missing "value" for add/replace. */
            return 7;
        }
        value = cJSON_Duplicate(value, 1);
        if (!value)
        {
            /* out of memory for add/replace. */
            return 8;
        }
    }

    /* Now, just add "value" to "path". */

    /* split pointer in parent and child */
    parentptr = cJSONUtils_strdup((unsigned char*)path->valuestring);
    childptr = (unsigned char*)strrchr((char*)parentptr, '/');
    if (childptr)
    {
        *childptr++ = '\0';
    }
    parent = cJSONUtils_GetPointer(object, (char*)parentptr);
    cJSONUtils_InplaceDecodePointerString(childptr);

    /* add, remove, replace, move, copy, test. */
    if (!parent)
    {
        /* Couldn't find object to add to. */
        free(parentptr);
        cJSON_Delete(value);
        return 9;
    }
    else if (cJSON_IsArray(parent))
    {
        if (!strcmp((char*)childptr, "-"))
        {
            cJSON_AddItemToArray(parent, value);
        }
        else
        {
            cJSON_InsertItemInArray(parent, atoi((char*)childptr), value);
        }
    }
    else if (cJSON_IsObject(parent))
    {
        cJSON_DeleteItemFromObject(parent, (char*)childptr);
        cJSON_AddItemToObject(parent, (char*)childptr, value);
    }
    else
    {
        cJSON_Delete(value);
    }
    free(parentptr);

    return 0;
}

CJSON_PUBLIC(int) cJSONUtils_ApplyPatches(cJSON *object, cJSON *patches)
{
    int err = 0;

    if (patches == NULL)
    {
        return 1;
    }

    if (cJSON_IsArray(patches))
    {
        /* malformed patches. */
        return 1;
    }
    if (patches)
    {
        patches = patches->child;
    }
    while (patches)
    {
        if ((err = cJSONUtils_ApplyPatch(object, patches)))
        {
            return err;
        }
        patches = patches->next;
    }

    return 0;
}

static void cJSONUtils_GeneratePatch(cJSON *patches, const unsigned char *op, const unsigned char *path, const unsigned char *suffix, cJSON *val)
{
    cJSON *patch = cJSON_CreateObject();
    cJSON_AddItemToObject(patch, "op", cJSON_CreateString((const char*)op));
    if (suffix)
    {
        unsigned char *newpath = (unsigned char*)malloc(strlen((const char*)path) + cJSONUtils_PointerEncodedstrlen(suffix) + 2);
        cJSONUtils_PointerEncodedstrcpy(newpath + sprintf((char*)newpath, "%s/", (const char*)path), suffix);
        cJSON_AddItemToObject(patch, "path", cJSON_CreateString((const char*)newpath));
        free(newpath);
    }
    else
    {
        cJSON_AddItemToObject(patch, "path", cJSON_CreateString((const char*)path));
    }
    if (val)
    {
        cJSON_AddItemToObject(patch, "value", cJSON_Duplicate(val, 1));
    }
    cJSON_AddItemToArray(patches, patch);
}

CJSON_PUBLIC(void) cJSONUtils_AddPatchToArray(cJSON *array, const char *op, const char *path, cJSON *val)
{
    cJSONUtils_GeneratePatch(array, (const unsigned char*)op, (const unsigned char*)path, 0, val);
}

static void cJSONUtils_CompareToPatch(cJSON *patches, const unsigned char *path, cJSON *from, cJSON *to)
{
    if ((from == NULL) || (to == NULL))
    {
        return;
    }

    if ((from->type & 0xFF) != (to->type & 0xFF))
    {
        cJSONUtils_GeneratePatch(patches, (const unsigned char*)"replace", path, 0, to);
        return;
    }

    switch ((from->type & 0xFF))
    {
        case cJSON_Number:
            if ((from->valueint != to->valueint) || (from->valuedouble != to->valuedouble))
            {
                cJSONUtils_GeneratePatch(patches, (const unsigned char*)"replace", path, 0, to);
            }
            return;

        case cJSON_String:
            if (strcmp(from->valuestring, to->valuestring) != 0)
            {
                cJSONUtils_GeneratePatch(patches, (const unsigned char*)"replace", path, 0, to);
            }
            return;

        case cJSON_Array:
        {
            size_t c = 0;
            unsigned char *newpath = (unsigned char*)malloc(strlen((const char*)path) + 23); /* Allow space for 64bit int. */
            /* generate patches for all array elements that exist in "from" and "to" */
            for ((void)(c = 0), (void)(from = from->child), to = to->child; from && to; (void)(from = from->next), (void)(to = to->next), c++)
            {
                /* check if conversion to unsigned long is valid
                 * This should be eliminated at compile time by dead code elimination
                 * if size_t is an alias of unsigned long, or if it is bigger */
                if (c > ULONG_MAX)
                {
                    free(newpath);
                    return;
                }
                sprintf((char*)newpath, "%s/%lu", path, (unsigned long)c); /* path of the current array element */
                cJSONUtils_CompareToPatch(patches, newpath, from, to);
            }
            /* remove leftover elements from 'from' that are not in 'to' */
            for (; from; (void)(from = from->next), c++)
            {
                /* check if conversion to unsigned long is valid
                 * This should be eliminated at compile time by dead code elimination
                 * if size_t is an alias of unsigned long, or if it is bigger */
                if (c > ULONG_MAX)
                {
                    free(newpath);
                    return;
                }
                sprintf((char*)newpath, "%lu", (unsigned long)c);
                cJSONUtils_GeneratePatch(patches, (const unsigned char*)"remove", path, newpath, 0);
            }
            /* add new elements in 'to' that were not in 'from' */
            for (; to; (void)(to = to->next), c++)
            {
                cJSONUtils_GeneratePatch(patches, (const unsigned char*)"add", path, (const unsigned char*)"-", to);
            }
            free(newpath);
            return;
        }

        case cJSON_Object:
        {
            cJSON *a = NULL;
            cJSON *b = NULL;
            cJSONUtils_SortObject(from);
            cJSONUtils_SortObject(to);

            a = from->child;
            b = to->child;
            /* for all object values in the object with more of them */
            while (a || b)
            {
                int diff = (!a) ? 1 : ((!b) ? -1 : cJSONUtils_strcasecmp((unsigned char*)a->string, (unsigned char*)b->string));
                if (!diff)
                {
                    /* both object keys are the same */
                    unsigned char *newpath = (unsigned char*)malloc(strlen((const char*)path) + cJSONUtils_PointerEncodedstrlen((unsigned char*)a->string) + 2);
                    cJSONUtils_PointerEncodedstrcpy(newpath + sprintf((char*)newpath, "%s/", path), (unsigned char*)a->string);
                    /* create a patch for the element */
                    cJSONUtils_CompareToPatch(patches, newpath, a, b);
                    free(newpath);
                    a = a->next;
                    b = b->next;
                }
                else if (diff < 0)
                {
                    /* object element doesn't exist in 'to' --> remove it */
                    cJSONUtils_GeneratePatch(patches, (const unsigned char*)"remove", path, (unsigned char*)a->string, 0);
                    a = a->next;
                }
                else
                {
                    /* object element doesn't exist in 'from' --> add it */
                    cJSONUtils_GeneratePatch(patches, (const unsigned char*)"add", path, (unsigned char*)b->string, b);
                    b = b->next;
                }
            }
            return;
        }

        default:
            break;
    }
}

CJSON_PUBLIC(cJSON *) cJSONUtils_GeneratePatches(cJSON *from, cJSON *to)
{
    cJSON *patches = cJSON_CreateArray();
    cJSONUtils_CompareToPatch(patches, (const unsigned char*)"", from, to);

    return patches;
}

/* sort lists using mergesort */
static cJSON *cJSONUtils_SortList(cJSON *list)
{
    cJSON *first = list;
    cJSON *second = list;
    cJSON *ptr = list;

    if (!list || !list->next)
    {
        /* One entry is sorted already. */
        return list;
    }

    while (ptr && ptr->next && (cJSONUtils_strcasecmp((unsigned char*)ptr->string, (unsigned char*)ptr->next->string) < 0))
    {
        /* Test for list sorted. */
        ptr = ptr->next;
    }
    if (!ptr || !ptr->next)
    {
        /* Leave sorted lists unmodified. */
        return list;
    }

    /* reset ptr to the beginning */
    ptr = list;
    while (ptr)
    {
        /* Walk two pointers to find the middle. */
        second = second->next;
        ptr = ptr->next;
        /* advances ptr two steps at a time */
        if (ptr)
        {
            ptr = ptr->next;
        }
    }
    if (second && second->prev)
    {
        /* Split the lists */
        second->prev->next = NULL;
    }

    /* Recursively sort the sub-lists. */
    first = cJSONUtils_SortList(first);
    second = cJSONUtils_SortList(second);
    list = ptr = NULL;

    while (first && second) /* Merge the sub-lists */
    {
        if (cJSONUtils_strcasecmp((unsigned char*)first->string, (unsigned char*)second->string) < 0)
        {
            if (!list)
            {
                /* start merged list with the first element of the first list */
                list = ptr = first;
            }
            else
            {
                /* add first element of first list to merged list */
                ptr->next = first;
                first->prev = ptr;
                ptr = first;
            }
            first = first->next;
        }
        else
        {
            if (!list)
            {
                /* start merged list with the first element of the second list */
                list = ptr = second;
            }
            else
            {
                /* add first element of second list to merged list */
                ptr->next = second;
                second->prev = ptr;
                ptr = second;
            }
            second = second->next;
        }
    }
    if (first)
    {
        /* Append rest of first list. */
        if (!list)
        {
            return first;
        }
        ptr->next = first;
        first->prev = ptr;
    }
    if (second)
    {
        /* Append rest of second list */
        if (!list)
        {
            return second;
        }
        ptr->next = second;
        second->prev = ptr;
    }

    return list;
}

CJSON_PUBLIC(void) cJSONUtils_SortObject(cJSON *object)
{
    object->child = cJSONUtils_SortList(object->child);
}

CJSON_PUBLIC(cJSON *) cJSONUtils_MergePatch(cJSON *target, cJSON *patch)
{
    if (!cJSON_IsObject(patch))
    {
        /* scalar value, array or NULL, just duplicate */
        cJSON_Delete(target);
        return cJSON_Duplicate(patch, 1);
    }

    if (!cJSON_IsObject(target))
    {
        cJSON_Delete(target);
        target = cJSON_CreateObject();
    }

    patch = patch->child;
    while (patch)
    {
        if (cJSON_IsNull(patch))
        {
            /* NULL is the indicator to remove a value, see RFC7396 */
            cJSON_DeleteItemFromObject(target, patch->string);
        }
        else
        {
            cJSON *replaceme = cJSON_DetachItemFromObject(target, patch->string);
            cJSON_AddItemToObject(target, patch->string, cJSONUtils_MergePatch(replaceme, patch));
        }
        patch = patch->next;
    }
    return target;
}

CJSON_PUBLIC(cJSON *) cJSONUtils_GenerateMergePatch(cJSON *from, cJSON *to)
{
    cJSON *patch = NULL;
    if (!to)
    {
        /* patch to delete everything */
        return cJSON_CreateNull();
    }
    if (!cJSON_IsObject(to) || !cJSON_IsObject(from))
    {
        return cJSON_Duplicate(to, 1);
    }

    cJSONUtils_SortObject(from);
    cJSONUtils_SortObject(to);

    from = from->child;
    to = to->child;
    patch = cJSON_CreateObject();
    while (from || to)
    {
        int compare = from ? (to ? strcmp(from->string, to->string) : -1) : 1;
        if (compare < 0)
        {
            /* from has a value that to doesn't have -> remove */
            cJSON_AddItemToObject(patch, from->string, cJSON_CreateNull());
            from = from->next;
        }
        else if (compare > 0)
        {
            /* to has a value that from doesn't have -> add to patch */
            cJSON_AddItemToObject(patch, to->string, cJSON_Duplicate(to, 1));
            to = to->next;
        }
        else
        {
            /* object key exists in both objects */
            if (cJSONUtils_Compare(from, to))
            {
                /* not identical --> generate a patch */
                cJSON_AddItemToObject(patch, to->string, cJSONUtils_GenerateMergePatch(from, to));
            }
            /* next key in the object */
            from = from->next;
            to = to->next;
        }
    }
    if (!patch->child)
    {
        cJSON_Delete(patch);
        return NULL;
    }

    return patch;
}
