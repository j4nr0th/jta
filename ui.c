//
// Created by jan on 21.5.2023.
//

#include <X11/keysym.h>
#include <inttypes.h>
#include "ui.h"
#include "core/jtasolve.h"
#include <solvers/jacobi_point_iteration.h>
#include <solvers/bicgstab_iteration.h>

jfw_result truss_mouse_button_press(jfw_window* this, i32 x, i32 y, u32 button, u32 mods)
{
    jta_draw_state* const state = jfw_window_get_usr_ptr(this);
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
        jfw_window_ask_for_redraw(this);
        break;
    case Button5:
        //  Scroll down
        jta_camera_zoom(&state->camera, -0.05f);
        state->vulkan_state->view = jta_camera_to_view_matrix(&state->camera);
        jfw_window_ask_for_redraw(this);
        break;

    default:break;
    }

    return JFW_RESULT_SUCCESS;
}

jfw_result truss_mouse_button_release(jfw_window* this, i32 x, i32 y, u32 button, u32 mods)
{
    jta_draw_state* const state = jfw_window_get_usr_ptr(this);
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
    return JFW_RESULT_SUCCESS;
}

jfw_result truss_mouse_motion(jfw_window* const this, i32 x, i32 y, const u32 mods)
{
    jta_draw_state* const state = jfw_window_get_usr_ptr(this);
    assert(state);
    if (mods & Button2Mask && state && state->track_turn)
    {
        //  Clamp x and y to intervals [0, w) and [0, h)
        if (x < 0)
        {
            x = 0;
        }
        else if (x > (i32)this->w)
        {
            x = (i32)this->w - 1;
        }
        if (y < 0)
        {
            y = 0;
        }
        else if (y > (i32)this->h)
        {
            y = (i32)this->h - 1;
        }

        const f32 w = (f32)this->w, h = (f32)this->h;
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
        jfw_window_ask_for_redraw(this);
    }
    if (mods & Button3Mask && state && state->track_move)
    {
        //  Clamp x and y to intervals [0, w) and [0, h)
        if (x < 0)
        {
            x = 0;
        }
        else if (x > (i32)this->w)
        {
            x = (i32)this->w - 1;
        }
        if (y < 0)
        {
            y = 0;
        }
        else if (y > (i32)this->h)
        {
            y = (i32)this->h - 1;
        }

        const f32 w = (f32)this->w, h = (f32)this->h;
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
        jfw_window_ask_for_redraw(this);
    }
    assert(state);
    state->mv_x = x;
    state->mv_y = y;
end:
    return JFW_RESULT_SUCCESS;
}

jfw_result truss_key_press(jfw_window* this, KeySym key_sym)
{
    JDM_ENTER_FUNCTION;

    jta_draw_state* const state = jfw_window_get_usr_ptr(this);
    static int solved = 0;
    if (key_sym == XK_F12)
    {
        //  Take a screenshot
        jfw_window_ask_for_redraw(this);
        state->screenshot = 1;
    }
    else if (key_sym == XK_space)
    {
        if (!solved)
        {
            assert(state);
            jta_result res = jta_solve_problem(&state->config->problem, state->p_problem, state->p_solution);
            if (res != JTA_RESULT_SUCCESS)
            {
                JDM_ERROR("Could not solve the problem, reason: %s", jta_result_to_str(res));
            }
            else
            {
                solved = 1;
            }
        }
    }
    else if (key_sym == XK_r)
    {
        jta_structure_meshes new_meshes = {};
        gfx_result res;
        res = mesh_init_sphere(&new_meshes.spheres, 4, state->vulkan_state, state->vulkan_resources);
        if (res != GFX_RESULT_SUCCESS)
        {
            JDM_ERROR("Could not initialise new sphere mesh, reason: %s", gfx_result_to_str(res));
            goto end;
        }

        res = mesh_init_cone(&new_meshes.cones, 1 << 4, state->vulkan_state, state->vulkan_resources);
        if (res != GFX_RESULT_SUCCESS)
        {
            JDM_ERROR("Could not initialise new cone mesh, reason: %s", gfx_result_to_str(res));
            mesh_uninit(&new_meshes.spheres);
            goto end;
        }

        res = mesh_init_truss(&new_meshes.cylinders, 1 << 4, state->vulkan_state, state->vulkan_resources);
        if (res != GFX_RESULT_SUCCESS)
        {
            JDM_ERROR("Could not initialise new cylinder mesh, reason: %s", gfx_result_to_str(res));
            mesh_uninit(&new_meshes.cones);
            mesh_uninit(&new_meshes.spheres);
            goto end;
        }


        if (solved)
        {
//            res = jta_structure_meshes_generate_undeformed(
//                    &new_meshes, &state->config->display, state->p_problem, state->vulkan_state);
//            if (res != GFX_RESULT_SUCCESS)
//            {
//                JDM_ERROR("Could not create new mesh, reason: %s", gfx_result_to_str(res));
//                mesh_uninit(&new_meshes.cones);
//                mesh_uninit(&new_meshes.spheres);
//                mesh_uninit(&new_meshes.cylinders);
//                goto end;
//            }
            res = jta_structure_meshes_generate_deformed(
                    &new_meshes, &state->config->display, state->p_problem, state->p_solution, state->vulkan_state);
        }
        else
        {
            res = jta_structure_meshes_generate_undeformed(
                    &new_meshes, &state->config->display, state->p_problem, state->vulkan_state);
        }

        if (res != GFX_RESULT_SUCCESS)
        {
            JDM_ERROR("Could not create new mesh, reason: %s", gfx_result_to_str(res));
            mesh_uninit(&new_meshes.cones);
            mesh_uninit(&new_meshes.spheres);
            mesh_uninit(&new_meshes.cylinders);
            goto end;
        }

        jta_structure_meshes old_meshes = state->meshes;
        state->meshes = new_meshes;
        mesh_uninit(&old_meshes.cones);
        mesh_uninit(&old_meshes.spheres);
        mesh_uninit(&old_meshes.cylinders);
    }

end:
    JDM_LEAVE_FUNCTION;
    return JFW_RESULT_SUCCESS;
}

jfw_result truss_mouse_button_double_press(jfw_window* this, i32 x, i32 y, u32 button, u32 mods)
{
    if (button == Button2)
    {
        jta_draw_state* const state = jfw_window_get_usr_ptr(this);
        state->camera = state->original_camera;
        state->vulkan_state->view = jta_camera_to_view_matrix(&state->camera);
        jfw_window_ask_for_redraw(this);
    }
    return JFW_RESULT_SUCCESS;
}
