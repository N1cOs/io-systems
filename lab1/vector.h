#include <linux/types.h>

#define VECTOR_ELEMENT_SIZE 4

typedef struct {
    uint32_t *buf;
    uint32_t len;
    uint32_t cap;
} vector;

vector* new_vector(uint32_t cap);
void append(vector *v, uint32_t val);
void destroy_vector(vector *v);
