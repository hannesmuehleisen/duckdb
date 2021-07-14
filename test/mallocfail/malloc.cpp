#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <stdexcept>
#include <atomic>
#include <new>

#include "mallocfail.h"

std::atomic<size_t> success_count (1000000);

typedef void *(*malloc_t)(size_t len);
malloc_t real_malloc;


extern "C" {

void *malloc(size_t s) {
//    fprintf(stderr, "called malloc(%zu)\n", s);
    if (!real_malloc) {
        real_malloc = (malloc_t) dlsym(RTLD_NEXT, "malloc");
    }

	if (success_count == 0) {
        throw std::bad_alloc();
    }
    success_count--;
    return real_malloc(s);
}


}

void *
operator new (size_t size)
{
   return malloc(size);
}

void
reset_malloc_limit (size_t limit)
{
    success_count = limit;
}



