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
        f32 rotation_angle = asinf(vec4_magnitude(rotation_axis)) * 2.0f;

        vec4 real_axis = VEC4(0, 0, 0);
//        real_axis = vec4_add(real_axis, vec4_mul_one(camera->ux, rotation_axis.x));
        real_axis = vec4_mul_one(camera->ux, rotation_axis.x);
        real_axis = vec4_add(real_axis, vec4_mul_one(camera->uy, rotation_axis.y));
        real_axis = vec4_add(real_axis, vec4_mul_one(camera->uz, rotation_axis.z));
        jtb_camera_rotate(camera, real_axis, rotation_angle);
//        const vec4 u = vec4_unit(real_axis);
//        printf("Axis of rotation (%g, %g, %g) by %6.2f degrees\n", u.x, u.y, u.z, 180.0 * rotation_angle);

        state->mv_x = x;
        state->mv_y = y;

        state->vulkan_state->view = jtb_camera_to_view_matrix(camera);
        jfw_widget_ask_for_redraw(this);

        state->mv_x = x;
        state->mv_y = y;
        return jfw_res_success;
    }
    return jfw_res_success;
}
