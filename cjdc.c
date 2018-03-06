#include <stdio.h>
#include <string.h>
#include <unistr.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>
#include <unistdio.h>

#include "cjdc.h"

static char *program = NULL;

static char *get_basename(char *path); 
static int open_class_file(const char *class_file_name);
static class_file_t *read_class_file(int fd);

static int read_constant_pool_element(int fd, cp_info_t *constant_pool_element);
static int read_constant_class(int fd, cp_info_t *constant_pool_element);
static int read_constant_fieldref(int fd, cp_info_t *constant_pool_element);
static int read_constant_methodref(int fd, cp_info_t *constant_pool_element);
static int read_constant_interface_methodref(int fd, cp_info_t *constant_pool_element);
static int read_constant_string(int fd, cp_info_t *constant_pool_element);
static int read_constant_integer(int fd, cp_info_t *constant_pool_element);
static int read_constant_float(int fd, cp_info_t *constant_pool_element);
static int read_constant_long(int fd, cp_info_t *constant_pool_element);
static int read_constant_double(int fd, cp_info_t *constant_pool_element);
static int read_constant_name_and_type(int fd, cp_info_t *constant_pool_element);
static int read_constant_utf8(int fd, cp_info_t *constant_pool_element);
static int read_constant_method_handle(int fd, cp_info_t *constant_pool_element);
static int read_constant_method_type(int fd, cp_info_t *constant_pool_element);
static int read_constant_invoke_dynamic(int fd, cp_info_t *constant_pool_element);


static void print_class_file (class_file_t *class_file);
static void print_constant_pool(class_file_t *class_file);
static void print_constant_pool_element(int i, cp_info_t *constant_pool_element);
static void print_access_flags(class_file_t *class_file);

static int read_bytes(int fd, void *buffer, int requested);
static int read_bytes_or_error(int fd, void *buffer, int requested, int so_far);

int main(int ac, char **av) {
    program = get_basename(av[0]);
    if (ac < 2) {
	fprintf(stderr, "usage: %s {.class-file-name}\n", program);
	exit(1);
    }
    char *class_file_name = av[1];

    int fd = open_class_file(class_file_name);
    if (fd < 0) {
	fprintf(stderr, "%s: exiting on failure to open file '%s'.\n", program, class_file_name);
	exit(1);
    }
    
    class_file_t *class_file = read_class_file(fd);
    if (class_file == NULL) {
	fprintf(stderr, "%s: failed to read class file '%s'.\n", program, class_file_name);
	exit(1);
    }

    int rc = close(fd);
    if (rc < 0) {
	fprintf(stderr, "%s: failed to close file '%s': %s.\n", program, class_file_name, strerror(errno));
	exit(1);
    }

    print_class_file(class_file);

    uint8_t *utf8_str = NULL;

    return 0;
}

static int open_class_file(const char *class_file_name) {
    int result = open(class_file_name, O_RDONLY);
    if (result < 0) {
	fprintf(stderr, "%s: failed to open '%s': %s.\n", program, class_file_name, strerror(errno));
    }
    return result;
}

static class_file_t *read_class_file(int fd) {
    class_file_t *result = malloc(sizeof(class_file_t));
    if (result == NULL) {
	fprintf(stderr, "%s: failed to malloc %d bytes.", program, sizeof(class_file_t));
	goto ERR_RETURN;
    }
    memset(result, 0, sizeof(class_file_t));

    if (read_bytes(fd, &(result->magic), sizeof(result->magic)) < 0) {
	fprintf(stderr, "%s: failed to read magic number\n", program);
	goto ERR_RETURN;
    }
    result->magic = ntohl(result->magic);

    if (read_bytes(fd, &(result->minor_version), sizeof(result->minor_version)) < 0) {
	fprintf(stderr, "%s: failed to read minor version\n", program);
	goto ERR_RETURN;
    }
    result->minor_version = ntohs(result->minor_version);

    if (read_bytes(fd, &(result->major_version), sizeof(result->major_version)) < 0) {
	fprintf(stderr, "%s: failed to read major version\n", program);
	goto ERR_RETURN;
    }
    result->major_version = ntohs(result->major_version);

    if (read_bytes(fd, &(result->constant_pool_count), sizeof(result->constant_pool_count)) < 0) {
	fprintf(stderr, "%s: failed to read constant_pool_count\n", program);
	goto ERR_RETURN;
    }
    result->constant_pool_count = ntohs(result->constant_pool_count);
    if (result->constant_pool_count) {
	result->constant_pool = calloc(result->constant_pool_count, sizeof(cp_info_t));
	if (result->constant_pool == NULL) {
	    fprintf(stderr, "%s: failed to allocate array of %ud constant pool elements", program, result->constant_pool_count);
	    goto ERR_RETURN;
	}
    }

    cp_info_t *constant_pool_element = result->constant_pool;
    int i;
    for (i = 1; i < result->constant_pool_count; i++) {
	if (read_constant_pool_element(fd, constant_pool_element++) < 0) {
	    fprintf(stderr, "%s: failed to read constant pool element %d\n", program, i);
	    goto ERR_RETURN;
	}
    }
    
    if (read_bytes(fd, &(result->access_flags), sizeof(result->access_flags)) < 0) {
	fprintf(stderr, "%s: failed to read access_flags\n", program);
	goto ERR_RETURN;
    }
    result->access_flags = ntohs(result->access_flags);

    return result;

ERR_RETURN:
    if (result) {
	if (result->constant_pool) {
	    free(result->constant_pool);
	}
	free(result);
    }
    return NULL;
}

