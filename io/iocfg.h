//
// Created by jan on 4.6.2023.
//

#ifndef JIO_INI_PARSING_H
#define JIO_INI_PARSING_H
#include "iobase.h"

typedef enum jio_cfg_type_enum jio_cfg_type;
enum jio_cfg_type_enum
{
    JIO_CFG_TYPE_BOOLEAN,
    JIO_CFG_TYPE_INT,
    JIO_CFG_TYPE_REAL,
    JIO_CFG_TYPE_STRING,
    JIO_CFG_TYPE_ARRAY,
};

typedef struct jio_cfg_value_struct jio_cfg_value;
typedef struct jio_cfg_array_struct jio_cfg_array;
struct jio_cfg_array_struct
{
    jallocator* allocator;
    uint32_t capacity;
    uint32_t count;
    jio_cfg_value* values;
};

struct jio_cfg_value_struct
{
    jio_cfg_type type;
    union
    {
        bool value_boolean;
        intmax_t value_int;
        double_t value_real;
        jio_string_segment value_string;
        jio_cfg_array value_array;
    };
};

typedef struct jio_cfg_element_struct jio_cfg_element;
struct jio_cfg_element_struct
{
    jio_string_segment key;
    jio_cfg_value value;
};

typedef struct jio_cfg_section_struct jio_cfg_section;
struct jio_cfg_section_struct
{
    jallocator* allocator;
    jio_string_segment name;
    uint32_t value_count;
    uint32_t value_capacity;
    jio_cfg_element* value_array;
    uint32_t subsection_count;
    uint32_t subsection_capacity;
    jio_cfg_section** subsection_array;
};

jio_result jio_cfg_section_insert(jio_cfg_section* parent, jio_cfg_section* child);

jio_result jio_cfg_element_insert(jio_cfg_section* section, jio_cfg_element element);

jio_result jio_cfg_section_create(jallocator* allocator, jio_string_segment name, jio_cfg_section** pp_out);

jio_result jio_cfg_section_destroy(jio_cfg_section* section);

jio_result jio_cfg_parse(const jio_memory_file* mem_file, jio_cfg_section** pp_root_section, jallocator* allocator);

jio_result jio_cfg_get_value_by_key(const jio_cfg_section* section, const char* key, jio_cfg_value* p_value);

jio_result jio_cfg_get_value_by_key_segment(const jio_cfg_section* section, jio_string_segment key, jio_cfg_value* p_value);

jio_result jio_cfg_get_subsection(const jio_cfg_section* section, const char* subsection_name, jio_cfg_section** pp_out);

jio_result jio_cfg_get_subsection_segment(
        const jio_cfg_section* section, jio_string_segment subsection_name, jio_cfg_section** pp_out);

jio_result jio_cfg_print_size(const jio_cfg_section* restrict section, size_t delim_size, size_t* restrict p_size, bool indent_subsections, bool equalize_key_length_pad);

jio_result jio_cfg_print(const jio_cfg_section* restrict section, char* restrict buffer, const char* restrict delimiter, size_t* restrict p_real_size, bool indent_subsections, bool equalize_key_length_pad, bool pad_left);

#endif //JIO_INI_PARSING_H
