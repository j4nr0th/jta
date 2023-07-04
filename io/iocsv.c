//
// Created by jan on 30.6.2023.
//

#include <jdm.h>
#include <inttypes.h>
#include "iocsv.h"


struct jio_csv_data_struct
{
    jallocator* allocator;                  //  Allocator used by this csv
    uint32_t column_capacity;               //  Max size of columns before resizing the array
    uint32_t column_count;                  //  Number of columns used
    uint32_t column_length;                 //  Length of each column
    jio_csv_column* columns;                //  Columns themselves
};

static inline jio_string_segment extract_string_segment(const char* ptr, const char* const row_end, const char** p_end, const char* restrict separator)
{
    jio_string_segment ret = {.begin = NULL, .len = 0};
    const char* entry_end = strstr(ptr, separator);
    if (entry_end > row_end || !entry_end)
    {
        entry_end = row_end;
    }

    ret.begin = ptr;
    ret.len = entry_end - ptr;
    *p_end = entry_end;
    return ret;
}

static inline u32 count_row_entries(const char* const csv, const char* const end_of_row, const char* restrict sep, size_t sep_len)
{
    u32 so_far = 0;
    for (const char* ptr = strstr(csv, sep); ptr && ptr < end_of_row; ++so_far)
    {
        ptr = strstr(ptr + sep_len, sep);
    }
    return so_far + 1;
}

static inline jio_result extract_row_entries(const u32 expected_elements, const char* const row_begin, const char* const row_end, const char* restrict sep, size_t sep_len, const bool trim, jio_string_segment* const p_out)
{
    JDM_ENTER_FUNCTION;
    u32 i;
    const char* end = NULL, *begin = row_begin;
    for (i = 0; i < expected_elements && end < row_end; ++i)
    {
        p_out[i] = extract_string_segment(begin, row_end, &end, sep);
        begin = end + sep_len;
    }
    
    if (i != expected_elements)
    {
        JDM_ERROR("Row contained only %u elements instead of %u", i, expected_elements);
        return JIO_RESULT_BAD_CSV_FORMAT;
    }
    if (end < row_end)
    {
        JDM_ERROR("Row contained %u elements instead of %u", count_row_entries(row_begin, row_end, sep, sep_len), expected_elements);
        return JIO_RESULT_BAD_CSV_FORMAT;
    }


    if (trim)
    {
        for (i = 0; i < expected_elements; ++i)
        {
            jio_string_segment segment = p_out[i];
            //  Trim front whitespace
            while (iswhitespace(*segment.begin) && segment.len)
            {
                segment.begin += 1;
                segment.len -= 1;
            }
            //  Trim back whitespace
            while (iswhitespace(*(segment.begin + segment.len - 1)) && segment.len)
            {
                segment.len -= 1;
            }
            p_out[i] = segment;
        }
    }
    
    JDM_LEAVE_FUNCTION;
    return JIO_RESULT_SUCCESS;
}

