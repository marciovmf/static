#ifndef CONFIG_H
#define CONFIG_H

#ifndef CFG_MAX_LINE_LENGTH 
#define CFG_MAX_LINE_LENGTH 256
#endif 

#ifndef CFG_INITIAL_CAPACITY 
#define CFG_INITIAL_CAPACITY 32  // Change the initial capacity to 32
#endif

typedef struct
{
  char *key;
  char *value;
} ConfigEntry;

typedef struct
{
  ConfigEntry *entries;
  size_t size;
  size_t capacity;
} Config;


/**
 * @brief Creates and initializes a new configuration object.
 * 
 * This function allocates memory for a Config structure and initializes its fields.
 * The initial capacity for the configuration entries is set to a predefined constant (CFG_INITIAL_CAPACITY).
 * 
 * @return A pointer to the newly created Config structure, or NULL if
 *         memory allocation fails.
 */
Config *cfg_create_config();

/**
 * @brief Frees the memory allocated for a configuration object.
 * 
 * This function releases the memory allocated for the Config structure 
 * and all its entries. It should be called when the configuration object 
 * is no longer needed to prevent memory leaks.
 * 
 * @param config A pointer to the Config structure to be freed. If NULL, the function does nothing.
 */
void cfg_free_config(Config *config);

/**
 * @brief Parses a configuration file and fills the Config structure.
 * 
 * This function reads a file line by line, ignoring empty lines and lines starting with '#'.
 * It splits lines in the format "entry = value" into key-value pairs and stores them in the provided Config structure.
 * 
 * @param filename The path to the configuration file to parse.
 * 
 * @return A pointer to a newly created config structure with all key/value pairs parsed from the file on success,
 *  or NULL if the file could not be opened or if any error occurs during parsing.
 */
Config* cfg_parse_file(const char *filename);

#endif // CONFIG_H
