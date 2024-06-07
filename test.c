#include "map.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../Testing/testing.h"

size_t int_hash(const void *key)
{
    return *(int *)key;
}

int int_cmp(const void *a, const void *b)
{
    return *(int *)a - *(int *)b;
}

TEST_MAKE(Map_Local_Value)
{
    MapTypeData type = MAP_TYPE(int, int, int_hash, int_cmp, NULL, NULL);
    Map *map = map_new(type, 10);
    TEST_ASSERT_MSG(map != NULL, "Failed to create map");

    // Test adding and retrieving elements
    for (int i = 0; i < 100; i++)
    {
        int key = i;
        int value = i * 10;
        int result = map_add(map, &key, &value);
        TEST_ASSERT_MSG(result == 0, "Failed to add key");
    }

    for (int i = 0; i < 100; i++)
    {
        int key = i;
        int *value = (int *)map_get(map, &key);
        TEST_ASSERT_MSG(value != NULL, "Failed to retrieve key");
    }

    // Test updating an existing key
    int key = 50;
    int new_value = 500;
    map_add(map, &key, &new_value);
    int *value = (int *)map_get(map, &key);
    TEST_ASSERT_MSG(value, "Failed to retrieve key");
    TEST_ASSERT_MSG(*value == new_value, "Failed to update key");

    // Test removing elements
    for (int i = 0; i < 100; i++)
    {
        int key = i;
        map_remove(map, &key);
        int *value = (int *)map_get(map, &key);
        TEST_ASSERT_MSG(value == NULL, "Failed to remove key");
    }

    // Test map load factor and resizing
    for (int i = 0; i < 1000; i++)
    {
        int key = i;
        int value = i * 10;
        int ret = map_add(map, &key, &value);
        TEST_ASSERT_MSG(ret == 0, "Failed to add key");
    }

    double load_factor = map_load_factor(map);

    // Test collision counting
    size_t collisions = map_count_collisions(map);
    map_optimize(&map);
    TEST_ASSERT_MSG(map != NULL, "Failed to optimize map");
    TEST_ASSERT_MSG(map_load_factor(map) < load_factor, "Failed to optimize map");
    TEST_ASSERT_MSG(map_count_collisions(map) <= collisions, "Failed to optimize map, too many collisions");

    map_free(map);
    TEST_PASS();
}

TEST_MAKE(Heap_Str)
{
    MapTypeData type = MAP_TYPE(char *, int, map_default_hash_str, map_default_cmp_str, map_default_free_str, NULL);

    Map *map = map_new(type, 10);
    for (int i = 0; i < 10; i++)
    {
        char *key = malloc(10);
        sprintf(key, "key%d", i);
        int value = i;
        int ret = map_add(map, &key, &value);
        TEST_ASSERT_CLEAN_FMT(ret == 0, map_free(map), "Failed to add key, ret: %d", i);
        int *result = (int *)map_get(map, &key);
        TEST_ASSERT_CLEAN_FMT(result != NULL, map_free(map), "Failed to retrieve key %s", key);
        TEST_ASSERT_CLEAN_FMT(*result == i, map_free(map), "Failed to retrieve key %s with value %d", key, *result);
    }
    map_free(map);
    TEST_PASS();
}

size_t int_ptr_hash(const void *key)
{
    return *(int *)key;
}

TEST_MAKE(Heap_Int)
{
    MapTypeData type = MAP_TYPE(int*, int, int_ptr_hash, int_cmp, map_deref_free, NULL);
    Map *map = map_new(type, 1000);
    const int max = 100000;
    for (int i = 0; i < max; i++)
    {
        int* key = malloc(sizeof(int));
        *key = i;
        int value = i * 10;
        int ret = map_add(map, &key, &value);
        TEST_ASSERT_CLEAN_FMT(ret == 0, map_free(map), "Failed to add key, ret: %d", i);
        int *result = (int *)map_get(map, &key);
        TEST_ASSERT_CLEAN_FMT(result != NULL, map_free(map), "Failed to retrieve key %d", key);
        TEST_ASSERT_CLEAN_FMT(*result == value, map_free(map), "Failed to retrieve key %d with value %d", *key, *result);
    }
    size_t col = map_count_collisions(map);
    map_optimize(&map);
    TEST_ASSERT_CLEAN_MSG(map != NULL, map_free(map), "Map optimze returned NULL");
    TEST_ASSERT_CLEAN_MSG(map_count_collisions(map) <= col, map_free(map), "Failed to optimize map, too many collisions");
    map_free(map);
    TEST_PASS();
}

TEST_SUITE_MAKE(Map)
{
    TEST_SUITE_LINK(Map, Heap_Str);
    TEST_SUITE_LINK(Map, Map_Local_Value);
    TEST_SUITE_LINK(Map, Heap_Int);
    TEST_SUITE_END(Map);
}

int main(void)
{
    TEST_SUITE_RUN(Map);
    return 0;
}