//
// Created by jan on 29.8.2023.
//

#include <jdm.h>
#include "frame_queue.h"
#include "../common/common.h"

gfx_result jta_frame_job_queue_create(unsigned int initial_capacity, jta_frame_job_queue** p_out)
{
    JDM_ENTER_FUNCTION;

    jta_frame_job_queue* const this = ill_alloc(G_ALLOCATOR, sizeof(*this));
    if (!this)
    {
        JDM_ERROR("Could not allocate memory for frame queue");
        JDM_LEAVE_FUNCTION;
        return GFX_RESULT_BAD_ALLOC;
    }

    this->job_capacity = initial_capacity;
    this->job_count = 0;
    this->jobs = ill_alloc(G_ALLOCATOR, sizeof(*this->jobs) * this->job_capacity);
    if (!this->jobs)
    {
        JDM_ERROR("Could not allocate memory for frame queue's job list");
        ill_jfree(G_ALLOCATOR, this);
        JDM_LEAVE_FUNCTION;
        return GFX_RESULT_BAD_ALLOC;
    }
    memset(this->jobs, 0xCC, sizeof(*this->jobs) * (this->job_capacity));

    *p_out =  this;

    JDM_LEAVE_FUNCTION;
    return GFX_RESULT_SUCCESS;
}

void jta_frame_job_queue_destroy(jta_frame_job_queue* queue)
{
    JDM_ENTER_FUNCTION;

    if (queue->job_count)
    {
        JDM_WARN("Frame queue was destroyed with %u jobs still enqueued", queue->job_count);
    }
    ill_jfree(G_ALLOCATOR, queue->jobs);
    ill_jfree(G_ALLOCATOR, queue);

    JDM_LEAVE_FUNCTION;
}

void jta_frame_job_queue_execute(jta_frame_job_queue* queue)
{
    JDM_ENTER_FUNCTION;
    unsigned i;
    for (i = 0; i < queue->job_count; ++i)
    {
        queue->jobs[i].job_callback(queue->jobs[i].job_param);
    }
    memset(queue->jobs, 0xCC, queue->job_capacity * sizeof(*queue->jobs));
    queue->job_count = 0;

    JDM_LEAVE_FUNCTION;
}

gfx_result jta_frame_job_queue_add_job(jta_frame_job_queue* queue, void (* job_callback)(void* job_param), void* job_param)
{
    JDM_ENTER_FUNCTION;

    if (queue->job_count == queue->job_capacity)
    {
        const unsigned new_capacity = queue->job_capacity << 1;
        jta_frame_job* const new_ptr = ill_jrealloc(G_ALLOCATOR, queue->jobs, sizeof(*queue->jobs) * new_capacity);
        if (!new_ptr)
        {
            JDM_ERROR("Could not reallocate frame job queue to %zu bytes", sizeof(*queue->jobs) * new_capacity);
            JDM_LEAVE_FUNCTION;
            return GFX_RESULT_BAD_ALLOC;
        }

        memset(new_ptr + queue->job_count, 0xCC, sizeof(*new_ptr) * (new_capacity - queue->job_count));

        queue->jobs = new_ptr;
        queue->job_capacity = new_capacity;
    }

    queue->jobs[queue->job_count] = (jta_frame_job){.job_callback = job_callback, .job_param = job_param};
    queue->job_count += 1;

    JDM_LEAVE_FUNCTION;
    return GFX_RESULT_SUCCESS;
}