static int read_constant_pool_element(int fd, cp_info_t *constant_pool_element) {
    if (read_bytes(fd, &(constant_pool_element->tag), sizeof(constant_pool_element->tag)) < 0) {
	fprintf(stderr, "%s: failed to read constant pool element tag", program);
	return -1;
    }

    int result = 0;
    switch (constant_pool_element->tag) {
    case CONSTANT_CLASS:
	result = read_constant_class(fd, constant_pool_element);
	break;
    case CONSTANT_FIELDREF:
	result = read_constant_fieldref(fd, constant_pool_element);
	break;
    case CONSTANT_METHODREF:
	result = read_constant_methodref(fd, constant_pool_element);
	break;
    case CONSTANT_INTERFACE_METHODREF:
	result = read_constant_interface_methodref(fd, constant_pool_element);
	break;
    case CONSTANT_STRING:
	result = read_constant_string(fd, constant_pool_element);
	break;
    case CONSTANT_INTEGER:
	result = read_constant_integer(fd, constant_pool_element);
	break;
    case CONSTANT_FLOAT:
	result = read_constant_float(fd, constant_pool_element);
	break;
    case CONSTANT_LONG:
	result = read_constant_long(fd, constant_pool_element);
	break;
    case CONSTANT_DOUBLE:
	result = read_constant_double(fd, constant_pool_element);
	break;
    case CONSTANT_NAME_AND_TYPE:
	result = read_constant_name_and_type(fd, constant_pool_element);
	break;
    case CONSTANT_UTF8:
	result = read_constant_utf8(fd, constant_pool_element);
	break;
    case CONSTANT_METHOD_HANDLE:
	result = read_constant_method_handle(fd, constant_pool_element);
	break;
    case CONSTANT_METHOD_TYPE:
	result = read_constant_method_type(fd, constant_pool_element);
	break;
    case CONSTANT_INVOKE_DYNAMIC:
	result = read_constant_invoke_dynamic(fd, constant_pool_element);
	break;
    default:
	fprintf(stderr, "%s: unknown constant pool tag %d\n", program, constant_pool_element->tag);
	result = -1;
	break;
    }
    return result;
}

