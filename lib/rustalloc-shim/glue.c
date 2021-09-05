#include <stdlib.h>
#include <inttypes.h>

void * rust_memalign(size_t align, size_t size) {
    return memalign(align, size);
}
void rust_free(void * ptr) {
    free(ptr);
}
