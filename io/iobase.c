//
// Created by jan on 30.6.2023.
//

#include "iobase.h"


#ifndef _WIN32
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

static void* file_to_memory(const char* filename, u64* p_out_size, int write, int must_create)
{
    JDM_ENTER_FUNCTION;
    static long PG_SIZE = 0;
    void* ptr = NULL;
    if (!PG_SIZE)
    {
        PG_SIZE = sysconf(_SC_PAGESIZE);
        if (PG_SIZE < -1)
        {
            JDM_ERROR("sysconf did not find page size, reason: %s", strerror(errno));
            PG_SIZE = 0;
            goto end;
        }
    }

    int o_flags = 0, p_flags = 0;
    if (write)
    {
        o_flags = O_RDWR;
        p_flags = PROT_READ | PROT_WRITE;
    }
    else
    {
        o_flags = O_RDONLY;
        p_flags = PROT_READ;
    }
    if (must_create)
    {
        if (!write)
        {
            JDM_ERROR("Create flag was specified for memory file, but write access was not demanded");
            goto end;
        }
        o_flags |= O_CREAT;
    }


    int fd = -1;
    if (!must_create)
    {
        fd = open(filename, o_flags);
    }
    else
    {
        fd = open(filename, o_flags, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    }
    if (fd < 0)
    {
        JDM_ERROR("Could not open a file descriptor for file \"%s\" (perms: %s), reason: %s", filename,
                  write ? (read ? "O_RDWR" : "O_WRONLY") : (read ? "O_RDONLY" : "0"), strerror(errno));
        goto end;
    }
    u64 size;
    if (*p_out_size == 0)
    {

        struct stat fd_stats;
        if (fstat(fd, &fd_stats) < 0)
        {
            JDM_ERROR("Could not retrieve stats for fd of file \"%s\", reason: %s", filename, strerror(errno));
            close(fd);
            goto end;
        }
        size = fd_stats.st_size + 1;  //  That extra one for the sick null terminator
    }
    else
    {
        size = *p_out_size;
        if (ftruncate(fd, (off_t)size) != 0)
        {
            JDM_ERROR("Failed truncating FD to %zu bytes, reason: %s", (size_t)size, JDM_ERRNO_MESSAGE);
            close(fd);
            goto end;
        }

    }

    //  Round size up to the closest larger multiple of page size
    u64 extra = size % PG_SIZE;
    if (extra)
    {
        size += (PG_SIZE - extra);
    }
    assert(size % PG_SIZE == 0);
    ptr = mmap(NULL, size, p_flags, MAP_SHARED, fd, 0);
    close(fd);
    if (ptr == MAP_FAILED)
    {
        JDM_ERROR("Failed mapping file \"%s\" to memory (prot: %s), reason: %s", filename,
                  write ? (read ? "PROT_READ|PROT_WRITE" : "PROT_WRITE") : (read ? "PROT_READ" : "0"), strerror(errno));
        ptr = NULL;
        goto end;
    }
    *p_out_size = size;

    end:
    JDM_LEAVE_FUNCTION;
    return ptr;
}

static void file_from_memory(void* ptr, u64 size)
{
    JDM_ENTER_FUNCTION;
    int r = munmap(ptr, size);
    if (r < 0)
    {
        JDM_ERROR("Failed unmapping pointer %p from memory, reason: %s", ptr, strerror(errno));
    }
    JDM_LEAVE_FUNCTION;
}



jio_result jio_memory_file_create(
        const char* filename, jio_memory_file* p_file_out, int write, int can_create, size_t size)
{
    JDM_ENTER_FUNCTION;
    jio_result res = JIO_RESULT_SUCCESS;
    int should_create = 0;
    if (!realpath(filename, p_file_out->name))
    {
        if (!can_create)
        {
            JDM_ERROR("Could not find full path of file \"%s\", reason: %s", filename, strerror(errno));
            res = JIO_RESULT_BAD_PATH;
            goto end;
        }
        should_create = 1;
    }

    u64 real_size = size;
    void* ptr = file_to_memory(filename, &real_size, write, should_create);
    if (!ptr)
    {
        JDM_ERROR("Failed mapping file to memory");
        res = JIO_RESULT_BAD_MAP;
        goto end;
    }
    p_file_out->can_write = (write != 0);
    p_file_out->ptr = ptr;
    p_file_out->file_size = real_size;
end:
    JDM_LEAVE_FUNCTION;
    return res;
}


#else

void* file_to_memory(const char* filename, u64* p_out_size)
{
    RMOD_ENTER_FUNCTION;
    void* ptr = NULL;
    FILE* fd = fopen(filename, "r");
    if (!fd)
    {
        RMOD_ERROR_CRIT("Could not open file \"%s\", reason: %s", filename, RMOD_ERRNO_MESSAGE);
        goto end;
    }
    fseek(fd, 0, SEEK_END);
    size_t size = ftell(fd) + 1;
    ptr = jalloc(size);
    fseek(fd, 0, SEEK_SET);
    if (ptr)
    {
        size_t v = fread(ptr, 1, size, fd);
        if (ferror(fd))
        {
            RMOD_ERROR("Failed calling fread on file \"%s\", reason: %s", filename, RMOD_ERRNO_MESSAGE);
            fclose(fd);
            goto end;
        }
        *p_out_size = v;
    }
    fclose(fd);

end:
    RMOD_LEAVE_FUNCTION;
    return ptr;
}

void unmap_file(void* ptr, u64 size)
{
    RMOD_ENTER_FUNCTION;
    (void)size;
    jfree(ptr);
    RMOD_LEAVE_FUNCTION;
}
static void file_from_memory(void* ptr, u64 size)
{
    RMOD_ENTER_FUNCTION;
    jfree(ptr);
    (void)size;
    RMOD_LEAVE_FUNCTION;
}


bool map_file_is_named(const jio_memory_file* f1, const char* filename)
{
    RMOD_ENTER_FUNCTION;
    const bool res = strcmp(f1->name, filename) != 0;
    RMOD_LEAVE_FUNCTION;
    return res;
}



rmod_result rmod_map_file_to_memory(const char* filename, jio_memory_file* p_file_out)
{
    RMOD_ENTER_FUNCTION;
    rmod_result res = RMOD_RESULT_SUCCESS;
    if (!GetFullPathNameA(filename, sizeof(p_file_out->name), p_file_out->name, NULL))
    {
        RMOD_ERROR("Could not find full path of file \"%s\", reason: %s", filename, RMOD_ERRNO_MESSAGE);
        res = RMOD_RESULT_BAD_PATH;
        goto end;
    }

    u64 size;
    void* ptr = file_to_memory(filename, &size);
    if (!ptr)
    {
        RMOD_ERROR("Failed mapping file to memory");
        res = RMOD_RESULT_BAD_FILE_MAP;
        goto end;
    }

    p_file_out->ptr = ptr;
    p_file_out->file_size = size;
    end:
    RMOD_LEAVE_FUNCTION;
    return res;
}


#endif


void jio_memory_file_destroy(jio_memory_file* p_file_out)
{
    file_from_memory(p_file_out->ptr, p_file_out->file_size);
}

bool string_segment_equal(const jio_string_segment* first, const jio_string_segment* second)
{
    return first->len == second->len && (memcmp(first->begin, second->begin, first->len) == 0);
}

bool string_segment_equal_case(const jio_string_segment* first, const jio_string_segment* second)
{
    return first->len == second->len && (strncasecmp((const char*)first->begin, (const char*)second->begin, first->len) == 0);
}

bool string_segment_equal_str(const jio_string_segment* first, const char* str)
{
    const size_t len = strlen(str);
    return first->len == len && (memcmp(first->begin, str, len) == 0);
}

bool string_segment_equal_str_case(const jio_string_segment* first, const char* str)
{
    const size_t len = strlen(str);
    return first->len == len && (strncasecmp((const char*)first->begin, str, len) == 0);
}

bool iswhitespace(char32_t c)
{
    return c == ' ' || c == '\t' || c == '\n';
}

jio_result jio_memory_file_sync(const jio_memory_file* file, int sync)
{
    JDM_ENTER_FUNCTION;
    jio_result res = JIO_RESULT_SUCCESS;
    if (!file)
    {
        JDM_ERROR("Mapped file was not provided");
        res = JIO_RESULT_NULL_ARG;
        goto end;
    }

    if (msync(file->ptr, file->file_size, sync ? MS_SYNC : MS_ASYNC) != 0)
    {
        JDM_ERROR("Could not sync memory mapping of file \"%s\" (write allowed: %u), reason: %s", file->name, file->can_write, JDM_ERRNO_MESSAGE);
        res = JIO_RESULT_BAD_ACCESS;
    }

end:
    JDM_LEAVE_FUNCTION;
    return res;
}

