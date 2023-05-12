//
// Created by jan on 17.1.2023.
//
#include "jfw_common.h"
#include "error_system/error_stack.h"
#undef jfw_malloc
#undef jfw_calloc
#undef jfw_realloc
#undef jfw_free
#include <errno.h>

jfw_res jfw_malloc(size_t size, void** pptr)
{
    assert(pptr);
    jfw_res result = jfw_res_success;
    void* ptr = malloc(size);
    if (!ptr)
    {
        JFW_ERROR("Failed call malloc(%zu), reason: %s", size, strerror(errno));
        result = jfw_res_bad_malloc;
    }
    memset(ptr, 0, size);
    *pptr = ptr;
    return result;
}

jfw_res jfw_calloc(size_t nmemb, size_t size, void** pptr)
{
    assert(pptr);
    jfw_res result = jfw_res_success;
    void* ptr = calloc(nmemb, size);
    if (!ptr)
    {
        JFW_ERROR("Failed call calloc(%zu, %zu), reason: %s", nmemb, size, strerror(errno));
        result = jfw_res_bad_malloc;
    }
    *pptr = ptr;
    return result;
}

jfw_res jfw_realloc(size_t new_size, void** ptr)
{
    assert(ptr);
    jfw_res result = jfw_res_success;
    void* new_ptr = realloc(*ptr, new_size);
    if (!new_ptr)
    {
        JFW_ERROR("Failed call realloc(%zu, %p), reason: %s", new_size, ptr, strerror(errno));
        result = jfw_res_bad_realloc;
    }
    else
    {
        *ptr = new_ptr;
    }
    return result;
}

jfw_res jfw_free(void** ptr)
{
    assert(ptr);
    free(*ptr);
    *ptr = NULL;
    return jfw_res_success;
}

int jfw_success(jfw_res res)
{
    return res == jfw_res_success;
}
