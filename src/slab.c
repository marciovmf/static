#include <stdx_common.h>
#include <stdx_string.h>
#include <stdx_filesystem.h>
#include <stdx_ini.h>
#include <stdx_io.h>
#include <stdx_log.h>
#include <stdio.h>
#include <stdlib.h>

#include "markdown.h"
#include "slab.h"
#include "stdx_arena.h"

/* 
 * <%meta ... %> parsing
 */
static const char* slab_trim(const char* s, const char* end, const char** out_end)
{
  const char* e;

  while (s < end &&
      (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n'))
  {
    s++;
  }

  e = end;
  while (e > s &&
      (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r' || e[-1] == '\n'))
  {
    e--;
  }

  if (out_end)
  {
    *out_end = e;
  }

  return s;
}

bool slab_config_load(const char* site_root, BltConfig* out_config)
{
  if (!site_root || !out_config)
    return false;

  XFSPath site_config_file;
  x_fs_path(&site_config_file, site_root, "site.ini");
  const char* ini_file_path = x_smallstr_cstr(&site_config_file);
  x_log_info("Loading ini file: %s", ini_file_path);

  size_t file_size;
  char *data = x_io_read_text(ini_file_path, &file_size);
  if (!data)
  {
    x_log_error("Unable to load site configuration file '%s'", ini_file_path);
    return false;
  }

  XIni ini;
  XIniError iniError;
  if (! x_ini_load_mem(data, strlen(data), &ini, &iniError))
  {
    x_log_error("Unable to parse site configuration file '%s'", ini_file_path);
    free(data);
    return false;
  }

  const char *s;
  out_config->data = data;
  out_config->site_name = x_ini_get(&ini, "site", "name", "");
  out_config->site_url  = x_ini_get(&ini, "site", "url", "");

  s = x_ini_get(&ini, "site", "output_dir", "out");
  if (x_fs_path_is_absolute_cstr(s))
    x_fs_path(&out_config->output_dir, s);
  else
    x_fs_path(&out_config->output_dir, site_root, s);
  x_fs_path_normalize(&out_config->output_dir);

  s = x_ini_get(&ini, "site", "pages_dir", ".");
  if (x_fs_path_is_absolute_cstr(s))
    x_fs_path(&out_config->pages_dir, s);
  else
    x_fs_path(&out_config->pages_dir, site_root, s);
  x_fs_path_normalize(&out_config->pages_dir);

  s = x_ini_get(&ini, "site", "posts_dir", ".");
  if (x_fs_path_is_absolute_cstr(s))
    x_fs_path(&out_config->posts_dir, s);
  else
    x_fs_path(&out_config->posts_dir, site_root, s);
  x_fs_path_normalize(&out_config->posts_dir);

  s = x_ini_get(&ini, "site", "template_dir", "");
  if (x_fs_path_is_absolute_cstr(s))
    x_fs_path(&out_config->template_dir, s);
  else
    x_fs_path(&out_config->template_dir, site_root, s);
  x_fs_path_normalize(&out_config->template_dir);

  s = x_ini_get(&ini, "site", "assets_dir", "");
  if (x_fs_path_is_absolute_cstr(s))
    x_fs_path(&out_config->assets_dir, s);
  else
    x_fs_path(&out_config->assets_dir, site_root, s);
  x_fs_path_normalize(&out_config->assets_dir);

  x_log_info("Site config:\n"
      "\tname = %s\n"
      "\turl = %s\n"
      "\tassets_dir = %s\n"
      "\toutput_dir = %s\n"
      "\tposts_dir = %s\n"
      "\tpages_dir = %s\n"
      "\ttemplate_dir = %s\n",
      out_config->site_name,
      out_config->site_url,
      x_fs_path_cstr(&out_config->output_dir),
      x_fs_path_cstr(&out_config->assets_dir),
      x_fs_path_cstr(&out_config->posts_dir),
      x_fs_path_cstr(&out_config->pages_dir),
      x_fs_path_cstr(&out_config->template_dir));
  return true;
}

void slab_config_unload(BltConfig* config)
{
  if (!config)
    return;

  free(config->data);
  config->site_name     = NULL;
  config->site_url      = NULL;
  config->data          = NULL;

}

i32 slab_process_directory_metadata(BltSite* site, const char* path)
{
  XFSDireEntry dir_entry;
  XArena* site_arena = site->arena;
  //BltConfig* config = &site->config;
  XFSDireHandle* handle = x_fs_find_first_file(path, &dir_entry);
  if (!handle)
  {
    x_log_error("Failed to scan posts directory %s\n", path);
    return 1;
  }

  do
  {
    if (! (x_cstr_ends_with(dir_entry.name, ".md") || x_cstr_ends_with(dir_entry.name, ".html") || x_cstr_ends_with(dir_entry.name, ".htm")))
      continue;

    BltPage page;
    size_t buf_size;
    XFSPath full_path;
    x_fs_path(&full_path, path, dir_entry.name);

    char* buf = x_io_read_text(x_fs_path_cstr(&full_path), &buf_size);
    if (! buf)
    {
      x_log_error("Failed to read from file %s\n", full_path);
      return 1;
    }

    x_log_info("Processing %s", full_path);

    BltMetaParseResult mr = slab_parse_page_meta(buf, buf_size, site_arena, &page);
    if (! mr.ok)
    {
      x_log_error("%s: %s\n", dir_entry.name, mr.error_msg);
      continue;
    }

    //const char* buff_past_meta_block = mr.start;
    //// If it's markdown we convert to html right away
    //if (x_cstr_ends_with(dir_entry.name, ".md"))
    //{
    //  printf("%s\n", md_to_html(buff_past_meta_block));
    //}

    if (!slab_site_add_page(site, &page, site_arena))
    {
      x_log_error("Failed to register page %s: %s\n", dir_entry.name, mr.error_msg);
      continue;
    }
  }
  while (x_fs_find_next_file(handle, &dir_entry));
  return 0;
}

BltSite* slab_site_create(size_t arena_size, BltConfig* config)
{
  if (arena_size <= 0)
    return NULL;

  // The main struct lives inside the actual arena
  XArena* arena = x_arena_create(1024 * 1024);
  if (!arena)
    return NULL;

  BltSite* site = x_arena_alloc_zero(arena, sizeof(BltSite));
  site->arena = arena;
  site->config = *config;
  return site;
}

void slab_site_destroy(BltSite* site)
{
  if (!site)
    return;

  x_arena_destroy(site->arena);
}

int slab_site_add_page(BltSite* site, const BltPage* page, XArena* arena)
{
  BltPage* dest;

  if (!site || !page)
  {
    return 0;
  }

  if (site->page_count == site->page_capacity)
  {
    size_t new_cap;
    size_t bytes;
    BltPage* new_pages;

    new_cap = (site->page_capacity == 0) ? 16 : site->page_capacity * 2;
    bytes = new_cap * sizeof(BltPage);

    new_pages = (BltPage*)x_arena_alloc(arena, bytes);
    if (!new_pages)
    {
      return 0;
    }

    if (site->pages && site->page_count > 0)
    {
      memcpy(new_pages, site->pages, site->page_count * sizeof(BltPage));
    }

    site->pages = new_pages;
    site->page_capacity = new_cap;
  }

  dest = &site->pages[site->page_count++];
  memset(dest, 0, sizeof(*dest));
  *dest = *page;

  return 1;
}

static void slab_set_field(BltPage* page,
    XArena* arena,
    const char* key,
    size_t key_len,
    const char* val,
    size_t val_len)
{
#define MATCH(lit) (key_len == sizeof(lit) - 1 && \
    memcmp(key, lit, key_len) == 0)

  const char* copied;

  copied = x_arena_slicedup(arena, val, val_len, true);
  if (!copied)
  {
    return;
  }

  if (MATCH("type"))
  {
    if (strncmp(copied, "post", 4) == 0)
    {
      page->type = slab_PAGE_TYPE_POST;
    }
    else if (strncmp(copied, "static", 6) == 0)
    {
      page->type = slab_PAGE_TYPE_STATIC;
    }
    else
    {
      page->type = slab_PAGE_TYPE_UNKNOWN;
    }
  }
  else if (MATCH("title"))
  {
    page->title = copied;
  }
  else if (MATCH("slug"))
  {
    /* For posts, slug usually comes from filename; you can decide
     * whether to allow overriding it here. For now, override. */
    page->slug = copied;
  }
  else if (MATCH("template"))
  {
    page->template_name = copied;
  }
  else if (MATCH("date"))
  {
    /* Same story as slug: can override filename if desired. */
    page->date = copied;
  }
  else if (MATCH("author"))
  {
    page->author = copied;
  }
  else if (MATCH("tags"))
  {
    page->tags = copied;
  }
  else if (MATCH("category"))
  {
    page->category = copied;
  }
  else
  {
    /* Unknown key: you could stash this in a future extra-map. */
  }

#undef MATCH
}

BltMetaParseResult slab_parse_page_meta(const char* data, size_t len, XArena* arena, BltPage* out_page)
{
  BltMetaParseResult R;
  const char* s;
  const char* end;
  const char* p;
  const char* meta_start;
  const char* meta_end;

  R.ok = 1;
  R.error_msg = NULL;
  R.start = data;

  if (!data || !out_page)
  {
    R.ok = 0;
    R.error_msg = "Invalid arguments to slab_parse_page_meta";
    return R;
  }

  memset(out_page, 0, sizeof(BltPage));
  s = data;
  end = data + len;

  /* Find "<%meta" */
  p = s;
  meta_start = NULL;
  while (p + 6 <= end)
  {
    if (p[0] == '<' && p[1] == '%' &&
        p[2] == 'm' && p[3] == 'e' &&
        p[4] == 't' && p[5] == 'a')
    {
      meta_start = p + 6;
      break;
    }

    p++;
  }

  if (!meta_start)
  {
    /* No meta block: caller decides whether that's an error. */
    R.ok = 0;
    R.error_msg = "Missing <%meta ... %> block";
    return R;
  }

  /* Find corresponding "%>" */
  meta_end = NULL;
  p = meta_start;
  while (p + 1 < end)
  {
    if (p[0] == '%' && p[1] == '>')
    {
      meta_end = p;
      break;
    }

    p++;
  }

  if (!meta_end)
  {
    R.ok = 0;
    R.error_msg = "Unterminated <%meta ... %> block";
    return R;
  }

  /* Parse lines inside [meta_start, meta_end) */
  {
    const char* line_start;

    line_start = meta_start;

    while (line_start < meta_end)
    {
      const char* line_end;
      const char* cur;
      const char* trimmed_end;
      const char* key_start;
      const char* key_end;
      const char* val_start;
      const char* val_end;

      line_end = memchr(line_start, '\n', (size_t)(meta_end - line_start));
      if (!line_end)
      {
        line_end = meta_end;
      }

      cur = slab_trim(line_start, line_end, &trimmed_end);
      if (cur < trimmed_end)
      {
        /* key = first token; rest of line = value */
        key_start = cur;
        key_end = cur;

        while (key_end < trimmed_end &&
            *key_end != ' ' && *key_end != '\t')
        {
          key_end++;
        }

        val_start = key_end;
        val_start = slab_trim(val_start, trimmed_end, &val_end);

        if (key_start < key_end && val_start < val_end)
        {
          slab_set_field(out_page,
              arena,
              key_start,
              (size_t)(key_end - key_start),
              val_start,
              (size_t)(val_end - val_start));
        }
      }

      line_start = line_end + 1;
    }
  }

  R.start = p+2; // +2 to skip %>
  return R;
}

int slab_validate_site(const BltSite* site)
{
  size_t i;

  if (!site)
  {
    return 0;
  }

  for (i = 0; i < site->page_count; ++i)
  {
    const BltPage* p;

    p = &site->pages[i];

    if (p->type == slab_PAGE_TYPE_UNKNOWN)
    {
      fprintf(stderr,
          "Error: page '%s' missing required meta key 'type'\n",
          p->source_path ? p->source_path : "<unknown>");
      return 0;
    }

    if (!p->title)
    {
      fprintf(stderr,
          "Error: page '%s' missing required meta key 'title'\n",
          p->source_path ? p->source_path : "<unknown>");
      return 0;
    }

    if (!p->slug)
    {
      fprintf(stderr,
          "Error: page '%s' missing required meta key 'slug'\n",
          p->source_path ? p->source_path : "<unknown>");
      return 0;
    }

    if (!p->template_name)
    {
      fprintf(stderr,
          "Error: page '%s' missing required meta key 'template'\n",
          p->source_path ? p->source_path : "<unknown>");
      return 0;
    }

    if (p->type == slab_PAGE_TYPE_POST && !p->date)
    {
      fprintf(stderr,
          "Error: post '%s' missing required meta key 'date'\n",
          p->source_path ? p->source_path : "<unknown>");
      return 0;
    }
  }

  return 1;
}

