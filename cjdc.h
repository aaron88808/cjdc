#ifndef CJDC_H
#define CJDC_H 1

#define CLASS_FILE_MAGIC (0xCAFEBABE)

/* class access and property modifiers */
#define ACC_PUBLIC	(0x0001)  /* Declared public; may be accessed from outside its package.                         */
#define ACC_FINAL	(0x0010)  /* Declared final; no subclasses allowed.						*/
#define ACC_SUPER	(0x0020)  /* Treat superclass methods specially when invoked by the invokespecial instruction.	*/
#define ACC_INTERFACE	(0x0200)  /* Is an interface, not a class.							*/
#define ACC_ABSTRACT	(0x0400)  /* Declared abstract; must not be instantiated.					*/
#define ACC_SYNTHETIC	(0x1000)  /* Declared synthetic; not present in the source code.				*/
#define ACC_ANNOTATION  (0x2000)  /* Declared as an annotation type.							*/
#define ACC_ENUM	(0x4000)  /* Declared as an enum type.                                                          */


typedef unsigned char u1_t;
typedef unsigned short u2_t;
typedef unsigned int u4_t;

typedef struct cp_info_s {
    u1_t tag;
    u1_t *info;
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
