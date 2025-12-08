/*
 * STDX - Arena Allocator
 * Part of the STDX General Purpose C Library by marciovmf
 * https://github.com/marciovmf/stdx
 * License: MIT
 *
 * Minimal, cache-friendly bump allocator with chunk growth, alignment,
 * fast O(1) steady-state allocations, optional marks (scoped rewinds),
 * and trimming APIs to keep memory bounded after spikes.
 *
 * To compile the implementation define X_IMPL_ARENA
 * in **one** source file before including this header.
 *
 * To customize how this module allocates memory, define
 * X_ARENA_ALLOC / X_ARENA_FREE before including.
 *
 */

#ifndef X_ARENA_H
#define X_ARENA_H

#ifndef X_ARENA_API
#define X_ARENA_API
#endif

#define X_ARENA_VERSION_MAJOR 1
#define X_ARENA_VERSION_MINOR 0
#define X_ARENA_VERSION_PATCH 0
#define X_ARENA_VERSION (X_ARENA_VERSION_MAJOR * 10000 + X_ARENA_VERSION_MINOR * 100 + X_ARENA_VERSION_PATCH)

#ifdef __cplusplus
extern "C" {
#endif

  typedef struct XArenaChunk XArenaChunk;

  typedef struct XArena_t
  {
    size_t       chunk_size;   // Preferred chunk size for growth.
    XArenaChunk* chunks;       // Head of chunk list (most recent first).
    XArenaChunk* current;      // Allocation cursor (first with free space).
  } XArena;

  // A mark is a lightweight snapshot for scoped unwinds.
  typedef struct XArenaMark
  {
    XArenaChunk* chunk;
    size_t       used;
  } XArenaMark;

  /**
   * @brief Create a new arena with the given default chunk size.
   * @return pointer to the new arena or NULL if creation fails
   */
  X_ARENA_API XArena* x_arena_create(size_t chunk_size);

  /** @brief Destroy the arena and free all memory. */
  X_ARENA_API void x_arena_destroy(XArena* arena);

  /** @brief Reset the arena: keep all chunks, set used = 0. */
  X_ARENA_API void x_arena_reset(XArena* arena);

  /** @brief Keep only the head chunk, free the rest. */
  X_ARENA_API void x_arena_reset_keep_head(XArena* arena);

  /** @brief Keep the first `keep_n` chunks, free the rest. */
  X_ARENA_API void x_arena_trim(XArena* arena, size_t keep_n);

  /**
   * @brief Allocate memory from the arena (aligned to X_ARENA_ALIGN).
   * @return pointer to the allocated memory region
   */
  X_ARENA_API void* x_arena_alloc(XArena* arena, size_t size);

  /**
   * @brief Allocate zero-initialized memory from the arena.
   * @return pointer to the allocated memory region
   */
  X_ARENA_API void* x_arena_alloc_zero(XArena* arena, size_t size);

  /** @brief Duplicate a C-string into the arena. */
  X_ARENA_API char* x_arena_strdup(XArena* arena, const char* cstr);

  /** @brief Duplicate a slice into the arena. */
  X_ARENA_API char* x_arena_slicedup(XArena* arena, const char* ptr, size_t len, bool null_terminate);

  /** @brief Take a mark (snapshot) of the arena state. */
  X_ARENA_API XArenaMark x_arena_mark(XArena* arena);

  /** @brief Rewind the arena to a previous mark (frees newer chunks). */
  void x_arena_release(XArena* arena, XArenaMark mark);

#ifdef __cplusplus
}
#endif


#ifdef X_IMPL_ARENA

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifndef X_ARENA_ALLOC
#include <stdlib.h>
#define X_ARENA_ALLOC(sz) malloc((sz))
#endif

#ifndef X_ARENA_FREE
#include <stdlib.h>
#define X_ARENA_FREE(p) free((p))
#endif

#ifndef X_ARENA_ALIGN
#define X_ARENA_ALIGN (sizeof(void*) > sizeof(double) ? sizeof(void*) : sizeof(double))
#endif

#ifdef X_ARENA_DEBUG
#ifndef X_ARENA_DEBUG_FILL
#define X_ARENA_DEBUG_FILL 0xDD
#endif
#endif

struct XArenaChunk
{
  XArenaChunk* next;
  size_t       capacity;
  size_t       used;
  uint8_t*     data; /* Inline storage right after the header. */
};

