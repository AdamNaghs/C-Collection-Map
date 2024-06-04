#include "map.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int test0()
{
    MapTypeData type = {
        .key_size = sizeof(char *),
        .value_size = sizeof(int),
        .key_hash = NULL,
        .key_cmp = map_default_cmp_str,
        .key_free = NULL,
        .value_free = NULL};

    Map *map = map_new(type, MAP_DEFAULT_BUCKETS_COUNT);
    if (map == NULL)
    {
        return -1;
    }

    char *key = "key";
    int value = 42;
    map_add(map, &key, &value);

    int *result = map_get(map, &key);
    printf("Test0 result: %d\n\t", *result);
    printf("%s\n", *result == 42 ? "Success" : "Failure");

    map_free(map);

    return 0;
}

size_t int_hash(const void *key)
{
    return *(int *)key;
}

int int_cmp(const void *a, const void *b)
{
    return *(int *)a - *(int *)b;
}

void int_free(void *key)
{
    // Free the value pointed to by the key
    // In this case, we assume the key is an int, so no action is needed
}

void value_free(void *value)
{
    // Free the value pointed to by the value
    // In this case, we assume the value is an int, so no action is needed
}

void test_map()
{
    MapTypeData type;
    type.key_size = sizeof(int);
    type.value_size = sizeof(int);
    type.key_hash = int_hash;
    type.key_cmp = int_cmp;
    type.key_free = int_free;
    type.value_free = value_free;

    Map *map = map_new(type, MAP_DEFAULT_BUCKETS_COUNT);
    if (map == NULL)
    {
        printf("Failed to create map\n");
        return;
    }

    // Test adding and retrieving elements
    for (int i = 0; i < 100; i++)
    {
        int key = i;
        int value = i * 10;
        int result = map_add(map, &key, &value);
        if (result != 0)
        {
            printf("Failed to add key %d\n", i);
        }
    }

    for (int i = 0; i < 100; i++)
    {
        int key = i;
        int *value = (int *)map_get(map, &key);
        if (value == NULL || *value != i * 10)
        {
            printf("Failed to retrieve key %d\n", i);
        }
    }

    // Test updating an existing key
    int key = 50;
    int new_value = 500;
    map_add(map, &key, &new_value);
    int *value = (int *)map_get(map, &key);
    if (value == NULL || *value != 500)
    {
        printf("Failed to update key %d\n", key);
    }

    // Test removing elements
    for (int i = 0; i < 100; i++)
    {
        int key = i;
        map_remove(map, &key);
        int *value = (int *)map_get(map, &key);
        if (value != NULL)
        {
            printf("Failed to remove key %d\n", i);
        }
    }

    // Test map load factor and resizing
    for (int i = 0; i < 1000; i++)
    {
        int key = i;
        int value = i * 10;
        map_add(map, &key, &value);
    }

    double load_factor = map_load_factor(map);
    if (load_factor > 0.75)
    {
        printf("Load factor too high: %f\n", load_factor);
    }

    // Test collision counting
    size_t collisions = map_count_collisions(map);
    printf("Number of collisions: %zu\n", collisions);

    clock_t start = clock();
    map_optimize(&map);
    clock_t end = clock();
    load_factor = map_load_factor(map);

    printf("Load factor after optimization: %f\n", load_factor);
    collisions = map_count_collisions(map);
    printf("Number of collisions after optimization: %zu\n", collisions);
    printf("Optimization time: %f seconds\n", ((double)(end - start)) / CLOCKS_PER_SEC);
    map_free(map);
}

void dyn_test()
{
    MapTypeData type = MAP_TYPE(char *, int, map_default_hash_str, map_default_cmp_str, map_deref_free, NULL);

    Map *map = map_new(type, 10);
    for (int i = 0; i < 10; i++)
    {
        char *key = malloc(10);
        sprintf(key, "key%d", i);
        int value = i;
        map_add(map, &key, &value); // Pass the address of the key and value
    }

    for (int i = 0; i < 10; i++)
    {
        char key[10];
        sprintf(key, "key%d", i);
        char *key_ptr = key;
        int *result = (int *)map_get(map, &key_ptr); // Pass the address of the key pointer
        if (result == NULL || *result != i)
        {
            printf("Failed to retrieve key %s\n", key);
        }
        else
        {
            printf("Retrieved key %s with value %d\n", key, *result);
        }
    }

    MAP_FOR_EACH(map, char*, key, int, value)
    {
        printf("Key: %s, Value: %d\n", *key, *value);
    }

    char** key;
    int* value;
    MapNode* node;
    size_t i;
    MAP_FOR_EACH_ANSI(map, i, node, char*, key, int, value)
    {
        printf("Key: %s, Value: %d\n", *key, *value);
    }

    MAP_FOR_EACH(map, char*, key, int, value)
    {
        map_remove(map, key);
    }

    map_free(map);
}

int main(void)
{
    test0();
    test_map();
    dyn_test();

    return 0;
}