#include "shared_resource.h"

#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

struct shared_resource_t *shared_resource_open(const char *path, int size, int create, void **mapping)
{
    struct shared_resource_t *resource = malloc(sizeof(struct shared_resource_t));

    resource->fd = -1;
    resource->mapping = MAP_FAILED;
    resource->size = 0;

    if (create == 0)
    {
        resource->fd = open(path, O_RDWR);
        if (resource->fd == -1)
        {
            shared_resource_close(resource);
            return NULL;
        }
    }
    else
    {
        resource->fd = open(path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
        if (resource->fd == -1)
        {
            shared_resource_close(resource);
            return NULL;
        }

        if (ftruncate(resource->fd, size) == -1)
        {
            shared_resource_close(resource);
            return NULL;
        }

        if (fchmod(resource->fd, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH) == -1)
        {
            shared_resource_close(resource);
            return NULL;
        }
    }

    resource->mapping = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, resource->fd, 0);
    if (resource->mapping == MAP_FAILED)
    {
        shared_resource_close(resource);
        return NULL;
    }

    resource->size = size;


    *mapping = resource->mapping;

    return resource;
}

int shared_resource_close(struct shared_resource_t *resource)
{
    if (resource->mapping != MAP_FAILED)
    {
        munmap(resource->mapping, resource->size);
        resource->mapping = MAP_FAILED;
        resource->size = 0;
    }

    if (resource->fd != -1)
    {
        close(resource->fd);
        resource->fd = -1;
    }

    free(resource);

    return 0;
}

