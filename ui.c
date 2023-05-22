//
// Created by jan on 21.5.2023.
//

#include "ui.h"
#include "jfw/error_system/error_stack.h"

jfw_res truss_mouse_button_press(jfw_widget* this, i32 x, i32 y, u32 button, u32 mods)
{
    jfw_res result = jfw_res_success;
    jtb_draw_state* const state = jfw_widget_get_user_pointer(this);
    assert(state);
    if (button == Button3)
    {
        //  press was with rmb
        state->track_move = 1;
        state->mv_x = x; state->mv_y = y;
        return jfw_res_success;
    }
    else if (button == Button4)
    {
        //  Scroll up
        jtb_camera_zoom(&state->camera, +0.05f);
        state->vulkan_state->view = jtb_camera_to_view_matrix(&state->camera);
        jfw_widget_ask_for_redraw(this);
    }
    else if (button == Button5)
    {
        //  Scroll down
        jtb_camera_zoom(&state->camera, -0.05f);
        state->vulkan_state->view = jtb_camera_to_view_matrix(&state->camera);
        jfw_widget_ask_for_redraw(this);
    }

    return result;
}

jfw_res truss_mouse_button_release(jfw_widget* this, i32 x, i32 y, u32 button, u32 mods)
{
    if (button == Button3)
    {
        //  press was with rmb
        jtb_draw_state* const state = jfw_widget_get_user_pointer(this);
        assert(state);
        state->track_move = 0;
        return jfw_res_success;
    }
    return jfw_res_success;
}

jfw_res truss_mouse_motion(jfw_widget* const this, const i32 x, const i32 y, const u32 mods)
{
    jtb_draw_state* const state = jfw_widget_get_user_pointer(this);
    assert(state);
    if (mods & Button3Mask && state && state->track_move == 1)
    {
        const f32 w = (f32)this->width, h = (f32)this->height;
        //  Update camera
        jtb_camera_3d* const camera = &state->camera;
        const i32 old_x = state->mv_x, old_y = state->mv_y;
        //  Determine the position on the sphere's surface
        f32 xs_new = 2 * (f32)x / w - 1, ys_new = -2 * (f32)y / w + h / w;
        f32 xs_old = 2 * (f32)old_x / w - 1, ys_old = -2 * (f32)old_y / w + h / w;
        //  Find the missing coordinates
        f32 vz2_new = 1 - xs_new * xs_new - ys_new * ys_new;
        f32 zs_new;
        if (vz2_new > 1)
        {
            zs_new = -1.0f;
            xs_new = ys_new = 0.0f;
        }
        else if (vz2_new < 0.0f)
        {
            zs_new = -0.0f;
        }
        else
        {
            zs_new = -sqrtf(vz2_new);
        }
        f32 vz2_old = 1 - xs_old * xs_old - ys_old * ys_old;
        f32 zs_old;
        if (vz2_old > 1)
        {
            zs_old = -1.0f;
            xs_old = ys_old = 0.0f;
        }
        else if (vz2_old < 0.0f)
        {
            zs_old = -0.0f;
        }
        else
        {
            zs_old = -sqrtf(vz2_old);
        }
        vec4 r_new = VEC4(xs_new, ys_new, zs_new);
        vec4 r_old = VEC4(xs_old, ys_old, zs_old);

        vec4 cross = vec4_cross(r_new, r_old);
        const f32 angle = vec4_magnitude(cross);
        vec4 real_axis = {};
        real_axis = vec4_add(real_axis, vec4_mul_one(camera->ux, cross.x));
        real_axis = vec4_add(real_axis, vec4_mul_one(camera->uy, cross.y));
        real_axis = vec4_add(real_axis, vec4_mul_one(camera->uz, cross.z));
        jtb_camera_rotate(camera, real_axis, angle);
//        printf("Rotating camera around axis (%g, %g, %g) by %g degrees\n", cross.x, cross.y, cross.z, 180.0f * angle * M_1_PI);
        state->vulkan_state->view = jtb_camera_to_view_matrix(camera);
        jfw_widget_ask_for_redraw(this);

        state->mv_x = x;
        state->mv_y = y;
        return jfw_res_success;
    }
    return jfw_res_success;
}