static int read_constant_class(int fd, cp_info_t *constant_pool_element){
    constant_pool_class_t* cp_class_info = &constant_pool_element->u.cp_class_info;

    if (read_bytes(fd, &cp_class_info->name_index, sizeof(cp_class_info->name_index))) {
	fprintf(stderr, "%s: could not read class constant name index\n", program);
	return -1;
    }
    cp_class_info->name_index = ntohs(cp_class_info->name_index);

    return 0;
}
static int read_constant_fieldref(int fd, cp_info_t *constant_pool_element){
    constant_pool_ref_t* cp_fieldref = &constant_pool_element->u.cp_fieldref;

    if (read_bytes(fd, &cp_fieldref->class_index, sizeof(cp_fieldref->class_index))) {
	fprintf(stderr, "%s: could not read fieldref constant class index\n", program);
	return -1;
    }
    cp_fieldref->class_index = ntohs(cp_fieldref->class_index);

    if (read_bytes(fd, &cp_fieldref->name_and_type_index, sizeof(cp_fieldref->name_and_type_index))) {
	fprintf(stderr, "%s: could not read fieldref constant name-and-type index\n", program);
	return -1;
    }
    cp_fieldref->name_and_type_index = ntohs(cp_fieldref->name_and_type_index);

    return 0;
}
static int read_constant_methodref(int fd, cp_info_t *constant_pool_element){
    constant_pool_ref_t* cp_methodref = &constant_pool_element->u.cp_methodref;

    if (read_bytes(fd, &cp_methodref->class_index, sizeof(cp_methodref->class_index))) {
	fprintf(stderr, "%s: could not read methodref constant class index\n", program);
	return -1;
    }
    cp_methodref->class_index = ntohs(cp_methodref->class_index);

    if (read_bytes(fd, &cp_methodref->name_and_type_index, sizeof(cp_methodref->name_and_type_index))) {
	fprintf(stderr, "%s: could not read methodref constant name-and-type index\n", program);
	return -1;
    }
    cp_methodref->name_and_type_index = ntohs(cp_methodref->name_and_type_index);

    return 0;
}
static int read_constant_interface_methodref(int fd, cp_info_t *constant_pool_element){
    constant_pool_ref_t* cp_interface_methodref = &constant_pool_element->u.cp_interface_methodref;

    if (read_bytes(fd, &cp_interface_methodref->class_index, sizeof(cp_interface_methodref->class_index))) {
	fprintf(stderr, "%s: could not read interface-methodref constant class index\n", program);
	return -1;
    }
    cp_interface_methodref->class_index = ntohs(cp_interface_methodref->class_index);

    if (read_bytes(fd, &cp_interface_methodref->name_and_type_index, sizeof(cp_interface_methodref->name_and_type_index))) {
	fprintf(stderr, "%s: could not read interface-methodref constant name-and-type index\n", program);
	return -1;
    }
    cp_interface_methodref->name_and_type_index = ntohs(cp_interface_methodref->name_and_type_index);

    return 0;
}
static int read_constant_string(int fd, cp_info_t *constant_pool_element){
    constant_pool_string_t* cp_string = &constant_pool_element->u.cp_string;

    if (read_bytes(fd, &cp_string->name_index, sizeof(cp_string->name_index))) {
	fprintf(stderr, "%s: could not read string constant name index\n", program);
	return -1;
    }
    cp_string->name_index = ntohs(cp_string->name_index);

    return 0;
}
static int read_constant_integer(int fd, cp_info_t *constant_pool_element){
    constant_pool_number4_t* cp_integer = &constant_pool_element->u.cp_integer;

    if (read_bytes(fd, &cp_integer->name_index, sizeof(cp_integer->name_index))) {
	fprintf(stderr, "%s: could not read integer constant name index\n", program);
	return -1;
    }
    cp_integer->name_index = ntohs(cp_integer->name_index);

    if (read_bytes(fd, &cp_integer->bytes, sizeof(cp_integer->bytes))) {
	fprintf(stderr, "%s: could not read integer constant bytes\n", program);
	return -1;
    }
    cp_integer->bytes = ntohl(cp_integer->bytes);

    return 0;
}
static int read_constant_float(int fd, cp_info_t *constant_pool_element){
    constant_pool_number4_t* cp_float = &constant_pool_element->u.cp_float;

    if (read_bytes(fd, &cp_float->name_index, sizeof(cp_float->name_index))) {
	fprintf(stderr, "%s: could not read float constant name index\n", program);
	return -1;
    }
    cp_float->name_index = ntohs(cp_float->name_index);

    if (read_bytes(fd, &cp_float->bytes, sizeof(cp_float->bytes))) {
	fprintf(stderr, "%s: could not read float constant bytes\n", program);
	return -1;
    }
    cp_float->bytes = ntohl(cp_float->bytes);

    return 0;
}
static int read_constant_long(int fd, cp_info_t *constant_pool_element){
    constant_pool_number8_t* cp_long = &constant_pool_element->u.cp_long;

    if (read_bytes(fd, &cp_long->name_index, sizeof(cp_long->name_index))) {
	fprintf(stderr, "%s: could not read long constant name index\n", program);
	return -1;
    }
    cp_long->name_index = ntohs(cp_long->name_index);

    if (read_bytes(fd, &cp_long->high_bytes, sizeof(cp_long->high_bytes))) {
	fprintf(stderr, "%s: could not read long constant high-bytes\n", program);
	return -1;
    }
    cp_long->high_bytes = ntohl(cp_long->high_bytes);

    if (read_bytes(fd, &cp_long->low_bytes, sizeof(cp_long->low_bytes))) {
	fprintf(stderr, "%s: could not read long constant low-bytes\n", program);
	return -1;
    }
    cp_long->low_bytes = ntohl(cp_long->low_bytes);

    return 0;
}
static int read_constant_double(int fd, cp_info_t *constant_pool_element){
    constant_pool_number8_t* cp_double = &constant_pool_element->u.cp_double;

    if (read_bytes(fd, &cp_double->name_index, sizeof(cp_double->name_index))) {
	fprintf(stderr, "%s: could not read double constant name index\n", program);
	return -1;
    }
    cp_double->name_index = ntohs(cp_double->name_index);

    if (read_bytes(fd, &cp_double->high_bytes, sizeof(cp_double->high_bytes))) {
	fprintf(stderr, "%s: could not read double constant high-bytes\n", program);
	return -1;
    }
    cp_double->high_bytes = ntohl(cp_double->high_bytes);

    if (read_bytes(fd, &cp_double->low_bytes, sizeof(cp_double->low_bytes))) {
	fprintf(stderr, "%s: could not read double constant low-bytes\n", program);
	return -1;
    }
    cp_double->low_bytes = ntohl(cp_double->low_bytes);

    return 0;
}
static int read_constant_name_and_type(int fd, cp_info_t *constant_pool_element){
    constant_pool_name_and_type_t* cp_name_and_type = &constant_pool_element->u.cp_name_and_type;

    if (read_bytes(fd, &cp_name_and_type->name_index, sizeof(cp_name_and_type->name_index))) {
	fprintf(stderr, "%s: could not read name-and-type constant name index\n", program);
	return -1;
    }
    cp_name_and_type->name_index = ntohs(cp_name_and_type->name_index);

    if (read_bytes(fd, &cp_name_and_type->descriptor_index, sizeof(cp_name_and_type->descriptor_index))) {
	fprintf(stderr, "%s: could not read name-and-type constant descriptor index\n", program);
	return -1;
    }
    cp_name_and_type->descriptor_index = ntohs(cp_name_and_type->descriptor_index);

    return 0;
}
static int read_constant_utf8(int fd, cp_info_t *constant_pool_element){
    constant_pool_utf8_t* cp_utf8 = &constant_pool_element->u.cp_utf8;

    if (read_bytes(fd, &cp_utf8->length, sizeof(cp_utf8->length))) {
	fprintf(stderr, "%s: could not read utf8 constant length\n", program);
	return -1;
    }
    cp_utf8->length = ntohs(cp_utf8->length);

    cp_utf8->bytes = calloc(1 + cp_utf8->length, sizeof(u1_t));
    if (cp_utf8->bytes == NULL) {
	fprintf(stderr, "%s: could not allocate %d bytes for string constant\n", program, 1 + cp_utf8->length);
	return -1;
    }
    
    if (read_bytes(fd, cp_utf8->bytes, cp_utf8->length)) {
	fprintf(stderr, "%s: could not read utf8 constant %d bytes\n", program, cp_utf8->length);
	return -1;
    }

    return 0;
}
static int read_constant_method_handle(int fd, cp_info_t *constant_pool_element){
    constant_pool_method_handle_t* cp_method_handle = &constant_pool_element->u.cp_method_handle;

    if (read_bytes(fd, &cp_method_handle->reference_kind, sizeof(cp_method_handle->reference_kind))) {
	fprintf(stderr, "%s: could not read method-handle constant reference kind\n", program);
	return -1;
    }

    if (read_bytes(fd, &cp_method_handle->reference_index, sizeof(cp_method_handle->reference_index))) {
	fprintf(stderr, "%s: could not read method-handle constant reference index\n", program);
	return -1;
    }
    cp_method_handle->reference_index = ntohs(cp_method_handle->reference_index);

    return 0;
}
static int read_constant_method_type(int fd, cp_info_t *constant_pool_element){
    constant_pool_method_type_t* cp_method_type = &constant_pool_element->u.cp_method_type;

    if (read_bytes(fd, &cp_method_type->descriptor_index, sizeof(cp_method_type->descriptor_index))) {
	fprintf(stderr, "%s: could not read method-type constant descriptor index\n", program);
	return -1;
    }
    cp_method_type->descriptor_index = ntohs(cp_method_type->descriptor_index);

    return 0;
}
static int read_constant_invoke_dynamic(int fd, cp_info_t *constant_pool_element){
    constant_pool_invoke_dynamic_t* cp_invoke_dynamic = &constant_pool_element->u.cp_invoke_dynamic;

    if (read_bytes(fd, &cp_invoke_dynamic->bootstrap_method_attr_index, sizeof(cp_invoke_dynamic->bootstrap_method_attr_index))) {
	fprintf(stderr, "%s: could not read invoke-dynamic constant bootstrap-method-attr index\n", program);
	return -1;
    }
    cp_invoke_dynamic->bootstrap_method_attr_index = ntohs(cp_invoke_dynamic->bootstrap_method_attr_index);

    if (read_bytes(fd, &cp_invoke_dynamic->name_and_type_index, sizeof(cp_invoke_dynamic->name_and_type_index))) {
	fprintf(stderr, "%s: could not read invoke-dynamic constant name-and-type index\n", program);
	return -1;
    }
    cp_invoke_dynamic->name_and_type_index = ntohs(cp_invoke_dynamic->name_and_type_index);

    return 0;
}