jio_result jio_parse_csv(
        const jio_memory_file* const mem_file, const char* restrict separator, const bool trim_whitespace, const bool has_headers,
        jio_csv_data** const pp_csv, jallocator* const allocator, linear_jallocator* const lin_allocator)
{
    JDM_ENTER_FUNCTION;
    if (!mem_file || !pp_csv || !allocator || !lin_allocator || !separator)
    {
        if (!mem_file)
        {
            JDM_ERROR("Memory mapped file to parse was not provided");
        }
        if (!pp_csv)
        {
            JDM_ERROR("Output pointer was null");
        }
        if (!allocator)
        {
            JDM_ERROR("Allocator was not provided");
        }
        if (!lin_allocator)
        {
            JDM_ERROR("Linear allocator was not provided");
        }
        if (!separator)
        {
            JDM_ERROR("Separator was not provided");
        }
        JDM_LEAVE_FUNCTION;
        return JIO_RESULT_NULL_ARG;
    }
    jio_result res;
    void* const base = lin_jalloc_get_current(lin_allocator);
    jio_csv_data* csv = NULL;
    csv = jalloc(allocator, sizeof(*csv));
    if (!csv)
    {
        res = JIO_RESULT_BAD_ALLOC;
        goto end;
    }

    memset(csv, 0, sizeof(*csv));
    csv->allocator = allocator;

    //  Parse the first row
    const char* row_begin = mem_file->ptr;
    const char* row_end = strchr(row_begin, '\n');

    //  Count columns in the csv file
    size_t sep_len = strlen(separator);
    const u32 column_count = count_row_entries(row_begin, row_end ?: mem_file->ptr + mem_file->file_size, separator,
                                               sep_len);
    u32 row_capacity = 128;
    u32 row_count = 0;
    jio_string_segment* segments = lin_jalloc(lin_allocator, sizeof(*segments) * row_capacity * column_count);
    if (!segments)
    {
        res = JIO_RESULT_BAD_ALLOC;
        JDM_ERROR("Could not allocate memory for csv parsing");
        goto end;
    }

    for (;;)
    {
        //  Check if more space is needed to parse the row
        if (row_capacity == row_count)
        {
            const u32 new_capacity = row_capacity + 128;
            jio_string_segment* const new_ptr = lin_jrealloc(lin_allocator, segments, sizeof(*segments) * new_capacity * column_count);
            if (!new_ptr)
            {
                res = JIO_RESULT_BAD_ALLOC;
                JDM_ERROR("Could not reallocate memory for csv parsing");
                goto end;
            }
            segments = new_ptr;
            row_capacity = new_capacity;
        }

        if ((res = extract_row_entries(column_count, row_begin, row_end, separator, sep_len, trim_whitespace, segments + row_count * column_count)))
        {
            JDM_ERROR("Failed parsing row %u of CSV file \"%s\", reason: %s", row_count + 1, mem_file->name, jio_result_to_str(res));
            goto end;
        }

        row_count += 1;
        //  Move to the next row
        if (row_end == mem_file->ptr + mem_file->file_size)
        {
            break;
        }
        row_begin = row_end + 1;
        row_end = strchr(row_end + 1, '\n');
        if (!row_end)
        {
            row_end = strchr(row_begin, 0);
        }
        if (row_end - row_begin < 2)
        {
            break;
        }
    }

    //  Convert the data into appropriate format
    csv->column_count = column_count;
    csv->column_length = row_count - (has_headers ? 1 : 0);

    jio_csv_column* const columns = jalloc(allocator, sizeof(*columns) * column_count);
    if (!columns)
    {
        res = JIO_RESULT_BAD_ALLOC;
        JDM_ERROR("Could not allocate memory for csv column array");
        goto end;
    }
    for (u32 i = 0; i < column_count; ++i)
    {
        jio_csv_column* const p_column = columns + i;
        p_column->capacity = (p_column->count = csv->column_length);
        jio_string_segment* const elements = jalloc(allocator, sizeof(*elements) * p_column->count);
        if (!elements)
        {
            for (u32 j = 0; j < i; ++j)
            {
                jfree(allocator, columns[j].elements);
            }
            JDM_ERROR("Could not allocate memory for csv column elements");
            goto end;
        }
        if (has_headers)
        {
            for (u32 j = 0; j < row_count - 1; ++j)
            {
                elements[j] = segments[i + j * column_count + column_count];
            }
        }
        else
        {
            for (u32 j = 0; j < row_count; ++j)
            {
                elements[j] = segments[i + j * column_count];
            }
        }
        p_column->elements = elements;
        p_column->header = has_headers ? segments[i] : (jio_string_segment){ .begin = NULL, .len = 0 };

    }
    lin_jfree(lin_allocator, segments);
    csv->columns = columns;

    *pp_csv = csv;
    JDM_LEAVE_FUNCTION;
    return JIO_RESULT_SUCCESS;
end:
    lin_jalloc_set_current(lin_allocator, base);
    jfree(allocator, csv);
    JDM_LEAVE_FUNCTION;
    return res;
}

jio_result jio_csv_release(jio_csv_data* data)
{
    JDM_ENTER_FUNCTION;
    if (!data)
    {
        JDM_ERROR("Csv file was not provided");
        JDM_LEAVE_FUNCTION;
        return JIO_RESULT_NULL_ARG;
    }
    for (u32 i = 0; i < data->column_count; ++i)
    {
        jfree(data->allocator, data->columns[i].elements);
    }

    jfree(data->allocator, data->columns);
    jfree(data->allocator, data);

    JDM_LEAVE_FUNCTION;
    return JIO_RESULT_SUCCESS;
}

