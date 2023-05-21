//
// Created by jan on 2.2.2023.
//

#include "widget-base.h"

jfw_res jfw_widget_destroy(jfw_widget* widget)
{
    assert(widget);
    JFW_ENTER_FUNCTION;
//    if (widget->window->base == widget)
//    {
//        return jfw_res_wnd_widget_root;
//    }
    if (widget->dtor_fn)
    {
        widget->dtor_fn(widget);
    }
    jfw_widget* const parent = widget->parent;
    if (parent)
    {
        u32 index;
        for (index = 0; index < parent->children_count; ++index)
        {
            if (parent->children_array[index] == widget) break;
        }
        assert(parent->children_count > index);
        if (parent->children_count > index)
        {
            memmove(parent->children_array + index, parent->children_array + index + 1, sizeof(*parent->children_array) * (parent->children_count - 1 - index));
            parent->children_array[parent->children_count - 1] = NULL;
            parent->children_count -= 1;
        }
        if (parent->functions.child_destroyed)
        {
            parent->functions.child_destroyed(parent, widget);
        }
    }
    while (widget->children_count)
    {
        jfw_widget* const child = widget->children_array[widget->children_count - 1];
        if (child->functions.parent_destroyed)
        {
            child->functions.parent_destroyed(child, widget);
        }
        jfw_widget_destroy(child);
    }
    if (widget->children_array)
    {
        jfw_free(&widget->children_array);
    }
    memset(widget, 0, sizeof(*widget));
    jfw_free(&widget);
    
    JFW_LEAVE_FUNCTION;
    return jfw_res_success;
}

jfw_res jfw_widget_resize(jfw_widget* widget, u32 w, u32 h)
{
    JFW_ENTER_FUNCTION;
    widget->width = w;
    widget->height = h;
    jfw_widget_ask_for_redraw(widget);
    JFW_LEAVE_FUNCTION;
    return jfw_res_success;
}

jfw_res jfw_widget_reposition(jfw_widget* widget, i32 x, i32 y)
{
    JFW_ENTER_FUNCTION;
    widget->x = x;
    widget->y = y;
    jfw_widget_ask_for_redraw(widget);
    JFW_LEAVE_FUNCTION;
    return jfw_res_wnd_widget_root;
}

jfw_res jfw_widget_ask_for_redraw(jfw_widget* widget)
{
    JFW_ENTER_FUNCTION;
    widget->redraw += 1;
    if (widget->parent)
    {
        jfw_widget_ask_for_redraw(widget->parent);
    }
    widget->window->redraw += 1;
    JFW_LEAVE_FUNCTION;
    return jfw_res_success;
}

void* jfw_widget_get_user_pointer(const jfw_widget* widget)
{
    JFW_ENTER_FUNCTION;
    void* pointer = widget->user_pointer;
    JFW_LEAVE_FUNCTION;
    return pointer;
}

u0 jfw_widget_set_user_pointer(jfw_widget* widget, u0* user_pointer)
{
    JFW_ENTER_FUNCTION;
    widget->user_pointer = user_pointer;
    JFW_LEAVE_FUNCTION;
}

static jfw_res widget_add_child(jfw_widget* parent, jfw_widget* child)
{
    JFW_ENTER_FUNCTION;
    jfw_res res;
    if (!parent->children_array)
    {
        if (!jfw_success(res = jfw_calloc(8, sizeof(jfw_widget*), &parent->children_array)))
        {
            JFW_ERROR("Failed allocating memory for child array");
            JFW_LEAVE_FUNCTION;
            return res;
        }
        parent->children_capacity = 8;
    }
    else if (parent->children_capacity == parent->children_count)
    {
        u32 new_size = parent->children_count + 8;
        if (!jfw_success(res = jfw_realloc(sizeof(jfw_widget*) * new_size, &parent->children_array)))
        {
            JFW_ERROR("Failed reallocating memory for child array");
            JFW_LEAVE_FUNCTION;
            return res;
        }
        parent->children_capacity = new_size;
        memset(parent->children_array + parent->children_count, 0, sizeof(jfw_widget*) * (parent->children_capacity - parent->children_count));
    }
    if (parent->functions.child_created)
    {
        if (!jfw_success(res = parent->functions.child_created(parent, child)))
        {
            char w_buffer[64] = {0};
            jfw_widget_to_string(parent, sizeof(w_buffer), w_buffer);
            JFW_ERROR("Failed calling child creation function on parent (%s)", w_buffer);
            JFW_LEAVE_FUNCTION;
            return res;
        }
    }
    parent->children_array[parent->children_count++] = child;
    child->parent = parent;
    child->window = parent->window;
    JFW_LEAVE_FUNCTION;
    return jfw_res_success;
}

