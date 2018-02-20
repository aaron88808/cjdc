#include <stdio.h>
#include <string.h>
#include <unistr.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>

#include "cjdc.h"

static char *program = NULL;

static char *get_basename(char *path); 
static int open_class_file(const char *class_file_name);
static class_file_t *read_class_file(int fd);
static void print_class_file (class_file_t *class_file);
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

    print_class_file(class_file);

    int rc = close(fd);
    if (rc < 0) {
	fprintf(stderr, "%s: failed to close file '%s': %s.\n", program, class_file_name, strerror(errno));
	exit(1);
    }

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
    
    return result;

ERR_RETURN:
    if (result) {
	free(result);
    }
    return NULL;
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
