#ifndef SHARED_RESOURCE_H
#define SHARED_RESOURCE_H

struct shared_resource_t
{
    int fd;
    void *mapping;
    int size;
};

struct shared_resource_t *shared_resource_open(const char *path, int size, int create, void **mapping);
int shared_resource_close(struct shared_resource_t *resource);

#endif //SHARED_RESOURCE_H