jfw_res jfw_widget_create_as_child(jfw_widget* parent, u32 w, u32 h, i32 x, i32 y, jfw_widget** pp_widget)
{
    JFW_ENTER_FUNCTION;
    assert(parent != NULL);
    assert(pp_widget);
    jfw_widget* this = NULL;
    jfw_res res;
    if (!jfw_success(res = jfw_malloc(sizeof(*this), &this)))
    {
        JFW_ERROR("Failed allocating memory for widget");
        JFW_LEAVE_FUNCTION;
        return res;
    }
    this->width = w;
    this->height = h;
    this->x = x;
    this->y = y;
    if (parent && !jfw_success(res = widget_add_child(parent, this)))
    {
        char w_buffer[64] = {0};
        jfw_widget_to_string(parent, sizeof(w_buffer), w_buffer);
        jfw_free(&this);
        JFW_ERROR("Failed adding child to parent (%s)", w_buffer);
        JFW_LEAVE_FUNCTION;
        return res;
    }
    this->parent = parent;
    *pp_widget = this;
    jfw_widget_ask_for_redraw(this);
    JFW_LEAVE_FUNCTION;
    return jfw_res_success;
}

u0 jfw_widget_bounds(jfw_widget* widget, i32* p_x0, i32* p_y0, i32* p_x1, i32* p_y1)
{
    JFW_ENTER_FUNCTION;
    i32 x0, x1, y0, y1;
    i32 parent_x0, parent_x1, parent_y0, parent_y1;
    if (widget->parent)
    {
        jfw_widget_bounds(widget->parent, &parent_x0, &parent_y0, &parent_x1, &parent_y1);
        x0 = parent_x0 + widget->x;
        y0 = parent_y0 + widget->y;
        x1 = x0 + (i32)widget->width;
        y1 = y0 + (i32)widget->height;
        x0 = clamp_i32(x0, parent_x0, parent_x1);
        x1 = clamp_i32(x1, parent_x0, parent_x1);
        y0 = clamp_i32(y0, parent_y0, parent_y1);
        y1 = clamp_i32(y1, parent_y0, parent_y1);
        *p_x0 = x0;
        *p_x1 = x1;
        *p_y0 = y0;
        *p_y1 = y1;
    }
    else
    {
        *p_x0 = 0;
        *p_y0 = 0;
        *p_x1 = (i32)widget->width;
        *p_y1 = (i32)widget->height;
    }
    JFW_LEAVE_FUNCTION;
}

jfw_res jfw_widget_create_as_base(jfw_window* parent, u32 w, u32 h, i32 x, i32 y, jfw_widget** pp_widget)
{
    JFW_ENTER_FUNCTION;
    assert(parent != NULL);
    assert(pp_widget);
    jfw_widget* this = NULL;
    jfw_res res;
    if (!jfw_success(res = jfw_malloc(sizeof(*this), &this)))
    {
        JFW_ERROR("Failed to allocate memory for widget");
        JFW_LEAVE_FUNCTION;
        return res;
    }
    this->width = w;
    this->height = h;
    this->x = x;
    this->y = y;
    this->window = parent;
    jfw_widget* old_base = parent->base;
    if (old_base)
    {
        if (!jfw_success(res = jfw_widget_add_child(this, old_base)))
        {
            JFW_ERROR("Could not add old base as a child to new");
            jfw_free(&this);
            return res;
        }
    }
    parent->base = this;
    *pp_widget = this;
    jfw_widget_ask_for_redraw(this);
    JFW_LEAVE_FUNCTION;
    return jfw_res_success;
}

jfw_res jfw_widget_add_child(jfw_widget* parent, jfw_widget* child)
{
    if (parent->children_count == parent->children_capacity)
    {
        const u32 new_capacity = parent->children_capacity ? parent->children_capacity << 1 : 8;
        jfw_res res = jfw_realloc(new_capacity * sizeof(*parent->children_array), &parent->children_array);
        if (!jfw_success(res))
        {
            JFW_ERROR("Failed reallocating child array of a window");
            return res;
        }
        parent->children_capacity = new_capacity;
    }
    assert(child->parent == NULL);
    child->parent = parent;
    parent->children_array[parent->children_count++] = child;
    return jfw_res_ctx_no_ic;
}
