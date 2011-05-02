#ifndef __bot_param_client_h__
#define __bot_param_client_h__

#include <stdio.h>
#include <lcm/lcm.h>

/**
 * SECTION:param
 * @title: Configuration Files
 * @short_description: Hierarchical key/value configuration files
 * @include: bot2-core/bot2-core.h
 *
 * TODO
 *
 * Linking: -lparam_client
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _BotParam BotParam;

/**
 * bot_param_new_from_server:
 * @lcm: Handle to the lcm object to be used
 * @keep_update: keep listening for parameter updates
 *
 * Gets the params for the param-server, and parse them. Returns a handle to the contents.
 * If no parameters are received within 5seconds, returns with an error.
 *
 * WARNING: This calls lcm_handle internally, so make sure that you create the param_client
 * BEFORE you subscribe with handlers that may use it!
 *
 * Returns: A handle to a newly-allocated %BotParam or %NULL on error.
 */
BotParam *
bot_param_new_from_server (lcm_t * lcm, int keep_updated);

/**
 * bot_param_new_from_file:
 * @filename: The name of the file @f.
 *
 * Parses a file and returns a handle to the contents.
 *
 * Returns: A handle to a newly-allocated %BotParam or %NULL on parse error.
 */
BotParam *
bot_param_new_from_file (const char * filename);

/**
 * bot_param_new_from_string:
 * @string: A character array containing the config file.
 * @length: The length of the character array
 *
 * Parses the string and returns a handle to the contents.
 *
 * Returns: A handle to a newly-allocated %BotParam or %NULL on parse error.
 */
BotParam *
bot_param_new_from_string (const char * string, int length);

/**
 * bot_param_alloc:
 *
 * Creates a new, meaningless, empty config struct.
 *
 * Returns: An uninitialized %BotParam object.
 */
BotParam *
bot_param_new( void );

/**
 * bot_param_free:
 * @param: The %BotParam to free.
 *
 * Frees a configuration handle
 */
void
bot_param_destroy (BotParam * param);

/**
 * bot_param_write:
 * @param: The configuration to write.
 * @f: The file handle to write to
 *
 * Writes the contents of a parsed configuration file to file handle @f.
 *
 * Returns: -1 on error, 0 on success.
 */
int
bot_param_write (BotParam * param, FILE * f);

/**
 * bot_param_write_to_string:
 * @param: the configuration to write.
 * @s: string to write to.
 *
 * The string will be allocated by the library, but must
 * be freed by the user.
 *
 * returns: -1 on error, 0 on success.
 */
int
bot_param_write_to_string (BotParam * param, char ** s);

/**
 * bot_param_print:
 * @param: The configuration to print.
 *
 * Prints the contents of a parsed configuration file.
 *
 * Returns: -1 on error, 0 on success.
 */
static inline int
bot_param_print (BotParam * param){
  return bot_param_write (param, stdout);
}

/**
 * bot_param_has_key:
 * @param: The configuration.
 * @key: The key to check the existence of.
 *
 * Checks if a key named @key exists in the file that was read in by @param.
 *
 * Returns: 1 if the key is present, 0 if not
 */
int bot_param_has_key (BotParam *param, const char *key);

/**
 * bot_param_get_num_subkeys:
 * @param: The configuration.
 * @containerKey: The key to check.
 *
 * Finds the number of sub keys of @containerKey that also have sub keys.
 *
 * Returns: The number of top-level keys in container named by containerKey or
 * -1 if container key not found.
 */
int
bot_param_get_num_subkeys (BotParam * param, const char * containerKey);

/**
 * bot_param_get_subkeys:
 * @param: The configuration.
 * @containerKey:
 *
 * Fetch all top-level keys in container named by containerKey.
 *
 * Returns: a newly allocated, %NULL-terminated, array of strings.  This array
 * should be freed by the caller (e.g. with g_strfreev)
 */
char **
bot_param_get_subkeys (BotParam * param, const char * containerKey);
                     


/**
 * bot_param_get_int:
 * @param: The configuration.
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
bot_param_get_int (BotParam * param, const char * key, int * val);

/**
 * bot_param_get_boolean:
 * @param: The configuration.
 * @key: The key to get the value for.
 * @val: Return value.
 *
 * Same as bot_param_get_int(), only for a boolean.
 *
 * Returns: 0 on success, -1, on failure.
 */
int
bot_param_get_boolean (BotParam * param, const char * key, int * val);

/**
 * bot_param_get_double:
 * @param: The configuration.
 * @key: The key to get the value for.
 * @val: Return value.
 *
 * Same as bot_param_get_int(), only for a double.
 *
 * Returns: 0 on success, -1, on failure.
 */
int
bot_param_get_double (BotParam * param, const char * key, double * val);

/**
 * bot_param_get_str:
 * @param: The configuration.
 * @key: The key to get the value for.
 * @val: Return value.
 *
 * Same as bot_param_get_int(), only for a string.
 * NOTE: this function returns a DUPLICATE of the string value, which should be freed!
 *
 * Returns: 0 on success, -1, on failure.
 */
int
bot_param_get_str (BotParam * param, const char * key, char ** val);


/**
 * bot_param_get_int_or_fail:
 * @param: The configuration.
 * @key: The key to get the value for.
 *
 * Same as bot_param_get_int(), except, on error, this will call abort().
 *
 * Returns: The int value at @key.
 */
