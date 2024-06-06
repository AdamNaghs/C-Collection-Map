#ifndef _MAP_H
#define _MAP_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

    /**
     * @brief When using key_free and value_free, the user should not free the void* they are given.
     * They should instead cast the void* to the correct type and free the value they are pointing to.
     * DO NOT FREE THE POINTER ITSELF.
     * ONLY FREE THE VALUE IT POINTS TO.
     *
     * @details You can set any of these to NULL if you do not want to free the key or value or want to use the default cmp or hash.
     *
     * @warning The default hash and cmp functions are not suitable for all types. The default hash hashes the memory address of the key, and the default cmp compares the memory addresses of the keys.
     */
    typedef struct
    {
        size_t key_size;
        size_t value_size;
        size_t (*key_hash)(const void *);
        int (*key_cmp)(const void *, const void *);
        void (*key_free)(void *);
        void (*value_free)(void *);
    } MapTypeData;

    typedef uint8_t byte;

    typedef struct MapNode
    {
        byte *key;
        byte *value;
        struct MapNode *next;
    } MapNode;

    typedef struct
    {
        MapTypeData type;
        size_t length;
        size_t buckets_count;
        MapNode *buckets;
    } Map;

#define MAP_DEFAULT_BUCKETS_COUNT 16

#define MAP_TYPE(_key_type, _value_type, _key_hash, _key_cmp, _key_free, _value_free) \
    (MapTypeData)                                                                     \
    {                                                                                 \
        .key_size = sizeof(_key_type),                                                \
        .value_size = sizeof(_value_type),                                            \
        .key_hash = _key_hash,                                                        \
        .key_cmp = _key_cmp,                                                          \
        .key_free = _key_free,                                                        \
        .value_free = _value_free                                                     \
    }

#define STR_MAP_TYPE MAP_TYPE(char *, int, map_default_hash_str, map_default_cmp_str, NULL, NULL)

#define MAP_TYPE_DEFAULT(key_type, value_type) MAP_TYPE(key_type, value_type, map_default_hash, map_default_cmp, map_default_free, map_default_free)

#define MAP(key_type, value_type)                   \
    map_new(MAP_TYPE_DEFAULT(key_type, value_type), \
            MAP_DEFAULT_BUCKETS_COUNT)

#define MAP_N(key_type, value_type, bucket_count)                   \
    map_new(MAP_TYPE_DEFAULT(key_type, value_type), \
            bucket_count)

#define MAP_T(type) map_new(type, MAP_DEFAULT_BUCKETS_COUNT)

/**
 * @brief Will crash if the value is NULL.
 *
 */
#define MAP_UNWRAP_VALUE(type, value) (*((type *)value))

/**
 * @brief Proivdes the key and value as pointers to the correct type. The key and value are only valid inside of the loop. Not ANSI C, but a useful macro for iterating over the map.
 *
 * @warning User is responsible for casting the key and value pointers to the correct type.
 *
 * @details Example:
 * (char*, int)
 *  MapTypeData type = MAP_TYPE(char *, int, map_default_hash_str, map_default_cmp_str, map_deref_free, NULL);
 *  Map *map = map_new(type, 10);
 *  MAP_FOR_EACH(map, char*, key, int, value)
 *  {
 *      printf("Key: %s, Value: %d\n", *key, *value);
 *  }
 */
#define MAP_FOR_EACH(map, key_type, key, value_type, value)                                                 \
    for (size_t __idx = 0; map && (__idx < map->buckets_count); __idx++)                                    \
        for (MapNode *__map_node = &map->buckets[__idx]; __map_node != NULL; __map_node = __map_node->next) \
            for (key_type *key = (key_type *)__map_node->key; key != NULL; key = NULL)                      \
                for (value_type *value = (value_type *)__map_node->value; value != NULL; value = NULL)

/**
 * @brief Example:
 * char** key;
 *  int* value;
 *  MapNode* node;
 *  size_t i;
 *  MAP_FOR_EACH_ANSI(map, i, node, char*, key, int, value)
 *  {
 *      printf("Key: %s, Value: %d\n", *key, *value);
 *  }
 *
 */