static void print_class_file (class_file_t *class_file) {
    if (class_file == NULL) {
	fprintf(stderr, "%s: class_file is NULL\n", program);
	return;
    }
    printf("magic: %x\n", class_file->magic);
    printf("minor_version: %d\n", class_file->minor_version);
    printf("major_version: %d\n", class_file->major_version);
    printf("constant_pool_count: %d\n", class_file->constant_pool_count);
    print_constant_pool(class_file);
    print_access_flags(class_file);
}

static void print_constant_pool(class_file_t *class_file) {
    cp_info_t *constant_pool_element = class_file->constant_pool;

    fprintf(stderr, "%s: CONSTANT POOL:\n", program);
    fprintf(stderr, "%s: ################################################################################:\n", program);

    int i;
    for (i = 1; i < class_file->constant_pool_count; i++) {
	print_constant_pool_element(i, constant_pool_element++);
    }

    fprintf(stderr, "%s: ################################################################################:\n", program);

}

static void print_constant_pool_element(int i, cp_info_t *constant_pool_element) {
    static char hack_utf8buf[65537];
    switch (constant_pool_element->tag) {
    case CONSTANT_CLASS:
	fprintf(stderr, "%s: [%d] CLASS, name_index=%d\n", program, i,
		constant_pool_element->u.cp_class_info.name_index);
	break;
    case CONSTANT_FIELDREF:
	fprintf(stderr, "%s: [%d] FIELDREF, class_index=%d, name_and_type_index=%d\n", program, i,
		constant_pool_element->u.cp_fieldref.class_index,
		constant_pool_element->u.cp_fieldref.name_and_type_index);
	break;
    case CONSTANT_METHODREF:
	fprintf(stderr, "%s: [%d] METHODREF, class_index=%d, name_and_type_index=%d\n", program, i,
		constant_pool_element->u.cp_fieldref.class_index,
		constant_pool_element->u.cp_fieldref.name_and_type_index);
	break;
    case CONSTANT_INTERFACE_METHODREF:
	fprintf(stderr, "%s: [%d] INTERFACE_METHODREF, class_index=%d, name_and_type_index=%d\n", program, i,
		constant_pool_element->u.cp_fieldref.class_index,
		constant_pool_element->u.cp_fieldref.name_and_type_index);
	break;
    case CONSTANT_STRING:
	fprintf(stderr, "%s: [%d] STRING, name_index=%d\n", program, i,
		constant_pool_element->u.cp_string.name_index);
	break;
    case CONSTANT_INTEGER:
	fprintf(stderr, "%s: [%d] INTEGER, name_index=%d, bytes=%x\n", program, i,
		constant_pool_element->u.cp_integer.name_index,
		constant_pool_element->u.cp_integer.bytes);
	break;
    case CONSTANT_FLOAT:
	fprintf(stderr, "%s: [%d] FLOAT, name_index=%d, bytes=%x\n", program, i,
		constant_pool_element->u.cp_integer.name_index,
		constant_pool_element->u.cp_integer.bytes);
	break;
    case CONSTANT_LONG:
	fprintf(stderr, "%s: [%d] LONG, name_index=%d, high_bytes=%x, low_bytes=%x\n", program, i,
		constant_pool_element->u.cp_long.name_index,
		constant_pool_element->u.cp_long.high_bytes,
		constant_pool_element->u.cp_long.low_bytes);
	break;
    case CONSTANT_DOUBLE:
	fprintf(stderr, "%s: [%d] DOUBLE, name_index=%d, high_bytes=%x, low_bytes=%x\n", program, i,
		constant_pool_element->u.cp_double.name_index,
		constant_pool_element->u.cp_double.high_bytes,
		constant_pool_element->u.cp_double.low_bytes);
	break;
    case CONSTANT_NAME_AND_TYPE:
	fprintf(stderr, "%s: [%d] NAME_AND_TYPE, name_index=%d, descriptor_index=%d\n", program, i,
		constant_pool_element->u.cp_name_and_type.name_index,
		constant_pool_element->u.cp_name_and_type.descriptor_index);
	break;
    case CONSTANT_UTF8:
	/* TODO be more careful about UTF8 */
	fprintf(stderr, "%s: [%d] UTF8, length=%d, bytes='%s'\n", program, i,
		constant_pool_element->u.cp_utf8.length,
		constant_pool_element->u.cp_utf8.bytes);
	break;
    case CONSTANT_METHOD_HANDLE:
	fprintf(stderr, "%s: [%d] METHOD_HANDLE, reference_kind=%d, reference_index=%d\n", program, i,
		constant_pool_element->u.cp_method_handle.reference_kind,
		constant_pool_element->u.cp_method_handle.reference_index);
	break;
    case CONSTANT_METHOD_TYPE:
	fprintf(stderr, "%s: [%d] METHOD_TYPE, descriptor_index=%d\n", program, i,
		constant_pool_element->u.cp_method_type.descriptor_index);
	break;
    case CONSTANT_INVOKE_DYNAMIC:
	fprintf(stderr, "%s: [%d] INVOKE_DYNAMIC, bootstrap_method_attr_index=%d, name_and_type_index=%d\n", program, i,
		constant_pool_element->u.cp_invoke_dynamic.bootstrap_method_attr_index,
		constant_pool_element->u.cp_invoke_dynamic.name_and_type_index);
	break;
    default:
	fprintf(stderr, "%s: unknown constant pool tag %d\n", program, constant_pool_element->tag);
	break;
    }
}