jio_result jio_csv_shape(const jio_csv_data* data, uint32_t* p_rows, uint32_t* p_cols)
{
//    JDM_ENTER_FUNCTION;
    if (p_rows)
    {
        *p_rows = data->column_length;
    }
    if (p_cols)
    {
        *p_cols = data->column_count;
    }
//    JDM_LEAVE_FUNCTION;
    return JIO_RESULT_SUCCESS;
}

jio_result jio_csv_get_column(const jio_csv_data* data, uint32_t index, const jio_csv_column** pp_column)
{
    JDM_ENTER_FUNCTION;
    if (!pp_column)
    {
        JDM_ERROR("Pointer where to store the output was null");
        JDM_LEAVE_FUNCTION;
        return JIO_RESULT_NULL_ARG;
    }
    if (data->column_count <= index)
    {
        JDM_ERROR("Requested column at index %u from csv that has %u columns", index, data->column_count);
        JDM_LEAVE_FUNCTION;
        return JIO_RESULT_BAD_INDEX;
    }
    *pp_column = data->columns + index;
    JDM_LEAVE_FUNCTION;
    return JIO_RESULT_SUCCESS;
}

jio_result jio_csv_get_column_by_name(const jio_csv_data* data, const char* name, const jio_csv_column** pp_column)
{
    JDM_ENTER_FUNCTION;
    jio_result res;
    if (!data || !name || !pp_column)
    {
        if (!data)
        {
            JDM_ERROR("Csv file was not provided");
        }
        if (!name)
        {
            JDM_ERROR("Header name was not provided");
        }
        if (!pp_column)
        {
            JDM_ERROR("Output pointer was not provided");
        }
        JDM_LEAVE_FUNCTION;
        return JIO_RESULT_NULL_ARG;
    }

    uint32_t idx = UINT32_MAX;
    for (uint32_t i = 0; i < data->column_count; ++i)
    {
        const jio_csv_column* column = data->columns + i;
        if (string_segment_equal_str(&column->header, name))
        {
#ifndef NDEBUG
            if (idx != UINT32_MAX)
            {
                JDM_WARN("More than one column has a header \"%s\" (%u and %u)", name, idx, i);
            }
            else
            {
                idx = i;
            }
#else
            idx = i;
            break;
#endif
        }
    }

    if (idx == UINT32_MAX)
    {
        JDM_ERROR("Csv file has no header that matches \"%s\"", name);
        res = JIO_RESULT_BAD_CSV_HEADER;
        goto failed;
    }

    res = jio_csv_get_column(data, idx, pp_column);

    JDM_LEAVE_FUNCTION;
    return res;
failed:
    JDM_LEAVE_FUNCTION;
    return res;
}

jio_result jio_csv_get_column_by_name_segment(
        const jio_csv_data* data, const jio_string_segment* name, const jio_csv_column** pp_column)
{
    JDM_ENTER_FUNCTION;
    jio_result res;

    if (!data || !name || !pp_column)
    {
        if (!data)
        {
            JDM_ERROR("Csv file was not provided");
        }
        if (!name)
        {
            JDM_ERROR("Header name was not provided");
        }
        if (!pp_column)
        {
            JDM_ERROR("Output pointer was not provided");
        }
        JDM_LEAVE_FUNCTION;
        return JIO_RESULT_NULL_ARG;
    }

    uint32_t idx = UINT32_MAX;
    for (uint32_t i = 0; i < data->column_count; ++i)
    {
        const jio_csv_column* column = data->columns + i;
        if (string_segment_equal(&column->header, name))
        {
#ifndef NDEBUG
            if (idx != UINT32_MAX)
            {
                JDM_WARN("More than one column has a header \"%.*s\" (%u and %u)", (int)name->len, name->begin, idx, i);
            }
            else
            {
                idx = i;
            }
#else
            idx = i;
            break;
#endif
        }
    }

    if (idx == UINT32_MAX)
    {
        JDM_ERROR("Csv file has no header that matches \"%.*s\"", (int)name->len, name->begin);
        res = JIO_RESULT_BAD_CSV_HEADER;
        goto failed;
    }

    res = jio_csv_get_column(data, idx, pp_column);

    JDM_LEAVE_FUNCTION;
    return res;
    failed:
    JDM_LEAVE_FUNCTION;
    return res;
}

