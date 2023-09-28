//
// Created by jan on 23.7.2023.
//

#ifndef JTA_JTASOLVE_H
#define JTA_JTASOLVE_H
#include "jtaproblem.h"
#include "../config/config_loading.h"

struct jta_solution_T
{
    //  Result information
    float final_residual_ratio; // measure of solution accuracy expressed as
                                // \f$ \epsilon = \frac{|| \Vec{F} - \mathbf{K} \Vec{u}||}{|| \Vec{u} ||} \f$

    //  Point results
    uint32_t point_count;
    float* point_displacements;
    float* point_reactions;
    float* point_masses;

    //  Element results
    uint32_t element_count;
    float* element_forces;
    float* element_stresses;
    float* element_masses;
};

typedef struct jta_solution_T jta_solution;

jta_result jta_solve_problem(
        const jta_config_problem* cfg, const jta_problem_setup* problem, jta_solution* solution, vec4 gravity);

jta_result jta_postprocess(const jta_problem_setup* problem, jta_solution* solution);

void jta_solution_free(jta_solution* solution);

#endif //JTA_JTASOLVE_H
