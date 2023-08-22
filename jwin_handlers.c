//
// Created by jan on 21.5.2023.
//

#include <inttypes.h>
#include "jwin_handlers.h"
#include "core/jtasolve.h"
#include "gfx/drawing_3d.h"
#include <solvers/jacobi_point_iteration.h>
#include <solvers/bicgstab_iteration.h>

static void truss_mouse_button_press(const jwin_event_mouse_button_press* e, void* param)
{
    jta_draw_state* const state = param;
    assert(state);
//    jwin_event_custom custom_redraw =
//            {
//            .base = e->base,
//            .custom = state,
//            };
//    custom_redraw.base.type = JWIN_EVENT_TYPE_CUSTOM + 1;
    switch (e->button)
    {
    case JWIN_MOUSE_BUTTON_TYPE_RIGHT:
        //  press was with rmb
        state->track_move = 1;
        state->mv_x = e->x; state->mv_y = e->y;
        break;
    case JWIN_MOUSE_BUTTON_TYPE_MIDDLE:
        //  press was with mmb
        state->track_turn = 1;
        state->mv_x = e->x; state->mv_y = e->y;
        break;

    case JWIN_MOUSE_BUTTON_TYPE_SCROLL_UP:
        //  Scroll up
        jta_camera_zoom(&state->camera, +0.05f);
        state->view_matrix = jta_camera_to_view_matrix(&state->camera);
        state->needs_redraw = 1;
        break;
    case JWIN_MOUSE_BUTTON_TYPE_SCROLL_DN:
        //  Scroll down
        jta_camera_zoom(&state->camera, -0.05f);
        state->view_matrix = jta_camera_to_view_matrix(&state->camera);
        state->needs_redraw = 1;
        break;
    default:break;
    }
}

static void truss_mouse_button_release(const jwin_event_mouse_button_release* e, void* param)
{
    jta_draw_state* const state = param;
    switch (e->button)
    {
    case JWIN_MOUSE_BUTTON_TYPE_MIDDLE:
        assert(state);
        state->track_turn = 0;
        break;
    case JWIN_MOUSE_BUTTON_TYPE_RIGHT:
        assert(state);
        state->track_move = 0;
        break;
    default:break;
    }
    state->mv_x = e->x;
    state->mv_y = e->y;
}

static void truss_mouse_motion(const jwin_event_mouse_motion* e, void* param)
{
    jta_draw_state* const state = param;
    jwin_event_custom custom_redraw =
            {
                    .base = e->base,
                    .custom = state,
            };
    custom_redraw.base.type = JWIN_EVENT_TYPE_CUSTOM + 1;
    assert(state);
    int x = e->x, y = e->y;
    unsigned width, height;
    jwin_window_get_size(e->base.window, &width, &height);
    //  Clamp x and y to intervals [0, width) and [0, height)
    if (x < 0)
    {
        x = 0;
    }
    else if (x > (i32)width)
    {
        x = (i32)width - 1;
    }
    if (y < 0)
    {
        y = 0;
    }
    else if (y > (i32)height)
    {
        y = (i32)height - 1;
    }
    if (state->track_turn)
    {
        const f32 w = (f32)width, h = (f32)height;
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

        state->view_matrix = jta_camera_to_view_matrix(camera);
        jwin_window_send_custom_event(e->base.window, &custom_redraw);
    }
    if (state->track_move)
    {
        const f32 w = (f32)width, h = (f32)height;
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

        state->view_matrix = jta_camera_to_view_matrix(camera);
        jwin_window_send_custom_event(e->base.window, &custom_redraw);
    }
    assert(state);
    state->mv_x = x;
    state->mv_y = y;
    end:;
}

static void truss_key_press(const jwin_event_key_press* e, void* param)
{
    if (e->repeated)
    {
        //  Do not acknowledge the repeated events
        return;
    }
    JDM_ENTER_FUNCTION;
    jta_draw_state* const state = param;
    jwin_event_custom custom_redraw =
            {
                    .base = e->base,
                    .custom = state,
            };
    custom_redraw.base.type = JWIN_EVENT_TYPE_CUSTOM + 1;
    static int solved = 0;
    if (e->keycode == JWIN_KEY_F12)
    {
        //  Take a screenshot
        jwin_window_send_custom_event(e->base.window, &custom_redraw);
        state->screenshot = 1;
    }
    else if (e->keycode == JWIN_KEY_SPACE)
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
    else if (e->keycode == JWIN_KEY_R)
    {
        jta_structure_meshes new_meshes = {};
        gfx_result res;
        res = mesh_init_sphere(&new_meshes.spheres, 4, state->wnd_ctx);
        if (res != GFX_RESULT_SUCCESS)
        {
            JDM_ERROR("Could not initialise new sphere mesh, reason: %s", gfx_result_to_str(res));
            goto end;
        }

        res = mesh_init_cone(&new_meshes.cones, 1 << 4, state->wnd_ctx);
        if (res != GFX_RESULT_SUCCESS)
        {
            JDM_ERROR("Could not initialise new cone mesh, reason: %s", gfx_result_to_str(res));
            mesh_uninit(&new_meshes.spheres);
            goto end;
        }

        res = mesh_init_truss(&new_meshes.cylinders, 1 << 4, state->wnd_ctx);
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
                    &new_meshes, &state->config->display, state->p_problem, state->p_solution, state->wnd_ctx);
        }
        else
        {
            res = jta_structure_meshes_generate_undeformed(
                    &new_meshes, &state->config->display, state->p_problem, state->wnd_ctx);
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
        state->needs_redraw = 1;
    }