jio_result
jio_csv_add_rows(jio_csv_data* data, uint32_t position, uint32_t row_count, const jio_string_segment* const* rows)
{
    JDM_ENTER_FUNCTION;
    jio_result res = JIO_RESULT_SUCCESS;
    if (!data || !rows)
    {
        if (!data)
        {
            JDM_ERROR("Csv file was not provided");
        }
        if (!rows)
        {
            JDM_ERROR("Rows were not provided");
        }
        JDM_LEAVE_FUNCTION;
        return JIO_RESULT_NULL_ARG;
    }

    if (position > data->column_length && position != UINT32_MAX)
    {
        JDM_ERROR("Rows were to be inserted at position %"PRIu32", but csv data has only %u rows", position, data->column_length);
        res = JIO_RESULT_BAD_INDEX;
        goto end;
    }

    if (data->columns[0].count + row_count >= data->columns[0].capacity)
    {
        //  Need to resize all columns to fit all columns
        const uint32_t new_capacity = data->columns[0].capacity + (row_count < 64 ? 64 : row_count);
        for (uint32_t i = 0; i < data->column_count; ++i)
        {
            jio_string_segment* const new_ptr = jrealloc(data->allocator, data->columns[i].elements, new_capacity * sizeof(*new_ptr));
            if (!new_ptr)
            {
                JDM_ERROR("Could not reallocate column %u to fit additional %u row elements", i, row_count);
                res = JIO_RESULT_BAD_ALLOC;
                goto end;
            }
            data->columns[i].elements = new_ptr;
            data->columns[i].capacity = new_capacity;
        }
    }

    //  Now move the old data out of the way of the new if needed
    if (position == UINT32_MAX)
    {
        //  Appending
        position = data->column_length;
    }
    else if (position != data->column_length)
    {
        //  Moving
        for (u32 i = 0; i < data->column_count; ++i)
        {
            const jio_csv_column* const column = data->columns + i;
            jio_string_segment* const elements = column->elements;
            memmove(elements + position + row_count, elements + position, sizeof(*elements) * (column->count - position));
        }
    }

    //  Inserting the new elements and correcting the column lengths
    for (u32 i = 0; i < data->column_count; ++i)
    {
        jio_csv_column* const column = data->columns + i;
        jio_string_segment* const elements = column->elements;
        for (u32 j = 0; j < row_count; ++j)
        {
            elements[position + j] = rows[i][j];
        }
        column->count += row_count;
    }
    data->column_length += row_count;



end:
    JDM_LEAVE_FUNCTION;
    return res;
}

jio_result jio_csv_add_cols(jio_csv_data* data, uint32_t position, uint32_t col_count, const jio_csv_column* cols)
{
    JDM_ENTER_FUNCTION;
    jio_result res = JIO_RESULT_SUCCESS;
    if (!data || !cols)
    {
        if (!data)
        {
            JDM_ERROR("Csv file was not provided");
        }
        if (!cols)
        {
            JDM_ERROR("Columns were not provided");
        }
        JDM_LEAVE_FUNCTION;
        return JIO_RESULT_NULL_ARG;
    }

    if (position > data->column_count&& position != UINT32_MAX)
    {
        JDM_ERROR("Columns were to be inserted at position %"PRIu32", but csv data has only %u columns", position, data->column_count);
        res = JIO_RESULT_BAD_INDEX;
        goto end;
    }
    bool bad = false;
    for (uint32_t i = 0; i < col_count; ++i)
    {
        if (cols[i].count != data->column_length)
        {
            JDM_ERROR("Column %u to be inserted had a length of %"PRIu32", but others have the length of %u", i, cols[i].count, data->column_length);
            bad = true;
        }
    }
    if (bad)
    {
        res = JIO_RESULT_BAD_CSV_COLUMN;
        goto end;
    }

    //  Resize the columns array if needed
    if (data->column_count + col_count >= data->column_capacity)
    {
        const uint32_t new_capacity = data->column_capacity + (col_count < 8 ? 8 :  col_count);
        jio_csv_column* const new_ptr = jrealloc(data->allocator, data->columns, sizeof(*new_ptr) * new_capacity);
        if (!new_ptr)
        {
            JDM_ERROR("Could not reallocate memory for column array");
            res = JIO_RESULT_BAD_ALLOC;
            goto end;
        }
        data->columns = new_ptr;
        data->column_capacity = new_capacity;
    }

    //  Now move the old data out of the way of the new if needed
    if (position == UINT32_MAX)
    {
        //  Appending
        position = data->column_count;
    }
    else if (position != data->column_count)
    {
        //  Moving
        memmove(data->columns + position + col_count, data->columns + position, sizeof(*data->columns) * (data->column_count - position));
    }

    //  Now insert the new data
    memcpy(data->columns + position, cols, sizeof(*cols) * col_count);

    data->column_count += col_count;
end:
    JDM_LEAVE_FUNCTION;
    return res;
}

