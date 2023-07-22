//
// Created by jan on 21.5.2023.
//

#include <X11/keysym.h>
#include <inttypes.h>
#include "ui.h"
#include <solvers/jacobi_point_iteration.h>
#include <solvers/bicgstab_iteration.h>

jfw_res truss_mouse_button_press(jfw_widget* this, i32 x, i32 y, u32 button, u32 mods)
{
    jta_draw_state* const state = jfw_widget_get_user_pointer(this);
    assert(state);
    switch (button)
    {
    case Button3:
        //  press was with rmb
        state->track_move = 1;
        state->mv_x = x; state->mv_y = y;
        break;
    case Button2:
        //  press was with mmb
        state->track_turn = 1;
        state->mv_x = x; state->mv_y = y;
        break;

    case Button4:
        //  Scroll up
        jta_camera_zoom(&state->camera, +0.05f);
        state->vulkan_state->view = jta_camera_to_view_matrix(&state->camera);
        jfw_widget_ask_for_redraw(this);
        break;
    case Button5:
        //  Scroll down
        jta_camera_zoom(&state->camera, -0.05f);
        state->vulkan_state->view = jta_camera_to_view_matrix(&state->camera);
        jfw_widget_ask_for_redraw(this);
        break;

    default:break;
    }

    return jfw_res_success;
}

jfw_res truss_mouse_button_release(jfw_widget* this, i32 x, i32 y, u32 button, u32 mods)
{
    jta_draw_state* const state = jfw_widget_get_user_pointer(this);
    switch (button)
    {
    case Button2:
        assert(state);
        state->track_turn = 0;
        break;
    case Button3:
        assert(state);
        state->track_move = 0;
        break;
    default:break;
    }
    state->mv_x = x;
    state->mv_y = y;
    return jfw_res_success;
}

