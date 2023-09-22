//
// Created by jan on 22.9.2023.
//

#ifndef JTA_JTALOAD_H
#define JTA_JTALOAD_H
#include "jtaproblem.h"

jta_result jta_load_points_from_file(const jio_context* io_ctx, jta_problem_setup* problem, const char* filename);

jta_result jta_load_materials_from_file(const jio_context* io_ctx, jta_problem_setup* problem, const char* filename);

jta_result jta_load_profiles_from_file(const jio_context* io_ctx, jta_problem_setup* problem, const char* filename);

jta_result jta_load_natbc_from_file(const jio_context* io_ctx, jta_problem_setup* problem, const char* filename);

jta_result jta_load_numbc_from_file(const jio_context* io_ctx, jta_problem_setup* problem, const char* filename);

jta_result jta_load_elements_from_file(const jio_context* io_ctx, jta_problem_setup* problem, const char* filename);

#endif //JTA_JTALOAD_H
