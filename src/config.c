#include "config.h"
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


Config *cfg_create_config()
{
  Config *config = malloc(sizeof(Config));
  if (!config)
  {
    perror("Failed to allocate memory for config");
    return NULL;
  }
  config->size = 0;
  config->capacity = CFG_INITIAL_CAPACITY;
  config->entries = malloc(sizeof(ConfigEntry) * config->capacity);
  if (!config->entries)
  {
    perror("Failed to allocate memory for config entries");
    free(config);
    return NULL;
  }
  return config;
}

void cfg_free_config(Config *config)
{
  if (config)
  {
    for (size_t i = 0; i < config->size; ++i)
    {
      free(config->entries[i].key);
      free(config->entries[i].value);
    }
    free(config->entries);
    free(config);
  }
}

static void cfg_add_entry(Config *config, const char *key, const char *value)
{
  if (config->size >= config->capacity)
  {
    config->capacity += CFG_INITIAL_CAPACITY; // Increase capacity by chunks 
    config->entries = realloc(config->entries, sizeof(ConfigEntry) * config->capacity);
    if (!config->entries)
    {
      perror("Failed to reallocate memory for config entries");
      return;
    }
  }
  config->entries[config->size].key = _strdup(key);
  config->entries[config->size].value = _strdup(value);
  config->size++;
}

Config *cfg_parse_file(const char *filename)
{
  FILE *file = fopen(filename, "r");
  if (!file)
  {
    perror("Could not open config file");
    return NULL;
  }

  Config* cfg = cfg_create_config();
  char line[CFG_MAX_LINE_LENGTH];
  while (fgets(line, sizeof(line), file))
  {
    // Trim whitespace and check if the line is empty or a comment
    char *trimmed_line = line;
    while (*trimmed_line == ' ') trimmed_line++;
    if (*trimmed_line == '\0' || *trimmed_line == '#') continue;

    // Split the line into key and value
    char *equals = strchr(trimmed_line, '=');
    if (equals)
    {
      *equals = '\0';
      char *key = trimmed_line;
      char *value = equals + 1; // Start after '='

      // Trim leading spaces
      while (*value == ' ')
        value++; 

      char *newline = strchr(value, '\n');
      if (newline)
        *newline = '\0';

      cfg_add_entry(cfg, key, value);
    }
  }

  fclose(file);
  return cfg;
}