jfw_res truss_mouse_motion(jfw_widget* const this, i32 x, i32 y, const u32 mods)
{
    jta_draw_state* const state = jfw_widget_get_user_pointer(this);
    assert(state);
    if (mods & Button2Mask && state && state->track_turn)
    {
        //  Clamp x and y to intervals [0, w) and [0, h)
        if (x < 0)
        {
            x = 0;
        }
        else if (x > (i32)this->width)
        {
            x = (i32)this->width - 1;
        }
        if (y < 0)
        {
            y = 0;
        }
        else if (y > (i32)this->height)
        {
            y = (i32)this->height - 1;
        }

        const f32 w = (f32)this->width, h = (f32)this->height;
        //  Update camera
        jta_camera_3d* const camera = &state->camera;
        const i32 old_x = state->mv_x, old_y = state->mv_y;
        if (old_x == x && old_y == y)
        {
            //  No rotation
            goto end;
        }
        //  Sphere radius does need to be known, since it is known that it is equal to sqrt(2) * width

        //  Compute proper coordinates
        f32 r1x = M_SQRT2 * ((f32)old_x / w - 0.5f);
        f32 r1y = M_SQRT2 * ((f32)old_y / w - h / (2.0f * w));
        f32 r1z = sqrtf(1 - hypotf(r1x, r1y));

        f32 r2x = M_SQRT2 * ((f32)x / w - 0.5f);
        f32 r2y = M_SQRT2 * ((f32)y / w - h / (2.0f * w));
        f32 r2z = sqrtf(1 - hypotf(r2x, r2y));

        vec4 r1 = VEC4(r1x, r1y, r1z);
        vec4 r2 = VEC4(r2x, r2y, r2z);

        vec4 rotation_axis = vec4_cross(r1, r2);
        assert(vec4_magnitude(rotation_axis) <= 1.0f);
        f32 rotation_angle = asinf(vec4_magnitude(rotation_axis));

        vec4 real_axis = VEC4(0, 0, 0);
//        real_axis = vec4_add(real_axis, vec4_mul_one(camera->ux, rotation_axis.x));
        real_axis = vec4_mul_one(camera->ux, rotation_axis.x);
        real_axis = vec4_add(real_axis, vec4_mul_one(camera->uy, rotation_axis.y));
        real_axis = vec4_add(real_axis, vec4_mul_one(camera->uz, rotation_axis.z));
        jta_camera_rotate(camera, real_axis, rotation_angle);
//        const vec4 u = vec4_unit(real_axis);
//        printf("Axis of rotation (%g, %g, %g) by %6.2f degrees\n", u.x, u.y, u.z, 180.0 * rotation_angle);

        state->mv_x = x;
        state->mv_y = y;

        state->vulkan_state->view = jta_camera_to_view_matrix(camera);
        jfw_widget_ask_for_redraw(this);
    }
    if (mods & Button3Mask && state && state->track_move)
    {
        //  Clamp x and y to intervals [0, w) and [0, h)
        if (x < 0)
        {
            x = 0;
        }
        else if (x > (i32)this->width)
        {
            x = (i32)this->width - 1;
        }
        if (y < 0)
        {
            y = 0;
        }
        else if (y > (i32)this->height)
        {
            y = (i32)this->height - 1;
        }

        const f32 w = (f32)this->width, h = (f32)this->height;
        //  Update camera
        jta_camera_3d* const camera = &state->camera;
        const i32 old_x = state->mv_x, old_y = state->mv_y;
        if (old_x == x && old_y == y)
        {
            //  No movement
            goto end;
        }

        //  Compute proper coordinates
        f32 r1x = ((f32)old_x / w - 0.5f);
        f32 r1y = ((f32)old_y / w - h / (2.0f * w));

        f32 r2x = ((f32)x / w - 0.5f);
        f32 r2y = ((f32)y / w - h / (2.0f * w));

        vec4 r1 = VEC4(r1x, r1y, 0);
        vec4 r2 = VEC4(r2x, r2y, 0);

        vec4 move = vec4_sub(r1, r2);
        f32 move_mag = vec4_magnitude(vec4_sub(camera->target , camera->position));

        vec4 real_axis = VEC4(0, 0, 0);
//        real_axis = vec4_add(real_axis, vec4_mul_one(camera->ux, rotation_axis.x));
        real_axis = vec4_mul_one(camera->ux, move.x);
        real_axis = vec4_add(real_axis, vec4_mul_one(camera->uy, move.y));
        jta_camera_move(camera, vec4_mul_one(real_axis, move_mag));

        state->vulkan_state->view = jta_camera_to_view_matrix(camera);
        jfw_widget_ask_for_redraw(this);
    }
    assert(state);
    state->mv_x = x;
    state->mv_y = y;
end:
    return jfw_res_success;
}

