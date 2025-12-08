#ifndef BLUNT_H
#define BLUNT_H

#include <stdx_common.h>
#include <stdx_arena.h>
#include <stdx_string.h>
#include <stdx_filesystem.h>

typedef struct BltConfig
{
  const char* site_url;
  const char* site_name;
  XFSPath pages_dir;
  XFSPath assets_dir;
  XFSPath output_dir;
  XFSPath posts_dir;
  XFSPath template_dir;
  void* data;
} BltConfig;

typedef enum BltPageType
{
  slab_PAGE_TYPE_UNKNOWN = 0,
  slab_PAGE_TYPE_POST,
  slab_PAGE_TYPE_STATIC
}
BltPageType;

typedef struct BltPage
{
  /* Identity / paths */
  const char* source_path;    /* full path to source file */
  const char* output_path;    /* where to write final HTML, optional for now */

  /* Core metadata */
  BltPageType type;           /* post / static / unknown */

  const char* title;          /* required for all pages */
  const char* slug;           /* required (for posts, taken from filename) */
  const char* template_name;  /* logical template key, e.g. "post" */
  const char* date;           /* "YYYY-MM-DD" - required for posts */

  /* Optional */
  const char* author;
  const char* category;
  const char* tags;           /* space-separated list for now */

  /* Reserved for future extensions (extra key/value map, etc.) */
}
BltPage;

typedef struct BltSite
{
  XArena*   arena;      // this arena provides memory for everything necessary
                        // during site metadata collection and config loading.
  BltPage*  pages;
  size_t    page_count;
  size_t    page_capacity;
  BltConfig config;
}
BltSite;

typedef struct BltMetaParseResult
{
  int ok;
  const char* start;
  const char* error_msg;
}
BltMetaParseResult;

BltSite* slab_site_create(size_t arena_size, BltConfig* config);
void slab_site_destroy(BltSite* site);

int slab_site_add_page(BltSite* site, const BltPage* page, XArena* arena);

BltMetaParseResult slab_parse_page_meta(const char* data,
    size_t len,
    XArena* arena,
    BltPage* out_page);

int slab_validate_site(const BltSite* site);

bool slab_config_load(const char* site_root, BltConfig* out_config);
void slab_config_unload(BltConfig* config);

i32 slab_process_directory_metadata(BltSite* site, const char* path);

#endif // BLUNT_H
