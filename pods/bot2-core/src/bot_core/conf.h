#ifndef __bot_conf_h__
#define __bot_conf_h__

#include <stdio.h>

/**
 * SECTION:conf
 * @title: Configuration Files
 * @short_description: Hierarchical key/value configuration files
 * @include: bot_core/bot_core.h
 *
 * TODO
 *
 * Linking: `pkg-config --libs bot2-core`
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _BotConf BotConf;

/**
 * bot_conf_parse_file:
 * @f: An open file to read from.
 * @filename: The name of the file @f.
 *
 * Parses a file and returns a handle to the contents.  The file should
 * already be opened and passed in as f.  filename is used merely for
 * printing error messages if the need should arise.
 *
 * Returns: A handle to a newly-allocated %BotConf or %NULL on parse error.
 */
BotConf *
bot_conf_parse_file (FILE * f, const char * filename);

/**
 * bot_conf_parse_string:
 * @string: A character array containing the config file.
 * @length: The length of the character array
 *
 * Parses the contents of the string and returns a handle to the contents.  The
 * name is used merely for printing error messages if the need should arise.
 *
 * Returns: A handle to a newly-allocated %BotConf or %NULL on parse error.
 */
BotConf *
bot_conf_parse_string (const char * string, int length);


/**
 * bot_conf_parse_default:
 *
 * Parses the default DGC configuration file (config/master.cfg) and
 * returns a handle to it.  If the environment variable BOT_CONF_PATH
 * is set, that path is used instead.
 *
 * Returns: A pointer to a newly-allocated %BotConf, or %NULL on parse error.
 */
BotConf *
bot_conf_parse_default (void);

/**
 * bot_conf_get_default_src:
 * @buf: The buffer to store the default path in.
 * @buflen: The length of @buf.
 *
 * Copies the filename of the default config file into buf.
 * In the future, it might also indicate that LC is to be used for the config
 * source
 *
 * Returns: 0 on success, -1 if buf isn't big enough
 */
int bot_conf_get_default_src (char *buf, int buflen);

/**
 * bot_conf_free:
 * @conf: The %BotConf to free.
 *
 * Frees a configuration handle
 */
void
bot_conf_free (BotConf * conf);

/**
 * bot_conf_write:
 * @conf: The configuration to write.
 * @f: The file handle to write to
 *
 * Writes the contents of a parsed configuration file to file handle @f.
 *
 * Returns: -1 on error, 0 on success.
 */
int
bot_conf_write (BotConf * conf, FILE * f);

/**
 * bot_conf_print:
 * @conf: The configuration to print.
 *
 * Prints the contents of a parsed configuration file.
 *
 * Returns: -1 on error, 0 on success.
 */
static inline int
bot_conf_print (BotConf * conf){
  return bot_conf_write (conf, stdout);
}

/**
 * bot_conf_has_key:
 * @conf: The configuration.
 * @key: The key to check the existance of.
 *
 * Checks if a key named @key exists in the file that was read in by @conf.
 *
 * Returns: 1 if the key is present, 0 if not
 */
int bot_conf_has_key (BotConf *conf, const char *key);

/**
 * bot_conf_get_num_subkeys:
 * @conf: The configuraion.
 * @containerKey: The key to check.
 *
 * Finds the number of sub keys of @containerKey that also have sub keys.
 *
 * Returns: The number of top-level keys in container named by containerKey or
 * -1 if container key not found.
 */
int
bot_conf_get_num_subkeys (BotConf * conf, const char * containerKey);

/**
 * bot_conf_get_subkeys:
 * @conf: The configuration.
 * @containerKey:
 *
 * Fetch all top-level keys in container named by containerKey.
 *
 * Returns: a newly allocated, %NULL-terminated, array of strings.  This array
 * should be freed by the caller (e.g. with g_strfreev)
 */
char **
bot_conf_get_subkeys (BotConf * conf, const char * containerKey);
                     


/**
 * bot_conf_get_int:
 * @conf: The configuration.
 * @key: The key to get the value for.
 * @val: Return value.
 *
 * This searches for a key (i.e. "sick.front.pos") and fetches the value
 * associated with it into @val, converting it to an %int.  If the conversion
 * is not possible or the key is not found, -1 is returned.  If the key
 * contains an array of multiple values, the first value is used.
 *
 * Returns: 0 on success, -1 on failure.
 */
int
bot_conf_get_int (BotConf * conf, const char * key, int * val);

/**
 * bot_conf_get_boolean:
 * @conf: The configuration.
 * @key: The key to get the value for.
 * @val: Return value.
 *
 * Same as bot_conf_get_int(), only for a boolean.
 *
 * Returns: 0 on success, -1, on failure.
 */
int
bot_conf_get_boolean (BotConf * conf, const char * key, int * val);

/**
 * bot_conf_get_double:
 * @conf: The configuration.
 * @key: The key to get the value for.
 * @val: Return value.
 *
 * Same as bot_conf_get_int(), only for a double.
 *
 * Returns: 0 on success, -1, on failure.
 */