jfw_res truss_key_press(jfw_widget* this, KeySym key_sym)
{
    JDM_ENTER_FUNCTION;

    if (key_sym == XK_F12)
    {
        //  Take a screenshot
        jfw_widget_ask_for_redraw(this);
        jta_draw_state* const state = jfw_widget_get_user_pointer(this);
        state->screenshot = 1;
    }
    else if (key_sym == XK_space)
    {
        static int solved = 0;
        if (!solved)
        {
            jta_draw_state* const state = jfw_widget_get_user_pointer(this);
            assert(state);
            jta_problem_setup_data* problem = &state->problem;
            jta_result res = jta_make_global_matrices(
                    problem->point_list, problem->element_list, problem->profile_list, problem->materials, problem->stiffness_matrix, problem->point_masses);
            if (res != JTA_RESULT_SUCCESS)
            {
                JDM_ERROR("Could not build global system matrices, reason: %s", jta_result_to_str(res));
                goto end;
            }

            res = jta_apply_natural_bcs(problem->point_list->count, problem->natural_bcs, problem->gravity, problem->point_masses, problem->forces);
            if (res != JTA_RESULT_SUCCESS)
            {
                JDM_ERROR("Could not apply natural BCs, reason: %s", jta_result_to_str(res));
                goto end;
            }
            jmtx_matrix_crs* k_reduced;
            bool* dofs = lin_jalloc(G_LIN_JALLOCATOR, sizeof(*dofs) * 3 * problem->point_list->count);
            if (!dofs)
            {
                JDM_ERROR("Could not allocate %zu bytes for the list of free DOFs", sizeof(*dofs) * 3 * problem->point_list->count);
                goto end;
            }
            float* f_r = lin_jalloc(G_LIN_JALLOCATOR, sizeof(*f_r) * 3 * problem->point_list->count);
            if (!f_r)
            {
                JDM_ERROR("Could not allocate %zu bytes for reduced force vector", sizeof(*f_r) * 3 * problem->point_list->count);
                lin_jfree(G_LIN_JALLOCATOR, dofs);
                goto end;
            }
            float* u_r = lin_jalloc(G_LIN_JALLOCATOR, sizeof(*u_r) * 3 * problem->point_list->count * 3);
            if (!u_r)
            {
                JDM_ERROR("Could not allocate %zu bytes for reduced force vector", sizeof(*f_r) * 3 * problem->point_list->count);
                lin_jfree(G_LIN_JALLOCATOR, dofs);
                lin_jfree(G_LIN_JALLOCATOR, f_r);
                goto end;
            }


            res = jta_reduce_system(problem->point_list->count, problem->stiffness_matrix, problem->numerical_bcs, &k_reduced, dofs);
            if (res != JTA_RESULT_SUCCESS)
            {
                JDM_ERROR("Could not apply reduce the system of equations to solve, reason: %s", jta_result_to_str(res));
                lin_jfree(G_LIN_JALLOCATOR, dofs);
                lin_jfree(G_LIN_JALLOCATOR, f_r);
                lin_jfree(G_LIN_JALLOCATOR, u_r);
                goto end;
            }

            uint32_t p_g, p_r;
            for (p_g = 0, p_r = 0; p_g < problem->point_list->count * 3; ++p_g)
            {
                if (dofs[p_g])
                {
                    f_r[p_r] = problem->forces[p_g];
                    p_r += 1;
                }
            }

            uint32_t iter_count;
            const uint32_t max_iters = 1 << 16;    //  This could be config parameter
            float max_err = 1e-7f;              //  This could be config parameter
            float err_evol[max_iters];
            float final_error;
            jta_timer solver_timer;
            jta_timer_set(&solver_timer);
            jmtx_result jmtx_res = jmtx_jacobi_relaxed_crs(k_reduced, f_r, u_r,
                                                   0.1f,
                                                   max_err, max_iters, &iter_count, err_evol, &final_error, NULL);
            f64 time_taken = jta_timer_get(&solver_timer);
            for (p_g = 0, p_r = 0; p_g < problem->point_list->count * 3; ++p_g)
            {
                if (dofs[p_g])
                {
                    problem->forces[p_g] = f_r[p_r];
                    problem->deformations[p_g] = u_r[p_r];
                    p_r += 1;
                }
//                else
//                {
//                    problem->forces[p_g] = 0;
//                    problem->deformations[p_g] = 0;
//                }
            }
            JDM_TRACE("Time taken for %"PRIu32" iterations of Jacobi was %g seconds", iter_count, time_taken);
            if (jmtx_res != JMTX_RESULT_SUCCESS && jmtx_res != JMTX_RESULT_NOT_CONVERGED)
            {
                JDM_ERROR("Failed solving the problem using Jacobi's method, reason: %s", jmtx_result_to_str(jmtx_res));
            }
            if (jmtx_res == JMTX_RESULT_NOT_CONVERGED)
            {
                JDM_WARN("Did not converge after %"PRIu32" iterations (error was %g)", iter_count, final_error);
            }

            printf(":(\n");
            solved = 1;
            lin_jfree(G_LIN_JALLOCATOR, u_r);
            lin_jfree(G_LIN_JALLOCATOR, f_r);
            lin_jfree(G_LIN_JALLOCATOR, dofs);
        }
    }

end:
    JDM_LEAVE_FUNCTION;
    return jfw_res_success;
}

jfw_res truss_mouse_button_double_press(jfw_widget* this, i32 x, i32 y, u32 button, u32 mods)
{
    if (button == Button2)
    {
        jta_draw_state* const state = jfw_widget_get_user_pointer(this);
        state->camera = state->original_camera;
        state->vulkan_state->view = jta_camera_to_view_matrix(&state->camera);
        jfw_widget_ask_for_redraw(this);
    }
    return jfw_res_success;
}
