#ifndef CJDC_H
#define CJDC_H 1

#define CLASS_FILE_MAGIC (0xCAFEBABE)

typedef uint8_t u1_t;
typedef uint16_t u2_t;
typedef uint32_t u4_t;

typedef enum constant_pool_tags_e {
    CONSTANT_CLASS			= 7,
    CONSTANT_FIELDREF			= 9,
    CONSTANT_METHODREF			= 10,
    CONSTANT_INTERFACE_METHODREF	= 11,
    CONSTANT_STRING			= 8,
    CONSTANT_INTEGER			= 3,
    CONSTANT_FLOAT			= 4,
    CONSTANT_LONG			= 5,
    CONSTANT_DOUBLE			= 6,
    CONSTANT_NAME_AND_TYPE		= 12,
    CONSTANT_UTF8			= 1,
    CONSTANT_METHOD_HANDLE		= 15,
    CONSTANT_METHOD_TYPE		= 16,
    CONSTANT_INVOKE_DYNAMIC		= 18
} constant_pool_tags_t;

#define ACC_PUBLIC(x)      ((x) & 0x0001)
#define ACC_FINAL(x)       ((x) & 0x0010)
#define ACC_SUPER(x)       ((x) & 0x0020)
#define ACC_INTERFACE(x)   ((x) & 0x0200)
#define ACC_ABSTRACT(x)    ((x) & 0x0400)
#define ACC_SYNTHETIC(x)   ((x) & 0x1000)
#define ACC_ANNOTATION(x)  ((x) & 0x2000)
#define ACC_ENUM(x)        ((x) & 0x4000)

typedef struct constant_pool_class_s {
    u2_t name_index;
} constant_pool_class_t;

/* fieldref, methodref, interface_methodref */
typedef struct constant_pool_ref_s {
    u2_t class_index;
    u2_t name_and_type_index;
} constant_pool_ref_t;

typedef struct constant_pool_string_s {
    u2_t name_index;
} constant_pool_string_t;

/* integer, float */
typedef struct constant_pool_number4_s {
    u2_t name_index;
    u4_t bytes;
} constant_pool_number4_t;

/* long, double */
typedef struct constant_pool_number8_s {
    u2_t name_index;
    u4_t high_bytes;
    u4_t low_bytes;
} constant_pool_number8_t;

typedef struct constant_pool_name_and_type_s {
    u2_t name_index;
    u2_t descriptor_index;
} constant_pool_name_and_type_t;

typedef struct constant_pool_utf8_s {
    u2_t length;
    u1_t *bytes;
} constant_pool_utf8_t;

typedef struct constant_pool_method_handle_s {
    u1_t reference_kind;
    u2_t reference_index;
} constant_pool_method_handle_t;

typedef struct constant_pool_method_type_s {
    u2_t descriptor_index;
} constant_pool_method_type_t;

typedef struct constant_pool_invoke_dynamic_s {
    u2_t bootstrap_method_attr_index;
    u2_t name_and_type_index;
} constant_pool_invoke_dynamic_t;



typedef struct cp_info_s {
    u1_t tag;
    union {
        constant_pool_class_t cp_class_info;
        constant_pool_ref_t cp_fieldref;
        constant_pool_ref_t cp_methodref;
        constant_pool_ref_t cp_interface_methodref;
        constant_pool_string_t cp_string;
        constant_pool_number4_t cp_integer;
        constant_pool_number4_t cp_float;
        constant_pool_number8_t cp_long;
        constant_pool_number8_t cp_double;
        constant_pool_name_and_type_t cp_name_and_type;
        constant_pool_utf8_t cp_utf8;
        constant_pool_method_handle_t cp_method_handle;
        constant_pool_method_type_t cp_method_type;
        constant_pool_invoke_dynamic_t cp_invoke_dynamic;
    } u;
} cp_info_t;

typedef struct attribute_info_s {
    u2_t attribute_name_index;
    u4_t attribute_length;
    u1_t *info;
} attribute_info_t;

typedef struct field_info_s {
    u2_t access_flags;
    u2_t name_index;
    u2_t descriptor_index;
    u2_t attributes_count;
    attribute_info_t *attributes;
} field_info_t;

typedef struct method_info_s {
    u2_t access_flags;
    u2_t name_index;
    u2_t descriptor_index;
    u2_t attributes_count;
    attribute_info_t *attributes;
} method_info_t;

typedef struct class_file_s {
    u4_t magic;
    u2_t minor_version;
    u2_t major_version;
    u2_t constant_pool_count;
    cp_info_t *constant_pool;  /* [constant_pool_count-1] */
    u2_t access_flags;
    u2_t this_class;
    u2_t super_class;
    u2_t interfaces_count;
    u2_t *interfaces;
    u2_t fields_count;
    field_info_t *fields;
    u2_t methods_count;
    method_info_t *methods;
    u2_t attributes_count;
    attribute_info_t *attributes;
} class_file_t;


#endif