int
bot_conf_get_double (BotConf * conf, const char * key, double * val);

/**
 * bot_conf_get_str:
 * @conf: The configuration.
 * @key: The key to get the value for.
 * @val: Return value.
 *
 * Same as bot_conf_get_int(), only for a string.
 *
 * Returns: 0 on success, -1, on failure.
 */
int
bot_conf_get_str (BotConf * conf, const char * key, char ** val);

/**
 * bot_conf_get_int_or_default:
 * @conf: The configuration.
 * @key: The key to get the value for.
 * @def: A "default" value.
 *
 * Like bot_conf_get_int(), except it returns the value instead of putting it
 * in a buffer.  Also, @def is returned in the case of an error.
 *
 * Returns: The value associated with @key, or @def.
 */
int
bot_conf_get_int_or_default (BotConf * conf, const char * key, int def);

/**
 * bot_conf_get_boolean_or_default:
 * @conf: The configuration.
 * @key: The key to get the value for.
 * @def: A "default" value.
 *
 * Like bot_conf_get_boolean(), except it returns the value instead of
 * putting it in a buffer.  Also, @def is returned in the case of an error.
 *
 * Returns: The value associated with @key, or @def.
 */
int
bot_conf_get_boolean_or_default (BotConf * conf, const char * key, int def);

/**
 * bot_conf_get_double_or_default:
 * @conf: The configuration.
 * @key: The key to get the value for.
 * @def: A "default" value.
 *
 * Like bot_conf_get_double(), except it returns the value instead of putting
 * it in a buffer.  Also, @def is returned in the case of an error.
 *
 * Returns: The value associated with @key, or @def.
 */
double
bot_conf_get_double_or_default (BotConf * conf, const char * key, double def);

/**
 * bot_conf_get_str_or_default:
 * @conf: The configuration.
 * @key: The key to get the value for.
 * @def: A "default" value.
 *
 * Like bot_conf_get_str(), except it returns the value instead of putting it
 * in a buffer.  Also, @def is returned in the case of an error.
 *
 * Returns: The value associated with @key, or @def.
 */
char *
bot_conf_get_str_or_default (BotConf * conf, const char * key, char * def);

/**
 * bot_conf_get_int_or_fail:
 * @conf: The configuration.
 * @key: The key to get the value for.
 *
 * Same as bot_conf_get_int(), except, on error, this will call abort().
 *
 * Returns: The int value at @key.
 */
int
bot_conf_get_int_or_fail(BotConf *conf, const char *key);

/**
 * bot_conf_get_boolean_or_fail:
 * @conf: The configuration.
 * @key: The key to get the value for.
 *
 * Same as bot_conf_get_boolean(), except, on error, this will call abort().
 *
 * Returns: The boolean value (1 or 0) at @key.
 */
int
bot_conf_get_boolean_or_fail(BotConf * conf, const char * key);

/**
 * bot_conf_get_double_or_fail:
 * @conf: The configuration.
 * @key: The key to get the value for.
 *
 * Same as bot_conf_get_double(), except, on error, this will call abort().
 *
 * Returns: The double value at @key.
 */
double bot_conf_get_double_or_fail (BotConf *conf, const char *key);

/**
 * bot_conf_get_str_or_fail:
 * @conf: The configuration.
 * @key: The key to get the string value for.
 *
 * Like bot_conf_get_double_or_fail(), except with a string instead.
 *
 * Returns: The string value at @key.
 */
char
*bot_conf_get_str_or_fail (BotConf *conf, const char *key);

/**
 * bot_conf_get_int_array:
 * @conf: The configuration:
 * @key: The key to look for an array of values.
 * @vals: An array of ints (return values).
 * @len: Number of elements in @vals array.
 *
 * Same as bot_conf_get_int(), except for an array.
 *
 * Returns: 0 on success, -1 is on error.
 */
int
bot_conf_get_int_array (BotConf * conf, const char * key, int * vals, int len);

/**
 * bot_conf_get_boolean_array:
 * @conf: The configuration:
 * @key: The key to look for an array of values.
 * @vals: An array of booleans (return values).
 * @len: Number of elements in @vals array.
 *
 * Same as bot_conf_get_boolean(), except for an array.
 *
 * Returns: 0 on success, -1 is on error.
 */
int
bot_conf_get_boolean_array (BotConf * conf, const char * key, int * vals, int len);

/**
 * bot_conf_get_double_array:
 * @conf: The configuration:
 * @key: The key to look for an array of values.
 * @vals: An array of doubles (return values).
 * @len: Number of elements in @vals array.
 *
 * Same as bot_conf_get_double(), except for an array.
 *
 * Returns: 0 on success, -1 is on error.
 */
int
bot_conf_get_double_array (BotConf * conf, const char * key, double * vals, int len);

/**
 * bot_conf_get_str_array:
 * @conf: The configuration:
 * @key: The key to look for an array of values.
 * @vals: An array of strings (return values).
 * @len: Number of elements in @vals array.
 *
 * Same as bot_conf_get_double(), except for an array.
 *
 * Returns: 0 on success, -1 is on error.
 */