jio_result jio_csv_remove_rows(jio_csv_data* data, uint32_t position, uint32_t row_count)
{
    //  Done already?
    if (!row_count || (data && position == data->column_length)) return JIO_RESULT_SUCCESS;
    JDM_ENTER_FUNCTION;
    if (!data)
    {
        JDM_ERROR("Csv file was not provided");
        JDM_LEAVE_FUNCTION;
        return JIO_RESULT_NULL_ARG;
    }
    jio_result res = JIO_RESULT_SUCCESS;
    //  Check that the range to remove lies within the list of available rows
    const uint32_t begin = position, end = position + row_count;
    if (begin > data->column_length || end > data->column_length)
    {
        JDM_ERROR("Range of rows to remove is [%"PRIu32", %"PRIu32"), but the csv file only has %"PRIu32" rows", begin, end, data->column_length);
        res = JIO_RESULT_BAD_INDEX;
        goto end;
    }

    for (uint32_t i = 0; i < data->column_count; ++i)
    {
        jio_csv_column* const column = data->columns + i;
        memmove(column->elements + begin, column->elements + end, sizeof(*column->elements) * (column->count - end));
        assert(column->count >= row_count);
        column->count -= row_count;
    }
    data->column_length -= row_count;
end:
    JDM_LEAVE_FUNCTION;
    return res;
}

jio_result jio_csv_remove_cols(jio_csv_data* data, uint32_t position, uint32_t col_count)
{
    //  Done already?
    if (!col_count || (data && position == data->column_count)) return JIO_RESULT_SUCCESS;
    JDM_ENTER_FUNCTION;
    if (!data)
    {
        JDM_ERROR("Csv file was not provided");
        JDM_LEAVE_FUNCTION;
        return JIO_RESULT_NULL_ARG;
    }
    jio_result res = JIO_RESULT_SUCCESS;
    //  Check that the range to remove lies within the list of available rows
    const uint32_t begin = position, end = position + col_count;
    if (begin > data->column_count || end > data->column_count)
    {
        JDM_ERROR("Range of columns to remove is [%"PRIu32", %"PRIu32"), but the csv file only has %"PRIu32" columns", begin, end, data->column_count);
        res = JIO_RESULT_BAD_INDEX;
        goto end;
    }

    for (uint32_t i = begin; i < end; ++i)
    {
        jio_csv_column* const column = data->columns + i;
        jfree(data->allocator, column->elements);
    }
    memmove(data->columns + begin, data->columns + end, sizeof(*data->columns) * (data->column_count - end));
    data->column_count -= col_count;
end:
    JDM_LEAVE_FUNCTION;
    return res;
}

