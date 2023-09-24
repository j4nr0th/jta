//
// Created by jan on 24.9.2023.
//

#ifndef JTA_JTAEXPORT_H
#define JTA_JTAEXPORT_H
#include "jtasolve.h"

jta_result jta_save_point_solution(const char* filename, const jta_point_list* points, const jta_solution* solution);

jta_result jta_save_element_solution(
        const char* filename, const jta_element_list* elements, const jta_point_list* points, const jta_solution* solution);



#endif //JTA_JTAEXPORT_H