int
bot_param_get_int_or_fail(BotParam *param, const char *key);

/**
 * bot_param_get_boolean_or_fail:
 * @param: The configuration.
 * @key: The key to get the value for.
 *
 * Same as bot_param_get_boolean(), except, on error, this will call abort().
 *
 * Returns: The boolean value (1 or 0) at @key.
 */
int
bot_param_get_boolean_or_fail(BotParam * param, const char * key);

/**
 * bot_param_get_double_or_fail:
 * @param: The configuration.
 * @key: The key to get the value for.
 *
 * Same as bot_param_get_double(), except, on error, this will call abort().
 *
 * Returns: The double value at @key.
 */
double bot_param_get_double_or_fail (BotParam *param, const char *key);

/**
 * bot_param_get_str_or_fail:
 * @param: The configuration.
 * @key: The key to get the string value for.
 *
 * Like bot_param_get_double_or_fail(), except with a string instead.
 * NOTE: this function returns a DUPLICATE of the string value, which should be freed!
 *
 * Returns: The string value at @key.
 */
char
*bot_param_get_str_or_fail (BotParam *param, const char *key);

/**
 * bot_param_get_int_array:
 * @param: The configuration:
 * @key: The key to look for an array of values.
 * @vals: An array of ints (return values).
 * @len: Number of elements in @vals array.
 *
 * Same as bot_param_get_int(), except for an array.
 *
 * Returns: Number of elements read or -1 on error.
 */
int
bot_param_get_int_array (BotParam * param, const char * key, int * vals, int len);

/**
 * bot_param_get_int_array_or_fail:
 * @param: The configuration:
 * @key: The key to look for an array of values.
 * @vals: An array of ints (return values).
 * @len: Number of elements in @vals array.
 *
 * Same as bot_param_get_int_or_fail(), except for an array. Calls abort() if
 * the number of elements read is less than len.
 *
 */
void
bot_param_get_int_array_or_fail (BotParam * param, const char * key, int * vals, int len);


/**
 * bot_param_get_boolean_array:
 * @param: The configuration:
 * @key: The key to look for an array of values.
 * @vals: An array of booleans (return values).
 * @len: Number of elements in @vals array.
 *
 * Same as bot_param_get_boolean(), except for an array.
 *
 * Returns: Number of elements read or -1 on error.
 */
int
bot_param_get_boolean_array (BotParam * param, const char * key, int * vals, int len);


/**
 * bot_param_get_boolean_array_or_fail:
 * @param: The configuration:
 * @key: The key to look for an array of values.
 * @vals: An array of booleans (return values).
 * @len: Number of elements in @vals array.
 *
 * Same as bot_param_get_boolean_or_fail(), except for an array. Calls abort() if
 * the number of elements read is less than len. 
 *
 */
void
bot_param_get_boolean_array_or_fail (BotParam * param, const char * key, int * vals, int len);


/**
 * bot_param_get_double_array:
 * @param: The configuration:
 * @key: The key to look for an array of values.
 * @vals: An array of doubles (return values).
 * @len: Number of elements in @vals array.
 *
 * Same as bot_param_get_double(), except for an array.
 *
 * Returns: Number of elements read or -1 on error.
 */
int
bot_param_get_double_array (BotParam * param, const char * key, double * vals, int len);


/**
 * bot_param_get_double_array_or_fail:
 * @param: The configuration:
 * @key: The key to look for an array of values.
 * @vals: An array of doubles (return values).
 * @len: Number of elements in @vals array.
 *
 * Same as bot_param_get_double(), except for an array. Calls abort() if
 * the number of elements read is less than len. 
 *
 */
void
bot_param_get_double_array_or_fail (BotParam * param, const char * key, double * vals, int len);


/**
 * bot_param_get_array_len:
 * @param: The configuration.
 * @key: The key to look for a value.
 *
 * Gets the length of the array at @key, or -1 if that key doesn't exist.
 *
 * Returns: the number of elements in the specified array, or -1 if @key
 * does not correspond to an array
 */
int bot_param_get_array_len (BotParam *param, const char * key);

/**
 * bot_param_get_str_array_alloc:
 * @param: The configuration.
 * @key: The key to look for a value.
 *
 * Allocates and returns a %NULL-terminated array of strings.  Free it with
 * bot_param_str_array_free().
 *
 * Returns: A newly-allocated, %NULL-terminated array of strings.
 */
char **
bot_param_get_str_array_alloc (BotParam * param, const char * key);

/**
 * bot_param_str_array_free:
 * @data: The %NULL-terminated string array to free.
 *
 * Frees a %NULL-terminated array of strings.
 */
void bot_param_str_array_free ( char **data);


/**
 * bot_param_get_global:
 * @lcm: The lcm object for the global_param client to use
 * @keep_updated: Set to 1 to keep the BotParam updated
 *
 * Upon first being called, this function instantiates and returns a new
 * BotParam instance. Subsequent calls return the same BotParam instance.
 *
 * WARNING: Creating the param_client calls lcm_handle internally,
 * so make sure that you create the param_client BEFORE
 * you subscribe handlers that may use it!
 *
 * Returns: pointer to BotParam
 */
BotParam*
bot_param_get_global(lcm_t * lcm,int keep_updated);


int64_t bot_param_get_server_id(BotParam * param);
int bot_param_get_seqno(BotParam * param);


#ifdef __cplusplus
}
#endif

#endif
