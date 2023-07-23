//
// Created by jan on 17.1.2023.
//
#include "jfw_common.h"
#include <jdm.h>
#undef jfw_malloc
#undef jfw_calloc
#undef jfw_realloc
#undef jfw_free
#include <errno.h>

jfw_result jfw_malloc(size_t size, void** pptr)
{
    assert(pptr);
    jfw_result result = JFW_RESULT_SUCCESS;
    void* ptr = malloc(size);
    if (!ptr)
    {
        JDM_ERROR("Failed call malloc(%zu), reason: %s", size, strerror(errno));
        result = JFW_RESULT_BAD_MALLOC;
    }
    memset(ptr, 0, size);
    *pptr = ptr;
    return result;
}

jfw_result jfw_calloc(size_t nmemb, size_t size, void** pptr)
{
    assert(pptr);
    jfw_result result = JFW_RESULT_SUCCESS;
    void* ptr = calloc(nmemb, size);
    if (!ptr)
    {
        JDM_ERROR("Failed call calloc(%zu, %zu), reason: %s", nmemb, size, strerror(errno));
        result = JFW_RESULT_BAD_MALLOC;
    }
    *pptr = ptr;
    return result;
}

jfw_result jfw_realloc(size_t new_size, void** ptr)
{
    assert(ptr);
    jfw_result result = JFW_RESULT_SUCCESS;
    void* new_ptr = realloc(*ptr, new_size);
    if (!new_ptr)
    {
        JDM_ERROR("Failed call realloc(%zu, %p), reason: %s", new_size, ptr, strerror(errno));
        result = JFW_RESULT_BAD_REALLOC;
    }
    else
    {
        *ptr = new_ptr;
    }
    return result;
}

jfw_result jfw_free(void** ptr)
{
    assert(ptr);
    free(*ptr);
    *ptr = NULL;
    return JFW_RESULT_SUCCESS;
}

