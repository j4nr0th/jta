//
// Created by jan on 21.5.2023.
//

//#include <inttypes.h>
#include "jwin_handlers.h"
#include "../core/jtasolve.h"
#include "../gfx/drawing.h"
#include "solvers/jacobi_point_iteration.h"
//#include "solvers/bicgstab_iteration.h"
#include <jdm.h>
#include <ctype.h>

static void truss_mouse_button_press(const jwin_event_mouse_button_press* e, void* param)
{
    jta_state* const state = param;
    assert(state);
    jrui_context* const ui_ctx = state->ui_state.ui_context;
    switch (e->button)
    {
    case JWIN_MOUSE_BUTTON_TYPE_LEFT:
        jrui_input_mouse_press(ui_ctx, e->x, e->y, JRUI_INPUT_LMB);
        break;

    case JWIN_MOUSE_BUTTON_TYPE_RIGHT:
        //  press was with rmb
        if (!jrui_input_mouse_press(ui_ctx, e->x, e->y, JRUI_INPUT_RMB))
        {
            //  if UI does not take this event, I do
            state->draw_state.track_move = 1;
            state->draw_state.mv_x = e->x; state->draw_state.mv_y = e->y;
        }
        break;

    case JWIN_MOUSE_BUTTON_TYPE_MIDDLE:
        //  press was with mmb
        if (!jrui_input_mouse_press(ui_ctx, e->x, e->y, JRUI_INPUT_MMB))
        {
            //  if UI does not take this event, I do
            state->draw_state.track_turn = 1;
            state->draw_state.mv_x = e->x; state->draw_state.mv_y = e->y;
        }
        break;

    case JWIN_MOUSE_BUTTON_TYPE_SCROLL_UP:
        //  Scroll up
        if (!jrui_input_mouse_press(ui_ctx, e->x, e->y, JRUI_INPUT_SRUP))
        {
            //  if UI does not take this event, I do
            jta_camera_zoom(&state->draw_state.camera, +0.05f);
            state->draw_state.view_matrix = jta_camera_to_view_matrix(&state->draw_state.camera);
            state->draw_state.needs_redraw = 1;
        }
        break;

    case JWIN_MOUSE_BUTTON_TYPE_SCROLL_DN:
        //  Scroll down
        if (!jrui_input_mouse_press(ui_ctx, e->x, e->y, JRUI_INPUT_SRDN))
        {
            //  if UI does not take this event, I do
            jta_camera_zoom(&state->draw_state.camera, -0.05f);
            state->draw_state.view_matrix = jta_camera_to_view_matrix(&state->draw_state.camera);
            state->draw_state.needs_redraw = 1;
        }
        break;

    default:break;
    }
}

static void truss_mouse_button_release(const jwin_event_mouse_button_release* e, void* param)
{
    jta_state* const state = param;
    jrui_context* const ui_ctx = state->ui_state.ui_context;
    switch (e->button)
    {
    case JWIN_MOUSE_BUTTON_TYPE_LEFT:
        jrui_input_mouse_release(ui_ctx, e->x, e->y, JRUI_INPUT_LMB);
        break;
    case JWIN_MOUSE_BUTTON_TYPE_MIDDLE:
        assert(state);
        jrui_input_mouse_release(ui_ctx, e->x, e->y, JRUI_INPUT_MMB);
        state->draw_state.track_turn = 0;
        break;
    case JWIN_MOUSE_BUTTON_TYPE_RIGHT:
        assert(state);
        jrui_input_mouse_release(ui_ctx, e->x, e->y, JRUI_INPUT_RMB);
        state->draw_state.track_move = 0;
        break;
    default:break;
    }
    state->draw_state.mv_x = e->x;
    state->draw_state.mv_y = e->y;
}

