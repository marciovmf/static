#include <stdx_common.h>
#define X_IMPL_STRING
#define X_IMPL_STRBUILDER
#define X_IMPL_ARENA
#define X_IMPL_FILESYSTEM
#define X_IMPL_IO
#define X_IMPL_LOG
#define X_IMPL_INI
#include <stdx_string.h>
#include <stdx_filesystem.h>
#include <stdx_ini.h>
#include <stdx_io.h>
#include <stdx_log.h>
#include <stdio.h>

#include "slab.h"
#define MD_IMPL
#include "markdown.h"
#include "template.h"

i32 slab_run(const char* site_root)
{
  if (!x_fs_path_is_directory_cstr(site_root))
  {
    x_log_error("Site path not found: %s", site_root);
    return 1;
  }

  BltConfig site_config;
  slab_config_load(site_root, &site_config);
  BltSite* site = slab_site_create(1024 * 1024, &site_config);


  // Collect metadata
  x_log_info("Collecting metedata.");
  // Pages directory
  slab_process_directory_metadata(site, x_fs_path_cstr(&site_config.pages_dir));

  // Posts directory
  if (! x_fs_path_eq(&site_config.pages_dir, &site_config.posts_dir))
    slab_process_directory_metadata(site, x_fs_path_cstr(&site_config.posts_dir));

  for(u32 i = 0; i < site->page_count; i++)
  {
    BltPage* page = &site->pages[i]; 
    x_log_debug("Date: %s, slug: %s, template: %s, tags: %s, author:%s",
        page->date, page->slug, page->template_name, page->tags, page->author);
  }


  slab_site_destroy(site);
  slab_config_unload(&site_config);
  return 0;
}

i32 main(int argc, char **argv)
{
  if (argc < 2)
  {
    fprintf(stderr, "Usage: %s <site_root>\n", argv[0]);
    return 1;
  }

  const char* site_root = argv[1];
  if (!x_fs_path_is_directory_cstr(site_root))
  {
    x_log_error("Site path not found: %s", site_root);
    return 1;
  }

  return slab_run(site_root);
}
