//
// Created by jan on 4.6.2023.
//

#ifndef JIO_PARSING_BASE_H
#define JIO_PARSING_BASE_H

#include "iobase.h"

typedef struct jio_xml_element_struct jio_xml_element;
struct jio_xml_element_struct
{
    uint32_t depth;
    jio_string_segment name;
    uint32_t attrib_capacity;
    uint32_t attrib_count;
    jio_string_segment* attribute_names;
    jio_string_segment* attribute_values;
    uint32_t child_count;
    uint32_t child_capacity;
    jio_xml_element* children;
    jio_string_segment value;
    jallocator* allocator;
};

jio_result jio_xml_release(jio_xml_element* root);

jio_result jio_xml_parse(const jio_memory_file* mem_file, jio_xml_element* p_root, jallocator* allocator);

jio_result rmod_serialize_xml(jio_xml_element* root, FILE* f_out);




#endif //JIO_PARSING_BASE_H