static void truss_mouse_motion(const jwin_event_mouse_motion* e, void* param)
{
    jta_state* const state = param;
    assert(state);
    int x = e->x, y = e->y;
    jrui_input_mouse_move(state->ui_state.ui_context, x, y);
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
    if (state->draw_state.track_turn)
    {
        const f32 w = (f32)width, h = (f32)height;
        //  Update camera
        jta_camera_3d* const camera = &state->draw_state.camera;
        const i32 old_x = state->draw_state.mv_x, old_y = state->draw_state.mv_y;
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

        state->draw_state.mv_x = x;
        state->draw_state.mv_y = y;

        state->draw_state.view_matrix = jta_camera_to_view_matrix(camera);
        state->draw_state.needs_redraw = 1;
    }
    if (state->draw_state.track_move)
    {
        const f32 w = (f32)width, h = (f32)height;
        //  Update camera
        jta_camera_3d* const camera = &state->draw_state.camera;
        const i32 old_x = state->draw_state.mv_x, old_y = state->draw_state.mv_y;
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

        state->draw_state.view_matrix = jta_camera_to_view_matrix(camera);
        state->draw_state.needs_redraw = 1;
    }
    assert(state);
    state->draw_state.mv_x = x;
    state->draw_state.mv_y = y;
    end:;
}

static void truss_key_press(const jwin_event_key_press* e, void* param)
{
    JDM_ENTER_FUNCTION;
    jta_state* const state = param;
    jrui_context* const ui_ctx = state->ui_state.ui_context;
    int handled_by_ui = 0;
    //  Forward keys to UI
    switch (e->keycode)
    {
    case JWIN_KEY_DOWN:handled_by_ui = jrui_input_key_down(ui_ctx, JRUI_INPUT_DOWN); break;
    case JWIN_KEY_UP:handled_by_ui = jrui_input_key_down(ui_ctx, JRUI_INPUT_UP); break;
    case JWIN_KEY_LEFT:handled_by_ui = jrui_input_key_down(ui_ctx, JRUI_INPUT_LEFT); break;
    case JWIN_KEY_RIGHT:handled_by_ui = jrui_input_key_down(ui_ctx, JRUI_INPUT_RIGHT); break;
    case JWIN_KEY_ESC:handled_by_ui = jrui_input_key_down(ui_ctx, JRUI_INPUT_ESC); break;
    case JWIN_KEY_TAB:
        if (e->mods & JWIN_MOD_STATE_TYPE_SHIFT)
        {
            handled_by_ui = jrui_input_key_down(ui_ctx, JRUI_INPUT_GROUP_NEXT);
        }
        else
        {
            handled_by_ui = jrui_input_key_down(ui_ctx, JRUI_INPUT_GROUP_PREV);
        }
        break;
    case JWIN_KEY_NUM_ENTER:
    case JWIN_KEY_RETURN:
        handled_by_ui = jrui_input_key_down(ui_ctx, JRUI_INPUT_RTRN);
        break;
    case JWIN_KEY_DELETE:
        handled_by_ui = jrui_input_key_down(ui_ctx, JRUI_INPUT_DEL);
        break;
    case JWIN_KEY_BACKSPACE:
        handled_by_ui = jrui_input_key_down(ui_ctx, JRUI_INPUT_BKSP);
        break;
    default:break;
    }

    if (e->repeated || handled_by_ui)
    {
        //  Do not acknowledge the repeated events and do not handle events that ui took
        JDM_LEAVE_FUNCTION;
        return;
    }

    JDM_LEAVE_FUNCTION;
}

static void truss_key_release(const jwin_event_key_release* e, void* param)
{
    JDM_ENTER_FUNCTION;
    jta_state* const state = param;
    jrui_context* const ui_ctx = state->ui_state.ui_context;
    
    //  Forward keys to UI
    switch (e->keycode)
    {
    case JWIN_KEY_DOWN:jrui_input_key_up(ui_ctx, JRUI_INPUT_DOWN); break;
    case JWIN_KEY_UP:jrui_input_key_up(ui_ctx, JRUI_INPUT_UP); break;
    case JWIN_KEY_LEFT:jrui_input_key_up(ui_ctx, JRUI_INPUT_LEFT); break;
    case JWIN_KEY_RIGHT:jrui_input_key_up(ui_ctx, JRUI_INPUT_RIGHT); break;
    case JWIN_KEY_ESC:jrui_input_key_up(ui_ctx, JRUI_INPUT_ESC); break;
    case JWIN_KEY_TAB:
        if (e->mods & JWIN_MOD_STATE_TYPE_SHIFT)
        {
            jrui_input_key_up(ui_ctx, JRUI_INPUT_GROUP_NEXT);
        }
        else
        {
            jrui_input_key_up(ui_ctx, JRUI_INPUT_GROUP_PREV);
        }
        break;
    case JWIN_KEY_NUM_ENTER:
    case JWIN_KEY_RETURN:
        jrui_input_key_up(ui_ctx, JRUI_INPUT_RTRN);
        break;
    case JWIN_KEY_DELETE:
        jrui_input_key_up(ui_ctx, JRUI_INPUT_DEL);
        break;
    case JWIN_KEY_BACKSPACE:
        jrui_input_key_up(ui_ctx, JRUI_INPUT_BKSP);
        break;
    default:break;
    }

    
    JDM_LEAVE_FUNCTION;
}

