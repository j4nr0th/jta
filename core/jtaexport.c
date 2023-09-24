//
// Created by jan on 24.9.2023.
//

#include <jdm.h>
#include "jtaexport.h"

static const char* const POINT_COLUMN_LABELS[] =
        {
        "Label",
        "u_x",
        "u_y",
        "u_z",
        "F_x",
        "F_y",
        "F_z",
        "m"
        };

static size_t POINT_COLUMN_COUNT = sizeof(POINT_COLUMN_LABELS) / sizeof(*POINT_COLUMN_LABELS);

jta_result jta_save_point_solution(const char* filename, const jta_point_list* points, const jta_solution* solution)
{
    JDM_ENTER_FUNCTION;
    FILE* const f_out = fopen(filename, "w");
    if (!f_out)
    {
        JDM_ERROR("Could not open file \"%s\" for point output, reason: %s", filename, strerror(errno));
        JDM_LEAVE_FUNCTION;
        return JTA_RESULT_BAD_IO;
    }

    fprintf(f_out, "%s, %s, %s, %s, %s, %s, %s, %s\n",
            POINT_COLUMN_LABELS[0],
            POINT_COLUMN_LABELS[1],
            POINT_COLUMN_LABELS[2],
            POINT_COLUMN_LABELS[3],
            POINT_COLUMN_LABELS[4],
            POINT_COLUMN_LABELS[5],
            POINT_COLUMN_LABELS[6],
            POINT_COLUMN_LABELS[7]);
    for (unsigned i = 0; i < points->count; ++i)
    {
        unsigned pt_idx = 3u * i;
        fprintf(f_out, "%.*s, %g, %g, %g, %g, %g, %g, %g\n", (int)points->label[i].len, points->label[i].begin,
                solution->point_displacements[pt_idx + 0ul],
                solution->point_displacements[pt_idx + 1ul],
                solution->point_displacements[pt_idx + 2ul],
                solution->point_reactions[pt_idx + 0ul],
                solution->point_reactions[pt_idx + 1ul],
                solution->point_reactions[pt_idx + 2ul],
                solution->point_masses[pt_idx]);
    }
    
    fclose(f_out);
    JDM_LEAVE_FUNCTION;
    return JTA_RESULT_SUCCESS;
}

static const char* const ELEMENT_COLUMN_LABELS[] =
        {
                "Label",
                "Node 0",
                "Node 1",
                "Stress",
                "Force",
                "Mass",
        };

static size_t ELEMENT_COLUMN_COUNT = sizeof(ELEMENT_COLUMN_LABELS) / sizeof(*ELEMENT_COLUMN_LABELS);
jta_result jta_save_element_solution(
        const char* filename, const jta_element_list* elements, const jta_point_list* points, const jta_solution* solution)
{
    JDM_ENTER_FUNCTION;
    if (elements->count != solution->element_count)
    {
        JDM_ERROR("Solution was not (successfully) post-processed");
        JDM_LEAVE_FUNCTION;
        return JTA_RESULT_BAD_PROBLEM;
    }

    FILE* const f_out = fopen(filename, "w");
    if (!f_out)
    {
        JDM_ERROR("Could not open file \"%s\" for element output, reason: %s", filename, strerror(errno));
        JDM_LEAVE_FUNCTION;
        return JTA_RESULT_BAD_IO;
    }

    fprintf(f_out, "%s, %s, %s, %s, %s, %s\n",
            ELEMENT_COLUMN_LABELS[0],
            ELEMENT_COLUMN_LABELS[1],
            ELEMENT_COLUMN_LABELS[2],
            ELEMENT_COLUMN_LABELS[3],
            ELEMENT_COLUMN_LABELS[4],
            ELEMENT_COLUMN_LABELS[5]);
    for (unsigned i = 0; i < elements->count; ++i)
    {
        const unsigned i_pt0 = elements->i_point0[i];
        const unsigned i_pt1 = elements->i_point1[i];
        fprintf(f_out, "%.*s, %.*s, %.*s, %g, %g, %g\n", (int)elements->labels[i].len, elements->labels[i].begin,
                (int)points->label[i_pt0].len, points->label[i_pt0].begin,
                (int)points->label[i_pt1].len, points->label[i_pt1].begin,
                solution->element_stresses[i],
                solution->element_forces[i],
                solution->element_masses[i]
                );
    }

    fclose(f_out);
    JDM_LEAVE_FUNCTION;
    return JTA_RESULT_SUCCESS;
}
