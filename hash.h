// Copyright (c) 2022 Seth Galasso
// SPDX-License-Identifier: MIT

#ifdef HASH_UNAME
#ifdef HASH_LNAME
#ifdef HASH_KEY_TYPE
#ifdef HASH_VAL_TYPE

//
// Header
//

#define HASH_CONC_IMPL(a,b) a ## b
#define HASH_CONC(a,b) HASH_CONC_IMPL(a,b)

#include <stdbool.h>
#include <stdint.h>

// Types

#define HASH HASH_UNAME
typedef void *HASH;
#define HASH_ITER HASH_CONC(HASH_UNAME,_ITER)
typedef void *HASH_ITER;

// Interface

#define hash_init_default HASH_CONC(HASH_LNAME,_init_default)
HASH hash_init_default(void);
#define hash_init HASH_CONC(HASH_LNAME,_init)
HASH hash_init(uint32_t size, float thresh);

#define hash_size HASH_CONC(HASH_LNAME,_size)
uint32_t hash_size(HASH table);
#define hash_set HASH_CONC(HASH_LNAME,_set)
int hash_set(HASH table, HASH_KEY_TYPE key, HASH_VAL_TYPE val);
#define hash_unset HASH_CONC(HASH_LNAME,_unset)
int hash_unset(HASH table, HASH_KEY_TYPE key);
#define hash_get HASH_CONC(HASH_LNAME,_get)
int hash_get(HASH table, HASH_KEY_TYPE key, HASH_VAL_TYPE *val);

#define hash_begin HASH_CONC(HASH_LNAME,_begin)
HASH_ITER hash_begin(HASH table);
#define hash_next HASH_CONC(HASH_LNAME,_next)
bool hash_next(HASH_ITER iterator, HASH_KEY_TYPE *key, HASH_VAL_TYPE *val);
#define hash_end HASH_CONC(HASH_LNAME,_end)
void hash_end(HASH_ITER *iterator);

#define hash_free HASH_CONC(HASH_LNAME,_free)
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
    struct data {
        bool filled;
        HASH_KEY_TYPE key;
        HASH_VAL_TYPE val;
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

static bool hash_eq(HASH_KEY_TYPE a, HASH_KEY_TYPE b);
static uint32_t hash_fn(HASH_KEY_TYPE obj, uint32_t maxhash);
static int rehash(Hash *table);
static int lookup(Hash *table, HASH_KEY_TYPE key, Iter *ret);
static void hash_free_impl(Hash *table);

// Definitions

HASH hash_init_default(void) {
    return hash_init(0, 0.0f);
}

HASH hash_init(uint32_t size_hint, float thresh) {
    if (size_hint == 0)
        size_hint = 16;
    if (thresh <= 0.0f)
        thresh = 0.8f;

    Hash *table = malloc(sizeof(Hash));
    if (!table) goto err_0;
    table->locked = 0;
    table->elts = 0;
    table->buckets = (size_hint + 1) / thresh;
    if (!table->buckets) goto err_1;
    table->thresh = thresh;
    if (!(table->data = calloc(table->buckets, sizeof (struct data))))
        goto err_2;

    return (HASH) table;

err_2:
err_1:
    free(table);
err_0:
    return NULL;
}

uint32_t hash_size(HASH htable) {
    Hash *table = htable;

    return table->elts;
}

int hash_set(HASH htable, HASH_KEY_TYPE key, HASH_VAL_TYPE val) {
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

int hash_unset(HASH htable, HASH_KEY_TYPE key) {
    Hash *table = htable;

    if (table->locked)
        return 1;

    Iter it;
    if (lookup(table, key, &it))
        return 0;
    else if (it.d->prev) {
        it.d->prev->next = it.d->next;
        if (it.d->next)
            it.d->next->prev = it.d->prev;
        free(it.d);
    }
    else
        it.d->filled = 0;
    return 0;
}

int hash_get(HASH htable, HASH_KEY_TYPE key, HASH_VAL_TYPE *val) {
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

bool hash_next(HASH_ITER iterator, HASH_KEY_TYPE *key, HASH_VAL_TYPE *val) {
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

int rehash(Hash *table) {
    HASH new = hash_init(table->elts * 2, table->thresh);
    if (!new) goto clean_0;

    HASH_ITER it = hash_begin((HASH) table);
    if (!it) goto clean_1;
    HASH_KEY_TYPE key;
    HASH_VAL_TYPE val;
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

int lookup(Hash *table, HASH_KEY_TYPE key, Iter *ret) {
    ret->table = table;
    ret->d = NULL;
    ret->index = hash_fn(key, table->buckets) % table->buckets;
    struct data *d = &(table->data[ret->index]);
    while (!(d->filled) || !(hash_eq(d->key, key))) {
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

#endif // #ifdef HASH_IMPLEMENTATION

#undef hash_free
#undef hash_end
#undef hash_next
#undef hash_begin
#undef hash_get
#undef hash_unset
#undef hash_set
#undef hash_size
#undef hash_init
#undef hash_init_default
#undef hash_ITER
#undef HASH
#undef HASH_CONC
#undef HASH_CONC_IMPL

#ifndef HASH_IMPLEMENTATION
#undef HASH_VAL_TYPE
#undef HASH_KEY_TYPE
#undef HASH_LNAME
#undef HASH_UNAME
#endif

#endif // #ifdef HASH_VAL_TYPE
#endif // #ifdef HASH_KEY_TYPE
#endif // #ifdef HASH_LNAME
#endif // #ifdef HASH_UNAME