jio_result jio_csv_replace_cols(
        jio_csv_data* data, uint32_t begin, uint32_t count, uint32_t col_count, const jio_csv_column* cols)
{
    JDM_ENTER_FUNCTION;
    jio_result res = JIO_RESULT_SUCCESS;
    if (!data || !cols)
    {
        if (!data)
        {
            JDM_ERROR("Csv file was not provided");
        }
        if (!cols)
        {
            JDM_ERROR("Columns were not provided");
        }
        res = JIO_RESULT_NULL_ARG;
        goto end;
    }

    const uint32_t end = begin + count;
    if (begin >= data->column_count || end >= data->column_count)
    {
        JDM_ERROR("Columns to be replaced were on the interval [%"PRIu32", %"PRIu32"), but only values on interval [0, %"PRIu32") can be given", begin, end, data->column_count);
        res = JIO_RESULT_BAD_INDEX;
        goto end;
    }

    //  What is the overall change in column count
    const int32_t d_col = (int32_t)col_count - (int32_t)count;
    //  Reallocate the memory for all the new columns
    if (d_col + data->column_count >= data->column_capacity)
    {
        const uint32_t new_capacity = data->column_capacity + (d_col < 8 ? 8 : d_col);
        jio_csv_column* const columns = jrealloc(data->allocator, data->columns, sizeof(*data->columns) * new_capacity);
        if (!columns)
        {
            JDM_ERROR("Could not reallocate memory for columns");
            res = JIO_RESULT_BAD_ALLOC;
            goto end;
        }
        data->columns = columns;
        data->column_capacity = new_capacity;
    }

    for (uint32_t i = begin; i < end; ++i)
    {
        jfree(data->allocator, data->columns[i].elements);
    }
    if (end != data->column_count)
    {
        //  This if is not really needed, I just don't want a call to memmove if not needed
        memmove(data->columns + begin + col_count, data->columns + end, sizeof(*data->columns) * (data->column_count - end));
    }
    //  Insert the new columns
    memcpy(data->columns + begin, cols, sizeof(*cols) * col_count);
    data->column_count += d_col;
end:
    JDM_LEAVE_FUNCTION;
    return res;
}

jio_result jio_csv_replace_rows(
        jio_csv_data* data, uint32_t begin, uint32_t count, uint32_t row_count, const jio_string_segment* const* rows)
{    JDM_ENTER_FUNCTION;
    jio_result res = JIO_RESULT_SUCCESS;
    if (!data || !rows)
    {
        if (!data)
        {
            JDM_ERROR("Csv file was not provided");
        }
        if (!rows)
        {
            JDM_ERROR("Rows were not provided");
        }
        res = JIO_RESULT_NULL_ARG;
        goto end;
    }

    const uint32_t end = begin + count;
    if (begin >= data->column_length || end >= data->column_length)
    {
        JDM_ERROR("Rows to be replaced were on the interval [%"PRIu32", %"PRIu32"), but only values on interval [0, %"PRIu32") can be given", begin, end, data->column_count);
        res = JIO_RESULT_BAD_INDEX;
        goto end;
    }

    //  What is the overall change in column count
    const int32_t d_row = (int32_t)row_count - (int32_t)count;
    //  Reallocate the memory for all the new columns
    if (d_row + data->column_length >= data->columns[0].capacity)
    {
        for (uint32_t i = 0; i < data->column_count; ++i)
        {
            jio_csv_column* const column = data->columns + i;
            const uint32_t new_capacity = column->capacity + (d_row < 8 ? 8 : d_row);
            jio_string_segment* const elements = jrealloc(data->allocator, column->elements, sizeof(*column->elements) * new_capacity);
            if (!elements)
            {
                JDM_ERROR("Could not reallocate memory for column elements");
                res = JIO_RESULT_BAD_ALLOC;
                goto end;
            }
            column->elements = elements;
            column->capacity = new_capacity;
        }
    }

    for (uint32_t i = 0; i < data->column_count; ++i)
    {
        jio_csv_column* const column = data->columns + i;
        memmove(column->elements + begin + row_count, column->elements + end, sizeof(*column->elements) * (data->column_length - end));
        //  Insert the new elements for each row
        for (uint32_t j = 0; j < row_count; ++j)
        {
            column->elements[j + begin] = rows[i][j];
        }
        column->count += d_row;
    }
    data->column_length += d_row;

    end:
    JDM_LEAVE_FUNCTION;
    return res;
}

jio_result jio_csv_column_index(const jio_csv_data* data, const jio_csv_column* column, uint32_t* p_idx)
{
    JDM_ENTER_FUNCTION;
    jio_result res = JIO_RESULT_SUCCESS;
    if (!data || !column || !p_idx)
    {
        if (!data)
        {
            JDM_ERROR("Csv file was not provided");
        }
        if (!column)
        {
            JDM_ERROR("Column was not provided");
        }
        if (!p_idx)
        {
            JDM_ERROR("Output pointer was not provided");
        }
        res = JIO_RESULT_NULL_ARG;
        goto end;
    }

    const uintptr_t idx = column - data->columns;
    if (idx < data->column_count)
    {
        *p_idx = idx;
    }
    else
    {
        JDM_ERROR("Column does not belong to the csv file");
        res = JIO_RESULT_BAD_PTR;
    }

end:
    JDM_LEAVE_FUNCTION;
    return res;
}