static void truss_mouse_button_double_press(const jwin_event_mouse_button_double_press* e, void* param)
{
    jta_state* const state = param;
    if (e->button == JWIN_MOUSE_BUTTON_TYPE_MIDDLE)
    {
        state->draw_state.needs_redraw = 1;
        state->draw_state.camera = state->draw_state.original_camera;
        state->draw_state.view_matrix = jta_camera_to_view_matrix(&state->draw_state.camera);
    }
}

static void refresh_event(const jwin_event_refresh* e, void* param)
{
    JDM_ENTER_FUNCTION;
    (void) e;
    jta_state* const state = param;
    state->draw_state.needs_redraw = 1;
    JDM_LEAVE_FUNCTION;
}

static void destroy_event(const jwin_event_destroy* e, void* param)
{
    (void) e;
    jta_state* const state = param;
    vkDeviceWaitIdle(state->draw_state.wnd_ctx->device);
    //  Destroy allocated buffers
    jta_structure_meshes_destroy(state->draw_state.wnd_ctx, state->draw_state.undeformed_mesh);
    jta_structure_meshes_destroy(state->draw_state.wnd_ctx, state->draw_state.deformed_mesh);
    if (state->ui_state.ui_vtx_buffer)
    {
        jvm_buffer_destroy(state->ui_state.ui_vtx_buffer);
    }
    if (state->ui_state.ui_idx_buffer)
    {
        jvm_buffer_destroy(state->ui_state.ui_idx_buffer);
    }
    jta_texture_destroy(state->draw_state.wnd_ctx, state->ui_state.ui_font_texture);
    jta_vulkan_window_context_destroy(state->draw_state.wnd_ctx);
    jta_vulkan_context_destroy(state->draw_state.vk_ctx);
}

static int close_event(const jwin_event_close* e, void* param)
{
    (void) param;
    jwin_context_mark_to_close(e->context);
    return 1;
}

static void unfocus_event(const jwin_event_focus_lose* e, void* param)
{
    (void) e;
    jta_state* const state = param;
    jrui_input_unfocus(state->ui_state.ui_context);
}

static void character_input_event(const jwin_event_key_char* e, void* param)
{
    jta_state* const state = param;
    jrui_context* ui_ctx = state->ui_state.ui_context;
    if (isprint(*e->utf8))
    {
        jrui_input_character(ui_ctx, e->utf8);
    }
}

const jta_event_handler JTA_HANDLER_ARRAY[] =
        {
                {.type = JWIN_EVENT_TYPE_MOUSE_PRESS, .callback.mouse_button_press = truss_mouse_button_press},
                {.type = JWIN_EVENT_TYPE_MOUSE_RELEASE, .callback.mouse_button_release = truss_mouse_button_release},
                {.type = JWIN_EVENT_TYPE_MOUSE_DOUBLE_PRESS, .callback.mouse_button_double_press = truss_mouse_button_double_press},
                {.type = JWIN_EVENT_TYPE_MOUSE_MOVE, .callback.mouse_motion = truss_mouse_motion},
                {.type = JWIN_EVENT_TYPE_KEY_PRESS, .callback.key_press = truss_key_press},
                {.type = JWIN_EVENT_TYPE_KEY_RELEASE, .callback.key_release = truss_key_release},
                {.type = JWIN_EVENT_TYPE_REFRESH, .callback.refresh = refresh_event},
                {.type = JWIN_EVENT_TYPE_CLOSE, .callback.close = close_event},
                {.type = JWIN_EVENT_TYPE_DESTROY, .callback.destroy = destroy_event},
                {.type = JWIN_EVENT_TYPE_FOCUS_LOSE, .callback.focus_lose = unfocus_event},
                {.type = JWIN_EVENT_TYPE_KEY_CHARACTER, .callback.key_char = character_input_event},
        };


const unsigned JTA_HANDLER_COUNT = sizeof(JTA_HANDLER_ARRAY) / sizeof(*JTA_HANDLER_ARRAY);