/* Align up helper. */
static inline size_t x_arena_align_up(size_t n, size_t a)
{
  size_t mask = a - 1u;
  return (n + mask) & ~mask;
}

/* Create a single-allocation chunk: header + inline data. */
static XArenaChunk* x_arena_chunk_create(size_t size)
{
  /* Guard absurd sizes and overflow of header + payload. */
  if (size > (SIZE_MAX - sizeof(XArenaChunk)))
  {
    return NULL;
  }

  size_t total = sizeof(XArenaChunk) + size;
  XArenaChunk* chunk = (XArenaChunk*) X_ARENA_ALLOC(total);
  if (!chunk)
  {
    return NULL;
  }

  chunk->next = NULL;
  chunk->capacity = size;
  chunk->used = 0u;
  chunk->data = (uint8_t*)(chunk + 1);
  return chunk;
}

static void x_arena_chunk_destroy(XArenaChunk* chunk)
{
  if (chunk)
  {
    X_ARENA_FREE(chunk);
  }
}

/* Push chunk to list head. */
static inline void x_arena_push_chunk(XArena* arena, XArenaChunk* chunk)
{
  chunk->next = arena->chunks;
  arena->chunks = chunk;
  arena->current = chunk;
}

X_ARENA_API XArena* x_arena_create(size_t chunk_size)
{
  if (chunk_size == 0u)
  {
    /* Fall back to a reasonable default (16 KB). */
    chunk_size = 16u * 1024u;
  }

  XArena* arena = (XArena*) X_ARENA_ALLOC(sizeof(XArena));
  if (!arena)
  {
    return NULL;
  }

  arena->chunk_size = chunk_size;
  arena->chunks = NULL;
  arena->current = NULL;

  /* Allocate the initial chunk so x_arena_alloc can be fast. */
  XArenaChunk* head = x_arena_chunk_create(chunk_size);
  if (!head)
  {
    X_ARENA_FREE(arena);
    return NULL;
  }
  x_arena_push_chunk(arena, head);
  return arena;
}

X_ARENA_API void x_arena_destroy(XArena* arena)
{
  if (!arena)
  {
    return;
  }

  XArenaChunk* c = arena->chunks;
  while (c)
  {
    XArenaChunk* next = c->next;
    x_arena_chunk_destroy(c);
    c = next;
  }

  X_ARENA_FREE(arena);
}

X_ARENA_API void x_arena_reset(XArena* arena)
{
  if (!arena)
  {
    return;
  }

  for (XArenaChunk* c = arena->chunks; c != NULL; c = c->next)
  {
#ifdef X_ARENA_DEBUG
    memset(c->data, X_ARENA_DEBUG_FILL, c->used);
#endif
    c->used = 0u;
  }

  arena->current = arena->chunks;
}

X_ARENA_API void x_arena_reset_keep_head(XArena* arena)
{
  if (!arena || !arena->chunks)
  {
    return;
  }

  XArenaChunk* head = arena->chunks;
  XArenaChunk* c = head->next;
  while (c)
  {
    XArenaChunk* next = c->next;
    x_arena_chunk_destroy(c);
    c = next;
  }

  head->next = NULL;
#ifdef X_ARENA_DEBUG
  memset(head->data, X_ARENA_DEBUG_FILL, head->used);
#endif
  head->used = 0u;
  arena->chunks = head;
  arena->current = head;
}

X_ARENA_API void x_arena_trim(XArena* arena, size_t keep_n)
{
  if (!arena)
  {
    return;
  }

  XArenaChunk* c = arena->chunks;
  size_t i = 0u;
  while (c && i + 1u < keep_n)
  {
    c = c->next;
    i = i + 1u;
  }

  if (!c)
  {
    return;
  }

  XArenaChunk* to_free = c->next;
  c->next = NULL;

  while (to_free)
  {
    XArenaChunk* next = to_free->next;
    x_arena_chunk_destroy(to_free);
    to_free = next;
  }

  arena->current = arena->chunks;
}

X_ARENA_API static void* x_arena_alloc_from_chunk(XArenaChunk* c, size_t size)
{
  size_t off = x_arena_align_up(c->used, X_ARENA_ALIGN);
  if (off > c->capacity)
  {
    return NULL;
  }
  size_t remaining = c->capacity - off;
  if (remaining < size)
  {
    return NULL;
  }
  void* ptr = c->data + off;
  c->used = off + size;
  return ptr;
}

