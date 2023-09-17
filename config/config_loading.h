//
// Created by jan on 23.7.2023.
//

#ifndef JTA_CONFIG_LOADING_H
#define JTA_CONFIG_LOADING_H
#include "../common/common.h"
#include "../core/jtaerr.h"
#include <jio/iobase.h>


//  Parameters related to actually solving the equations
struct jta_config_problem_T
{
    //  Definition of the problem geometry
    struct
    {
        char* points_file;          //  File to load the point information from
        char* materials_file;       //  File to load material information from
        char* profiles_file;        //  File to load profile information from
        char* elements_file;        //  File to load element information from
        char* natural_bcs_file;     //  File to load natural BC information from
        char* numerical_bcs_file;   //  File to load numerical BC information from
    } definition;

    //  Parameters for the solver and simulation
    struct
    {
        float gravity[3];               //  Gravity vector
        unsigned thrd_count;            //  Threads to use for the solver (optional)
        float convergence_criterion;    //  Convergence criterion for the solver
        unsigned max_iterations;        //  Number of maximum iterations for the solver
        float relaxation_factor;        //  The relaxation factor for the solver (recommended to not exceed 0.67)
    } sim_and_sol;
};

typedef struct jta_config_problem_T jta_config_problem;


//  Parameters related to graphics and display
struct jta_config_display_T
{
    float radius_scale;             //  Scale by which the (equivalent) radius of elements is exaggerated
    float deformation_scale;        //  Scale by which point_displacements are exaggerated
    jta_color deformed_color;       //  Color of the deformed mesh
    char* material_cmap_file;       //  Path to the material color map (optional)
    jta_color dof_point_colors[4];  //  Colors for points with 0 DoFs, 1 DoF, 2 DoFs, and 3/all DoFs
    float dof_point_scales[4];      //  Scale by which to increase the point size compared to the radius of the largest element connected to it based on the DoFs of the point
    float force_radius_ratio;       //  Radius of the force arrows divided by the maximum node radius
    float force_head_ratio;         //  Ratio between the force arrows' head and tail
    float force_length_ratio;       //  Length of the largest force arrow as a fraction of the length of the longest element
    char* stress_cmap_file;         //  Path to the stress color map (optional)
};

typedef struct jta_config_display_T jta_config_display;


//  Parameters related to output and saving of results
struct jta_config_output_T
{
    char* point_output_file;    //  Path to where point outputs are saved to (optional)
    char* element_output_file;  //  Path to where element outputs are saved to (optional)
    char* general_output_file;  //  Path to where general outputs are saved to (optional)
    char* matrix_output_file;   //  Path to where matrices are saved to (optional)
    char* figure_output_file;   //  Path to where figures are saved to (optional)
};

typedef struct jta_config_output_T jta_config_output;


struct jta_config_T
{
    jta_config_problem  problem;    //  Parameters related to actually solving the equations
    jta_config_display  display;    //  Parameters related to graphics and display
    jta_config_output   output;     //  Parameters related to output and saving of results
};

typedef struct jta_config_T jta_config;

jta_result jta_load_configuration(const jio_context* io_ctx, const char* filename, jta_config* p_out);

jta_result jta_free_configuration(jta_config* cfg);

#endif //JTA_CONFIG_LOADING_H
