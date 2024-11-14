#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_KEY_LENGTH 100
#define MAX_VALUE_LENGTH 100

typedef struct {
    char key[MAX_KEY_LENGTH];
    char value[MAX_VALUE_LENGTH];
} DataEntry;

typedef struct {
    DataEntry* entries;
    int size;
    int capacity;
} InMemoryDatabase;

InMemoryDatabase* init_database(int capacity) {
    InMemoryDatabase* db = (InMemoryDatabase*)malloc(sizeof(InMemoryDatabase));
    if (db == NULL) {
        printf("Memory allocation failed for database\n");
        return NULL;
    }

    db->entries = (DataEntry*)malloc(capacity * sizeof(DataEntry));
    if (db->entries == NULL) {
        printf("Memory allocation failed for entries\n");
        free(db);
        return NULL;
    }

    db->size = 0;
    db->capacity = capacity;
    return db;
}

void add_data(InMemoryDatabase* db, const char* key, const char* value) {
    if (db->size < db->capacity) {
        strncpy_s(db->entries[db->size].key, key, MAX_KEY_LENGTH);
        strncpy_s(db->entries[db->size].value, value, MAX_VALUE_LENGTH);
        db->size++;
    }
    else {
        printf("Database is full!\n");
    }
}

char* get_data(InMemoryDatabase* db, const char* key) {
    for (int i = 0; i < db->size; i++) {
        if (strncmp(db->entries[i].key, key, MAX_KEY_LENGTH) == 0) {
            return db->entries[i].value;  // Return the value if the key matches
        }
    }
    return NULL;  // Return NULL if the key is not found
}

void write_floats_to_file(const char* filename, float* numbers, size_t count) {
    FILE* file;
    errno_t err = fopen_s(&file, filename, "wb");  // Secure fopen_s usage
    if (err != 0 || file == NULL) {
        perror("Unable to open file for writing");
        exit(1);
    }

    size_t result = fwrite(numbers, sizeof(float), count, file);
    if (result != count) {
        perror("Error writing data to file");
        exit(1);
    }

    fclose(file);
}

void read_floats_from_file(const char* filename, float* numbers, size_t count) {
    FILE* file;
    errno_t err = fopen_s(&file, filename, "rb");  // Secure fopen_s usage
    if (err != 0 || file == NULL) {
        perror("Unable to open file for reading");
        exit(1);
    }

    size_t result = fread(numbers, sizeof(float), count, file);
    if (result != count) {
        perror("Error reading data from file");
        exit(1);
    }

    fclose(file);
}