//
// Created by jan on 29.8.2023.
//

#ifndef JTA_FRAME_QUEUE_H
#define JTA_FRAME_QUEUE_H
#include "gfxerr.h"

struct jta_frame_job_T
{
    void (*job_callback)(void* job_param);
    void* job_param;
};
typedef struct jta_frame_job_T jta_frame_job;

struct jta_frame_work_queue_T
{
    unsigned job_capacity;
    unsigned job_count;
    jta_frame_job* jobs;
};
typedef struct jta_frame_work_queue_T jta_frame_job_queue;

gfx_result jta_frame_job_queue_create(unsigned initial_capacity, jta_frame_job_queue** p_out);

void jta_frame_job_queue_destroy(jta_frame_job_queue* queue);

void jta_frame_job_queue_execute(jta_frame_job_queue* queue);

gfx_result jta_frame_job_queue_add_job(jta_frame_job_queue* queue, void (*job_callback)(void* job_param), void* job_param);



#endif //JTA_FRAME_QUEUE_H