jio_result jio_process_csv_exact(
        const jio_memory_file* mem_file, const char* separator, uint32_t column_count, const jio_string_segment* headers,
        bool (** converter_array)(jio_string_segment*, void*), void** param_array, linear_jallocator* lin_allocator)
{
    JDM_ENTER_FUNCTION;
    if (!mem_file || !lin_allocator || !converter_array || !param_array)
    {
        if (!mem_file)
        {
            JDM_ERROR("Memory mapped file to parse was not provided");
        }
        if (!lin_allocator)
        {
            JDM_ERROR("Linear allocator was not provided");
        }
        if (!converter_array)
        {
            JDM_ERROR("Converter array was not provided");
        }
        if (!param_array)
        {
            JDM_ERROR("Parameter array was not provided");
        }
        JDM_LEAVE_FUNCTION;
        return JIO_RESULT_NULL_ARG;
    }
    {
        bool converters_complete = true;
        for (uint32_t i = 0; i < column_count; ++i)
        {
            if (converter_array[i] != NULL)
            {
                converters_complete = false;
                JDM_ERROR("Converter for column %u (%.*s) was not provided", i, (int)headers[i].len, headers[i].begin);
            }
        }
        if (!converters_complete)
        {
            JDM_LEAVE_FUNCTION;
            return JIO_RESULT_BAD_CONVERTER;
        }
    }
    jio_result res;
    void* const base = lin_jalloc_get_current(lin_allocator);

    //  Parse the first row
    const char* row_begin = mem_file->ptr;
    const char* row_end = strchr(row_begin, '\n');

    //  Count columns in the csv file
    size_t sep_len = strlen(separator);
    {
        const u32 real_column_count = count_row_entries(
                row_begin, row_end ?: mem_file->ptr + mem_file->file_size, separator, sep_len);
        if (real_column_count == column_count)
        {
            JDM_ERROR("Csv file has %"PRIu32" columns, but %"PRIu32" were specified", real_column_count, column_count);
            res = JIO_RESULT_BAD_CSV_HEADER;
            goto end;
        }
    }
    jio_string_segment* segments = lin_jalloc(lin_allocator, sizeof(*segments) * column_count);
    if (!segments)
    {
        res = JIO_RESULT_BAD_ALLOC;
        JDM_ERROR("Could not allocate memory for csv parsing");
        goto end;
    }

    uint32_t row_count = 0;
    while (row_end)
    {
        if ((res = extract_row_entries(column_count, row_begin, row_end, separator, sep_len, false, segments)))
        {
            JDM_ERROR("Failed parsing row %u of CSV file \"%s\", reason: %s", row_count + 1, mem_file->name, jio_result_to_str(res));
            goto end;
        }
        for (uint32_t i = 0; i < column_count; ++i)
        {
            if (!converter_array[i](segments + i, param_array[i]))
            {
                JDM_ERROR("Element %"PRIu32" in row %"PRIu32" could not be converted", i + 1, row_count + 1);
                res = JIO_RESULT_BAD_VALUE;
                goto end;
            }
        }

        row_count += 1;
        //  Move to the next row
        row_begin = row_end + 1;
        row_end = strchr(row_end + 1, '\n');
    }

    lin_jfree(lin_allocator, segments);
    JDM_LEAVE_FUNCTION;
    return JIO_RESULT_SUCCESS;
    end:
    lin_jalloc_set_current(lin_allocator, base);
    JDM_LEAVE_FUNCTION;
    return res;
}