#define MAP_FOR_EACH_ANSI(map, __idx, map_node_ptr, key_type, key, value_type, value)                      \
    for (__idx = 0; map && (__idx < map->buckets_count); __idx++)                                          \
        for (map_node_ptr = &map->buckets[__idx]; map_node_ptr != NULL; map_node_ptr = map_node_ptr->next) \
            for (key = (key_type *)map_node_ptr->key; key != NULL; key = NULL)                             \
                for (value = (value_type *)map_node_ptr->value; value != NULL; value = NULL)

    /**
     * @brief Create a new map.
     *
     * @param type
     * @return Map*
     */
    Map *map_new(MapTypeData type, size_t buckets_count);

    /**
     * @brief Free the map.
     *
     * @param map
     */
    void map_free(Map *map);

    /**
     * @brief
     *
     * @param map
     * @param key Valid memory address to key.
     * @param value Valid memory address to value.
     * @return 0 on success, -1 on failure, 1 if the key is already in the map and it updated the value.
     */
    int map_add(Map *map, const void *key, const void *value);

    /**
     * @brief Find a key in the map, and return its value pair.
     *
     * @param map
     * @param key Valid memory address to key.
     * @return void* to matching key or NULL if not found.
     */
    void *map_get(Map *map, const void *key);

    /**
     * @brief Remove a key from the map.
     *
     * @param map
     * @param key Valid memory address to key.
     * @return 0 on success, -1 on failure.
     *
     * @note If the key is not found, nothing happens.
     *
     * @details Calls the key_free and value_free functions if they are set.
     */
    int map_remove(Map *map, const void *key);

    /**
     * @brief Get the number of elements in the map.
     *
     * @param map
     * @return double
     */
    double map_load_factor(const Map *map);

    /**
     * @brief Get the number of elements in the map.
     *
     * @param map
     * @return size_t
     */
    size_t map_count_collisions(const Map *map);

    /**
     * @brief Resize the map to have a load factor of 0.75.
     *
     * @param map
     * @return void
     *
     * @details This function will create a new map of the correct size, and add all elements from the old map to the new map. Then it will free the old map and set the pointer to the new map.
     *
     * @note If the load factor is above 0.75 after optimzing, the hash function may not be suitable for the data.
     *
     */
    void map_optimize(Map **map);

    /**
     * @brief Pass to MAP_TYPE to use the default hash function for strings.
     *
     * @param key
     * @return size_t
     *
     * @details Hashes the string pointed to by key.
     *
     * @warning Found on wikipedia. Probably not the best string hash function.
     */
    size_t map_default_hash_str(const void *key);

    /**
     * @brief Pass to MAP_TYPE to use the default compare function for strings.
     *
     * @param a
     * @param b
     * @return int
     *
     * @details Compares the strings pointed to by a and b.
     *
     * @warning Uses strcmp.
     */
    int map_default_cmp_str(const void *a, const void *b);

    /**
     * @brief Pass to MAP_TYPE to use the default free function for strings.
     * 
     * @param ptr 
     * 
     * @note Suiteable for keys or values allocated with malloc.
     * 
     * @details Derefrences the pointer and frees the memory.
     */
    void map_deref_free(void *ptr);

#define map_default_free_str map_deref_free

    /**
     * @brief Pass to MAP_TYPE to use the default hash function.
     *
     * @param key
     * @return size_t
     *
     * @details Uses the pointer value as the hash.
     */
    size_t map_default_hash(const void *key);

    /**
     * @brief Pass to MAP_TYPE to use the default compare function.
     *
     * @param key1
     * @param key2
     * @return int
     *
     * @details Compares the pointers as size_t.
     */
    int map_default_cmp(const void *key1, const void *key2);

    /**
     * @brief Pass to MAP_TYPE to use the default free function.
     *
     * @param ptr
     *
     * @details Does nothing. Suitable for values that do not need to be freed.
     */
    void map_default_free(void *ptr);

#ifdef __cplusplus
} /* Extern "C" */
#endif

#endif /* _MAP_H */