static void print_access_flags(class_file_t *class_file) {
    fprintf(stderr, "%s: ACCESS:", program);
    if (ACC_PUBLIC(class_file->access_flags)) {
        fprintf(stderr, " public");
    }
    if (ACC_FINAL(class_file->access_flags)) {
        fprintf(stderr, " final");
    }
    if (ACC_SUPER(class_file->access_flags)) {
        fprintf(stderr, " super");
    }
    if (ACC_INTERFACE(class_file->access_flags)) {
        fprintf(stderr, " interface");
    }
    if (ACC_SYNTHETIC(class_file->access_flags)) {
        fprintf(stderr, " synthetic");
    }
    if (ACC_ANNOTATION(class_file->access_flags)) {
        fprintf(stderr, " annotation");
    }
    if (ACC_ENUM(class_file->access_flags)) {
        fprintf(stderr, " enum");
    }
    fprintf(stderr, "\n");
}

static int read_bytes(int fd, void *buffer, int requested) {
    if (requested <= 0) {
	return requested;
    }
    if (buffer == NULL) {
	return -1;
    }

    int so_far = 0;

    int bytes_read = read_bytes_or_error(fd, buffer, requested, so_far);
    while((bytes_read > 0) && (so_far + bytes_read < requested)){
	so_far += bytes_read;
	bytes_read = read_bytes_or_error(fd, buffer, requested, so_far);
    }

    if (bytes_read < 0) {
	fprintf(stderr, "%s: read_bytes: did not read %d bytes", program, requested);
	return -1;
    }

    if ((bytes_read == 0) && (so_far < requested)) {
	fprintf(stderr, "%s: end of file after reading only %d of %d bytes\n", program, so_far, requested);
	return -1;
    }
    
    return 0;
}

static int read_bytes_or_error(int fd, void *buffer, int requested, int so_far) {
    int bytes_read = read(fd, buffer + so_far, requested - so_far);
    if (bytes_read < 0) {
	fprintf(stderr, "%s: after reading %d bytes, failed to read any more of the %d bytes requested from fd %d: %s",
		program,
		so_far,
		requested,
		fd,
		strerror(errno));
	return -1;
    }
    return bytes_read;
}

static char* get_basename(char *path) {
    if (path == NULL) {
	return NULL;
    }
    if (strlen(path) == 0) {
	return path;
    }
    char *bn = path;
    while (*path) {
	if (*path == '/') {
	    bn = path;
	}
	path++;
    }
    if (*bn == '/') {
	bn++;
    }
    return bn;
}
