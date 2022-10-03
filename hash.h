#pragma once

//
// Header
//

#include <stdbool.h>
#include <stdint.h>

// Types

typedef void *HASH;
typedef void *HASH_ITER;

typedef bool HASH_eq_fn(void *a, void *b);
typedef uint32_t HASH_fn(void *obj, uint32_t maxhash);

// Interface

HASH hash_init_default(void);
HASH hash_init(uint32_t size, float thresh, HASH_eq_fn *eq, HASH_fn *hash);

uint32_t hash_size(HASH table);
int hash_set(HASH table, void *key, void *val);
int hash_get(HASH table, void *key, void **val);

HASH_ITER hash_begin(HASH table);
bool hash_next(HASH_ITER iterator, void **key, void **val);
void hash_end(HASH_ITER *iterator);

void hash_free(HASH *table);

//
// Implementation
//

#ifdef HASH_IMPLEMENTATION

#include <stdlib.h>

// Types

typedef struct {
    uint32_t locked;
    uint32_t elts;
    uint32_t buckets;
    float thresh;
    HASH_eq_fn *eq;
    HASH_fn *hash;
    struct data {
        bool filled;
        void *key, *val;
        struct data *prev;
        struct data *next;
    } *data;
} Hash;

typedef struct {
    Hash *table;
    uint32_t index;
    struct data *d;
} Iter;

// Declarations

static HASH_eq_fn default_eq;
static HASH_fn default_hash;
static int rehash(Hash *table);
static int lookup(Hash *table, void *key, Iter *ret);
static void hash_free_impl(Hash *table);

// Definitions

HASH hash_init_default(void) {
    return hash_init(0, 0.0f, NULL, NULL);
}

HASH hash_init(uint32_t size_hint, float thresh, HASH_eq_fn *eq, HASH_fn *hash) {
    if (size_hint == 0)
        size_hint = 16;
    if (thresh <= 0.0f)
        thresh = 0.8f;
    if (!eq)
        eq = default_eq;
    if (!hash)
        hash = default_hash;

    Hash *table = malloc(sizeof(Hash));
    if (table) {
        table->locked = 0;
        table->elts = 0;
        table->buckets = (size_hint + 1) / thresh;
        table->thresh = thresh;
        table->eq = eq;
        table->hash = hash;
        if (!(table->data = calloc(table->buckets, sizeof (struct data)))) {
            free(table);
            return NULL;
        }
    }

    return (HASH) table;
}

uint32_t hash_size(HASH htable) {
    Hash *table = htable;

    return table->elts;
}

int hash_set(HASH htable, void *key, void *val) {
    Hash *table = htable;

    if (table->locked)
        return 1;

    Iter it;
    if (lookup(table, key, &it)) {
        float new_load = (float) (table->elts + 1) / table->buckets;
        if (new_load > table->thresh)
            if (rehash(table))
                return 1;

        struct data *prev = NULL;
        struct data *next = NULL;
        struct data *new = &(table->data[it.index]);
        if (new->filled) {
            struct data *old = new;
            new = malloc(sizeof (struct data));
            if (!new)
                return 1;
            prev = old;
            next = old->next;
            old->next = new;
            if (next)
                next->prev = new;
        }

        new->filled = 1;
        new->key = key;
        new->val = val;
        new->prev = prev;
        new->next = next;

        table->elts++;
    }
    else
        it.d->val = val;

    return 0;
}

int hash_get(HASH htable, void *key, void **val) {
    Hash *table = htable;

    Iter it;
    int ret = lookup(table, key, &it);

    if (!ret && val)
        *val = it.d->val;
    return ret;
}

HASH_ITER hash_begin(HASH htable) {
    Hash *table = htable;

    Iter *it = malloc(sizeof (Iter));
    if (it) {
        (table->locked)++;
        it->table = table;
        it->index = 0;
        it->d = table->data;
    }
    return it;
}

bool hash_next(HASH_ITER iterator, void **key, void **val) {
    Iter *it = iterator;

    while (it->index < it->table->buckets) {
        Iter old = *it;
        it->d = it->d->next ? it->d->next : it->table->data + ++(it->index);
        if (old.d->filled) {
            if (key)
                *key = old.d->key;
            if (val)
                *val = old.d->val;
            return 1;
        }
    }

    return 0;
}

void hash_end(HASH_ITER *iterator) {
    Iter *it = *iterator;
    (it->table->locked)--;
    free(it);
    *iterator = NULL;
}

void hash_free(HASH *ptable) {
    Hash *table = *ptable;
    hash_free_impl(table);
    free(table);
    *ptable = NULL;
}

bool default_eq(void *a, void *b) {
    return a == b;
}

uint32_t default_hash(void *obj, uint32_t maxhash) {
    return ((uintptr_t) obj) % maxhash;
}

int rehash(Hash *table) {
    HASH new = hash_init(table->elts * 2, table->thresh, table->eq, table->hash);
    if (!new) goto clean_0;

    HASH_ITER it = hash_begin((HASH) table);
    if (!it) goto clean_1;
    void *key, *val;
    while (hash_next(it, &key, &val))
        if (hash_set(new, key, val)) goto clean_2;
    hash_end(&it);

    Hash *new_table = new;
    hash_free_impl(table);
    *table = *new_table;
    free(new_table);

    return 0;

clean_2:
    hash_end(it);
clean_1:
    hash_free(&new);
clean_0:
    return 1;
}

int lookup(Hash *table, void *key, Iter *ret) {
    ret->table = table;
    ret->d = NULL;
    ret->index = table->hash(key, table->buckets) % table->buckets;
    struct data *d = &(table->data[ret->index]);
    while (!(d->filled) || !(table->eq(d->key, key))) {
        d = d->next;
        if (!d)
            return 1;
    }

    ret->d = d;
    return 0;
}

void hash_free_impl(Hash *table) {
    for (uint32_t i = 0; i < table->buckets; i++) {
        struct data *d = table->data[i].next;
        while (d) {
            struct data *next = d->next;
            free(d);
            d = next;
        }
    }
    free(table->data);
}

#endif
