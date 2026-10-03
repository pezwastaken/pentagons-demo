#pragma once

#include "utils.hh"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Arena {
    uint8_t* base;
    size_t   count;      // bytes used, including alignment padding
    size_t   capacity;
};

inline Arena* arena_create(size_t capacity) {
    assert(capacity > 0);
    Arena* p_arena = (Arena*) malloc(sizeof(Arena) + capacity);
    if (!p_arena)
        return NULL;

    p_arena->base     = (uint8_t*) (p_arena + 1);
    p_arena->count    = 0;
    p_arena->capacity = capacity;
    return p_arena;
}

inline void arena_destroy(Arena* p_arena) {
    assert(p_arena != NULL);
    free(p_arena);
}

inline void arena_reset(Arena* p_arena) {
    p_arena->count = 0;
}

inline void* arena_alloc(Arena* p_arena, size_t size, size_t alignment) {
    assert(p_arena != NULL);
    assert(alignment != 0 && (alignment & (alignment - 1)) == 0);

    uintptr_t start   = (uintptr_t) p_arena->base + p_arena->count;
    uintptr_t aligned = (start + alignment - 1) & ~(uintptr_t) (alignment - 1);
    size_t    offset  = (size_t) (aligned - (uintptr_t) p_arena->base);

    if (offset > p_arena->capacity || size > p_arena->capacity - offset)
        return NULL;

    p_arena->count = offset + size;
    return p_arena->base + offset;
}

inline void* arena_alloc_zinit(Arena* p_arena, size_t size, size_t alignment) {
    void* ptr = arena_alloc(p_arena, size, alignment);
    if (ptr)
        memset(ptr, 0, size);
    return ptr;
}

inline void* arena_alloc_array(Arena* p_arena, size_t len, size_t type_size, size_t alignment) {
    if (type_size != 0 && len > SIZE_MAX / type_size)
        return NULL;
    return arena_alloc(p_arena, len * type_size, alignment);
}

inline size_t get_current_index(const Arena* p_arena) {
    return p_arena->count;
}


template <typename T>
T* arena_new(Arena* p_arena) {
    void* mem = arena_alloc_zinit(p_arena, sizeof(T), alignof(T));
    if (!mem) {
        fprintf(stderr, "arena out of memory\n");
        abort();
    }
    return static_cast<T*>(mem);
}

template <typename T>
T* arena_new_array(Arena* p_arena, size_t len) {
    void* mem = arena_alloc_array(p_arena, len, sizeof(T), alignof(T));
    if (!mem) {
        fprintf(stderr, "arena out of memory\n");
        abort();
    }
    return static_cast<T*>(mem);
}
