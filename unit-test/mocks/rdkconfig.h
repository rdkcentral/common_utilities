#ifndef RDKCONFIG_H
#define RDKCONFIG_H

#include <stddef.h>
#include <stdint.h>

#define RDKCONFIG_OK   0
#define RDKCONFIG_FAIL 1

int rdkconfig_get(uint8_t **sbuff, size_t *sbuffsz, const char *refname);
int rdkconfig_free(uint8_t **sbuff, size_t sbuffsz);

#endif