jio_result jio_csv_print_size(
        const jio_csv_data* const data, size_t* const p_size, const size_t separator_length, const uint32_t extra_padding, const bool same_width)
{
    JDM_ENTER_FUNCTION;
    jio_result res = JIO_RESULT_SUCCESS;
    if (!data || !p_size)
    {
        if (!data)
        {
            JDM_ERROR("Csv file was not provided");
        }
        if (!p_size)
        {
            JDM_ERROR("Output file was not provided");
        }
        res = JIO_RESULT_NULL_ARG;
        goto end;
    }


    size_t total_chars = 0;
    if (!same_width)
    {
        for (uint32_t i = 0; i < data->column_count; ++i)
        {
            const jio_csv_column* const column = data->columns + i;
            total_chars += column->header.len;

            for (uint32_t j = 0; j < column->count; ++j)
            {
                total_chars += column->elements[j].len;
            }
        }

        //  characters needed to pad entries
        total_chars += (extra_padding) * (data->column_length + 1) * data->column_count;
    }
    else
    {
        // Find the maximum width of any entry
        uint32_t width = 0;
        for (uint32_t i = 0; i < data->column_count; ++i)
        {
            const jio_csv_column* const column = data->columns + i;
            if (column->header.len > width)
            {
                width = column->header.len;
            }
            for (uint32_t j = 0; j < column->count; ++j)
            {
                if (column->elements[j].len > width)
                {
                    width = column->elements[j].len;
                }
            }
        }
        //  characters needed to fit entries
        total_chars = (width + extra_padding) * (data->column_length + 1) * data->column_count;
    }
    //  characters needed for separators
    total_chars += (data->column_count - 1) * (data->column_length + 1) * separator_length;
    //  Characters needed for new line characters
    total_chars += (data->column_length);

    *p_size = total_chars + 1;

end:
    JDM_LEAVE_FUNCTION;
    return res;
}

static size_t print_entry(char* restrict buffer, const char* restrict src, size_t n, uint32_t extra_padding, uint32_t min_width, bool align_left)
{
    uint32_t same_pad = 0;
    if (n < min_width)
    {
        same_pad = min_width - n;
    }
    if (align_left)
    {
        memcpy(buffer, src, n);
        memset(buffer + n, ' ', same_pad + extra_padding);
    }
    else
    {
        memset(buffer, ' ', same_pad + extra_padding);
        memcpy(buffer + same_pad + extra_padding, src, n);
    }
    return same_pad + extra_padding + n;
}

jio_result jio_csv_print(
        const jio_csv_data* data, size_t* p_usage, char* restrict buffer, const char* separator, uint32_t extra_padding, bool same_width,
        bool align_left)
{
    JDM_ENTER_FUNCTION;
    jio_result res = JIO_RESULT_SUCCESS;
    if (!data || !buffer || !separator)
    {
        if (!data)
        {
            JDM_ERROR("Csv file was not provided");
        }
        if (!buffer)
        {
            JDM_ERROR("Output buffer was not provided");
        }
        if (!separator)
        {
            JDM_ERROR("Separator was not provided");
        }
        res = JIO_RESULT_NULL_ARG;
        goto end;
    }
    size_t min_width = 0;
    if (same_width)
    {
        for (uint32_t i = 0; i < data->column_count; ++i)
        {
            const jio_csv_column* const column = data->columns + i;
            if (column->header.len > min_width)
            {
                min_width = column->header.len;
            }
            for (uint32_t j = 0; j < column->count; ++j)
            {
                if (column->elements[j].len > min_width)
                {
                    min_width = column->elements[j].len;
                }
            }
        }
    }

    const size_t sep_len = strlen(separator);
    //  Print headers
    char* pos = buffer;
    pos += print_entry(pos, data->columns[0].header.begin, data->columns[0].header.len, extra_padding, min_width, align_left);
    for (uint32_t i = 1; i < data->column_count; ++i)
    {
        memcpy(pos, separator, sep_len); pos += sep_len;
        pos += print_entry(pos, data->columns[i].header.begin, data->columns[i].header.len, extra_padding, min_width, align_left);
    }
    *pos = '\n'; ++pos;

    //  Now to print all entries to the buffer
    for (uint32_t i = 0; i < data->column_length; ++i)
    {
        pos += print_entry(pos, data->columns[0].elements[i].begin, data->columns[0].elements[i].len, extra_padding, min_width, align_left);
        for (uint32_t j = 1; j < data->column_count; ++j)
        {
            const jio_string_segment segment = data->columns[j].elements[i];
            memcpy(pos, separator, sep_len); pos += sep_len;
            pos += print_entry(pos, segment.begin, segment.len, extra_padding, min_width, align_left);
        }
        *pos = '\n'; ++pos;
    }
    *pos = 0;

    if (p_usage)
    {
        *p_usage = pos - buffer;
    }

end:
    JDM_LEAVE_FUNCTION;
    return res;
}
