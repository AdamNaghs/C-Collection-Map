#include "map.h"
#include <stdlib.h>
#include <string.h>

static size_t map_default_hash(const void *key)
{
    return *(size_t *)key;
}

static int map_default_cmp(const void *key1, const void *key2)
{
    return *(int *)key1 - *(int *)key2;
}

void map_deref_free(void *ptr)
{
    free(*(void **)ptr);
}

/* doesnt free anything because it is just a placeholder.
    A function that freed something would have to cast the void* to the correct type and free the value it points to.

    Example:
    void bigint_map_free(void *key)
    {
        struct BigInt *bi = (struct BigInt *)key;
        bigint_free(*bi);
    }
*/
static void map_default_free(void *ptr)
{
    return;
}

Map *map_new(MapTypeData type, size_t buckets_count)
{
    Map *map = (Map *)malloc(sizeof(Map));
    if (map == NULL)
    {
        return NULL;
    }
    if (type.key_hash == NULL)
    {
        type.key_hash = map_default_hash;
    }
    if (type.key_cmp == NULL)
    {
        type.key_cmp = map_default_cmp;
    }
    if (type.key_free == NULL)
    {
        type.key_free = map_default_free;
    }
    if (type.value_free == NULL)
    {
        type.value_free = map_default_free;
    }

    map->type = type;
    map->length = 0;
    map->buckets_count = buckets_count;
    map->buckets = (MapNode *)calloc(map->buckets_count, sizeof(MapNode));
    if (map->buckets == NULL)
    {
        free(map);
        return NULL;
    }

    return map;
}

void map_free(Map *map)
{
    size_t i;
    for (i = 0; i < map->buckets_count; i++)
    {
        MapNode *node = map->buckets[i].next;
        while (node != NULL)
        {
            MapNode *next = node->next;
            if (map->type.key_free != NULL)
                map->type.key_free(node->key);
            if (map->type.value_free != NULL)
                map->type.value_free(node->value);
            free(node->key);
            free(node->value);
            free(node);
            node = next;
        }
    }

    free(map->buckets);
    free(map);
}

int map_add(Map *map, const void *key, const void *value)
{
    size_t hash = map->type.key_hash(key);
    size_t index = hash % map->buckets_count;

    MapNode *node = &map->buckets[index];

    if (node->key == NULL && node->value == NULL)
    {
        /* First time accessing this bucket. */
        node->key = (byte *)malloc(map->type.key_size);
        if (node->key == NULL)
        {
            return -1;
        }

        node->value = (byte *)malloc(map->type.value_size);
        if (node->value == NULL)
        {
            free(node->key);
            return -1;
        }

        memcpy(node->key, key, map->type.key_size);
        memcpy(node->value, value, map->type.value_size);

        map->length++;
        return 0;
    }

    while (node->next != NULL)
    {
        if (map->type.key_cmp(node->key, key) == 0)
        {
            /* Key already contained, update value */
            memcpy(node->value, value, map->type.value_size);
            return 1;
        }
        node = node->next;
    }
    /* Add node. */
    MapNode *new_node = (MapNode *)malloc(sizeof(MapNode));
    if (new_node == NULL)
    {
        return -1;
    }

    new_node->key = (byte *)malloc(map->type.key_size);
    if (new_node->key == NULL)
    {
        free(new_node);
        return -1;
    }

    new_node->value = (byte *)malloc(map->type.value_size);
    if (new_node->value == NULL)
    {
        free(new_node->key);
        free(new_node);
        return -1;
    }

    memcpy(new_node->key, key, map->type.key_size);
    memcpy(new_node->value, value, map->type.value_size);
    new_node->next = NULL;

    node->next = new_node;
    map->length++;
    return 0;
}

void *map_get(Map *map, const void *key)
{
    size_t hash = map->type.key_hash(key);
    size_t index = hash % map->buckets_count;

    MapNode *node = &map->buckets[index];
    while (node != NULL)
    {
        if (node->key == NULL && node->value == NULL)
        {
            return NULL;
        }
        int cmp = map->type.key_cmp(node->key, key);
        if (cmp == 0)
        {
            return node->value;
        }
        node = node->next;
    }

    return NULL;
}

void map_remove(Map *map, const void *key)
{
    size_t hash = map->type.key_hash(key);
    size_t index = hash % map->buckets_count;

    MapNode *node = &map->buckets[index];
    MapNode *prev = NULL;
    while (node != NULL)
    {
        if (node->key == NULL && node->value == NULL)
        {
            return;
        }
        if (map->type.key_cmp(node->key, key) == 0)
        {
            if (prev == NULL)
            {
                if (node->next != NULL)
                {
                /* promote next to a bucket */
                map->buckets[index] = *node->next;
                }
            }
            else
            {
                /* Remove node from linked list */
                prev->next = node->next;
            }
            map->type.key_free(node->key);
            map->type.value_free(node->value);
            free(node->key);
            free(node->value);
            node->key = NULL;
            node->value = NULL;
            if (prev)
            {
                /* Free node if its not a bucket */
                free(node);
                node = NULL;
            }
            map->length--;
            return;
        }
        prev = node;
        node = node->next;
    }
}

double map_load_factor(const Map *map)
{
    return (double)map->length / (double)map->buckets_count;
}

size_t map_count_collisions(const Map *map)
{
    size_t collisions = 0, i;
    for (i = 0; i < map->buckets_count; i++)
    {
        MapNode *node = map->buckets[i].next;
        if (node != NULL)
        {
            collisions++;
            while (node->next != NULL)
            {
                collisions++;
                node = node->next;
            }
        }
    }
    return collisions;
}

void map_optimize(Map **inp)
{
    Map *map = *inp;
    size_t new_buckets_count = (size_t)((double)map->length / 0.75), i;
    Map *new_map = map_new(map->type, new_buckets_count);

    for (i = 0; i < map->buckets_count; i++)
    {
        MapNode *node = map->buckets[i].next;
        while (node != NULL)
        {
            map_add(new_map, node->key, node->value);

            MapNode *next = node->next;
            node = next;
        }
    }
    map->type.key_free = NULL;
    map->type.value_free = NULL;
    map_free(map);
    *inp = new_map;
}

size_t map_default_hash_str(const void *key)
{
    char *str = *(char **)key;
    size_t len = strlen(str), i;
    const int p = 31;
    const int m = 1e9 + 9;
    long long hash_value = 0;
    long long p_pow = 1;
    for (i = 0; i < len; i++)
    {
        char c = str[i];
        hash_value = (hash_value + (c - 'a' + 1) * p_pow) % m;
        p_pow = (p_pow * p) % m;
    }
    return hash_value;
}

int map_default_cmp_str(const void *a, const void *b)
{
    return strcmp(*(char **)a, *(char **)b);
}