X_ARENA_API void* x_arena_alloc(XArena* arena, size_t size)
{
  if (!arena || size == 0u)
  {
    return NULL;
  }

  /* Paranoid guard against integer wrap in callers. */
  if (size > SIZE_MAX - X_ARENA_ALIGN)
  {
    return NULL;
  }

  /* Fast path: try current chunk. */
  if (arena->current)
  {
    void* p = x_arena_alloc_from_chunk(arena->current, size);
    if (p)
    {
      return p;
    }
  }

  /* Try other existing chunks before growing. */
  for (XArenaChunk* c = arena->chunks; c != NULL; c = c->next)
  {
    if (c == arena->current)
    {
      continue;
    }
    void* p = x_arena_alloc_from_chunk(c, size);
    if (p)
    {
      arena->current = c;
      return p;
    }
  }

  /* Need a new chunk: grow by max(request, default chunk size). */
  size_t grow = size > arena->chunk_size ? size : arena->chunk_size;
  XArenaChunk* n = x_arena_chunk_create(grow);
  if (!n)
  {
    return NULL;
  }

  x_arena_push_chunk(arena, n);
  return x_arena_alloc_from_chunk(n, size);
}

X_ARENA_API void* x_arena_alloc_zero(XArena* arena, size_t size)
{
  void* p = x_arena_alloc(arena, size);
  if (p)
  {
    memset(p, 0, size);
  }
  return p;
}

X_ARENA_API char* x_arena_strdup(XArena* arena, const char* cstr)
{
  if (!cstr)
  {
    return NULL;
  }
  size_t n = strlen(cstr);
  char* d = (char*) x_arena_alloc(arena, n + 1u);
  if (!d)
  {
    return NULL;
  }
  memcpy(d, cstr, n);
  d[n] = '\0';
  return d;
}

X_ARENA_API char* x_arena_slicedup(XArena* arena, const char* ptr, size_t len, bool null_terminate)
{
  if (!ptr)
  {
    return NULL;
  }

  char* d = (char*) x_arena_alloc(arena, len + null_terminate ? 1u : 0u);
  if (!d)
  {
    return NULL;
  }
  memcpy(d, ptr, len);
  if (null_terminate)
    d[len] = '\0';
  return d;
}

X_ARENA_API XArenaMark x_arena_mark(XArena* arena)
{
  XArenaMark m;
  if (!arena || !arena->chunks)
  {
    m.chunk = NULL;
    m.used = 0u;
    return m;
  }
  m.chunk = arena->chunks;
  m.used = arena->chunks->used;
  return m;
}

X_ARENA_API void x_arena_release(XArena* arena, XArenaMark mark)
{
  if (!arena)
  {
    return;
  }

  /* If mark.chunk is NULL, clear everything. */
  if (mark.chunk == NULL)
  {
    XArenaChunk* c = arena->chunks;
    while (c)
    {
      XArenaChunk* next = c->next;
      x_arena_chunk_destroy(c);
      c = next;
    }

    arena->chunks = NULL;
    arena->current = NULL;

    /* Recreate an initial chunk for future fast allocations. */
    XArenaChunk* head = x_arena_chunk_create(arena->chunk_size);
    if (head)
    {
      x_arena_push_chunk(arena, head);
    }
    return;
  }

  /* Free chunks newer than mark.chunk. */
  XArenaChunk* c = arena->chunks;
  while (c && c != mark.chunk)
  {
    XArenaChunk* next = c->next;
    x_arena_chunk_destroy(c);
    c = next;
  }

  arena->chunks = c;
  arena->current = c;
  if (c)
  {
#ifdef X_ARENA_DEBUG
    if (mark.used < c->used)
    {
      size_t span = c->used - mark.used;
      memset(c->data + mark.used, X_ARENA_DEBUG_FILL, span);
    }
#endif
    c->used = mark.used;
  }
}


/* --- Testing helpers (debug-only) ----------------------------------------- */
#ifdef X_ARENA_TESTING
size_t x_arena_chunk_count(const XArena* a)
{
  size_t n = 0;
  for (const XArenaChunk* c = a ? a->chunks : NULL; c; c = c->next) n++;
  return n;
}
size_t x_arena_head_capacity(const XArena* a)
{
  return (a && a->chunks) ? a->chunks->capacity : 0u;
}
size_t x_arena_head_used(const XArena* a)
{
  return (a && a->chunks) ? a->chunks->used : 0u;
}
#endif /* X_ARENA_TESTING */

#endif // X_IMPL_ARENA
#endif // X_ARENA_H