int
bot_conf_get_str_array (BotConf * conf, const char * key, char ** vals, int len);

/**
 * bot_conf_get_array_len:
 * @conf: The configuration.
 * @key: The key to look for a value.
 *
 * Gets the length of the array at @key, or -1 if that key doesn't exist.
 *
 * Returns: the number of elements in the specified array, or -1 if @key
 * does not correspond to an array
 */
int bot_conf_get_array_len (BotConf *conf, const char * key);

/**
 * bot_conf_get_str_array_alloc:
 * @conf: The configuration.
 * @key: The key to look for a value.
 *
 * Allocates and returns a %NULL-terminated array of strings.  Free it with
 * bot_conf_str_array_free().
 *
 * Returns: A newly-allocated, %NULL-terminated array of strings.
 */
char **
bot_conf_get_str_array_alloc (BotConf * conf, const char * key);

/**
 * bot_conf_str_array_free:
 * @data: The %NULL-terminated string array to free.
 *
 * Frees a %NULL-terminated array of strings.
 */
void bot_conf_str_array_free ( char **data);

/**
 * bot_conf_alloc:
 *
 * Creates a new, meaningless, empty config struct.
 *
 * Returns: An uninitialized %BotConf object.
 */
BotConf *
bot_conf_alloc( void );

/**
 * bot_conf_set_int:
 * @conf: The configuration.
 * @key: The key to look for a value.
 * @val: The value to set it to.
 *
 * This function searches for a key (or creates it if it does not exist yet)
 * and stores a single value (@val) associated with that key.
 *
 * Returns: 0 on success, -1 on failure.
 */
int
bot_conf_set_int (BotConf * conf,
                const char * key,
                int val);

/**
 * bot_conf_set_boolean:
 * @conf: The configuration.
 * @key: The key to look for a value.
 * @val: The value to set it to.
 *
 * This function searches for a key (or creates it if it does not exist yet)
 * and stores a single value (@val) associated with that key.
 *
 * Returns: 0 on success, -1 on failure.
 */
int
bot_conf_set_boolean (BotConf * conf,
                    const char * key,
                    int val);

/**
 * bot_conf_set_double:
 * @conf: The configuration.
 * @key: The key to look for a value.
 * @val: The value to set it to.
 *
 * This function searches for a key (or creates it if it does not exist yet)
 * and stores a single value (@val) associated with that key.
 *
 * Returns: 0 on success, -1 on failure.
 */
int
bot_conf_set_double (BotConf * conf,
                   const char * key,
                   double val);

/**
 * bot_conf_set_str:
 * @conf: The configuration.
 * @key: The key to look for a value.
 * @val: The value to set it to.
 *
 * This function searches for a key (or creates it if it does not exist yet)
 * and stores a single value (@val) associated with that key.
 *
 * Returns: 0 on success, -1 on failure.
 */
int
bot_conf_set_str (BotConf * conf,
                const char * key,
                char * val);

/**
 * bot_conf_set_int_array:
 * @conf: The configuration.
 * @key: The key to look up (or create).
 * @vals: Array of values to set.
 * @len: Number of members in @vals.
 *
 * This function searches for a key, or creates it if it does not exist.  It
 * stores the entire input array @vals at that key.
 *
 * Returns: 0 on success, -1 on failure.
 */
int
bot_conf_set_int_array (BotConf * conf,
                      const char * key,
                      int * vals,
                      int len);

/**
 * bot_conf_set_boolean_array:
 * @conf: The configuration.
 * @key: The key to look up (or create).
 * @vals: Array of values to set.
 * @len: Number of members in @vals.
 *
 * This function searches for a key, or creates it if it does not exist.  It
 * stores the entire input array @vals at that key.
 *
 * Returns: 0 on success, -1 on failure.
 */
int
bot_conf_set_boolean_array (BotConf * conf,
                          const char * key,
                          int * vals,
                          int len);

/**
 * bot_conf_set_double_array:
 * @conf: The configuration.
 * @key: The key to look up (or create).
 * @vals: Array of values to set.
 * @len: Number of members in @vals.
 *
 * This function searches for a key, or creates it if it does not exist.  It
 * stores the entire input array @vals at that key.
 *
 * Returns: 0 on success, -1 on failure.
 */
int
bot_conf_set_double_array (BotConf * conf,
                         const char * key,
                         double * vals,
                         int len);

/**
 * bot_conf_set_str_array:
 * @conf: The configuration.
 * @key: The key to look up (or create).
 * @vals: Array of values to set.
 * @len: Number of members in @vals.
 *
 * This function searches for a key, or creates it if it does not exist.  It
 * stores the entire input array @vals at that key.
 *
 * Returns: 0 on success, -1 on failure.
 */
int
bot_conf_set_str_array (BotConf * conf,
                      const char * key,
                      char ** vals,
                      int len);


#ifdef __cplusplus
}
#endif

#endif