end:
    JDM_LEAVE_FUNCTION;
}

static void truss_mouse_button_double_press(const jwin_event_mouse_button_double_press* e, void* param)
{
    jta_draw_state* const state = param;
    jwin_event_custom custom_redraw =
            {
                    .base = e->base,
                    .custom = state,
            };
    custom_redraw.base.type = JWIN_EVENT_TYPE_CUSTOM + 1;
    if (e->button == JWIN_MOUSE_BUTTON_TYPE_MIDDLE)
    {
        state->camera = state->original_camera;
        state->view_matrix = jta_camera_to_view_matrix(&state->camera);
        jwin_window_send_custom_event(e->base.window, &custom_redraw);
    }
}

static void refresh_event(const jwin_event_refresh* e, void* param)
{
    JDM_ENTER_FUNCTION;
    (void) e;
    jta_draw_state* const state = param;
//    gfx_result draw_res = jta_draw_frame(
//            state->wnd_ctx, state->view_matrix, &state->meshes, &state->camera);
//    if (draw_res != GFX_RESULT_SUCCESS)
//    {
//        JDM_WARN("Drawing of frame failed, reason: %s", gfx_result_to_str(draw_res));
//    }
    state->needs_redraw = 1;
    JDM_LEAVE_FUNCTION;
}

static void custom_event(const jwin_event_custom* e, void* param)
{
    (void) param;
    JDM_ENTER_FUNCTION;
    switch (e->base.type)
    {
    case JWIN_EVENT_TYPE_CUSTOM + 1:
        //  Custom redraw
        refresh_event(&e->base, e->custom);
        break;
    default:JDM_ERROR("Got a custom event with type JWIN_EVENT_TYPE_CUSTOM + %d", e->base.type - JWIN_EVENT_TYPE_CUSTOM);
    }
    JDM_LEAVE_FUNCTION;
}

static void destroy_event(const jwin_event_destroy* e, void* param)
{
    (void) e;
    (void) param;
    jta_draw_state* const state = param;
    vkDeviceWaitIdle(state->wnd_ctx->device);
    jta_vulkan_window_context_destroy(state->wnd_ctx);
    jta_vulkan_context_destroy(state->vk_ctx);
}

static int close_event(const jwin_event_close* e, void* param)
{
    (void) param;
    jwin_context_mark_to_close(e->context);
    return 1;
}

const jta_event_handler JTA_HANDLER_ARRAY[] =
        {
                {.type = JWIN_EVENT_TYPE_MOUSE_PRESS, .callback.mouse_button_press = truss_mouse_button_press},
                {.type = JWIN_EVENT_TYPE_MOUSE_RELEASE, .callback.mouse_button_release = truss_mouse_button_release},
                {.type = JWIN_EVENT_TYPE_MOUSE_DOUBLE_PRESS, .callback.mouse_button_double_press = truss_mouse_button_double_press},
                {.type = JWIN_EVENT_TYPE_MOUSE_MOVE, .callback.mouse_motion = truss_mouse_motion},
                {.type = JWIN_EVENT_TYPE_KEY_PRESS, .callback.key_press = truss_key_press},
                {.type = JWIN_EVENT_TYPE_REFRESH, .callback.refresh = refresh_event},
                {.type = JWIN_EVENT_TYPE_CUSTOM, .callback.custom = custom_event},
                {.type = JWIN_EVENT_TYPE_CLOSE, .callback.close = close_event},
                {.type = JWIN_EVENT_TYPE_DESTROY, .callback.destroy = destroy_event},
        };


const unsigned JTA_HANDLER_COUNT = sizeof(JTA_HANDLER_ARRAY) / sizeof(*JTA_HANDLER_ARRAY);
