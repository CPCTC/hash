// Copyright (c) 2022 Seth Galasso
// SPDX-License-Identifier: MIT

#ifdef HASH_NAME
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

#define HASH HASH_NAME
typedef void *HASH;
#define HASH_ITER HASH_CONC(HASH_NAME,_ITER)
typedef void *HASH_ITER;

// Interface

#define HASH_init_default HASH_CONC(HASH_NAME,_init_default)
HASH HASH_init_default(void);
#define HASH_init HASH_CONC(HASH_NAME,_init)
HASH HASH_init(uint32_t size, float thresh);

#define HASH_size HASH_CONC(HASH_NAME,_size)
uint32_t HASH_size(HASH table);
#define HASH_set HASH_CONC(HASH_NAME,_set)
int HASH_set(HASH table, HASH_KEY_TYPE key, HASH_VAL_TYPE val);
#define HASH_unset HASH_CONC(HASH_NAME,_unset)
int HASH_unset(HASH table, HASH_KEY_TYPE key);
#define HASH_get HASH_CONC(HASH_NAME,_get)
int HASH_get(HASH table, HASH_KEY_TYPE key, HASH_VAL_TYPE *val);

#define HASH_begin HASH_CONC(HASH_NAME,_begin)
HASH_ITER HASH_begin(HASH table);
#define HASH_next HASH_CONC(HASH_NAME,_next)
bool HASH_next(HASH_ITER iterator, HASH_KEY_TYPE *key, HASH_VAL_TYPE *val);
#define HASH_end HASH_CONC(HASH_NAME,_end)
void HASH_end(HASH_ITER *iterator);

#define HASH_free HASH_CONC(HASH_NAME,_free)
void HASH_free(HASH *table);

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

HASH HASH_init_default(void) {
    return HASH_init(0, 0.0f);
}

HASH HASH_init(uint32_t size_hint, float thresh) {
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

uint32_t HASH_size(HASH htable) {
    Hash *table = htable;

    return table->elts;
}

int HASH_set(HASH htable, HASH_KEY_TYPE key, HASH_VAL_TYPE val) {
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

int HASH_unset(HASH htable, HASH_KEY_TYPE key) {
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

int HASH_get(HASH htable, HASH_KEY_TYPE key, HASH_VAL_TYPE *val) {
    Hash *table = htable;

    Iter it;
    int ret = lookup(table, key, &it);

    if (!ret && val)
        *val = it.d->val;
    return ret;
}

HASH_ITER HASH_begin(HASH htable) {
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

bool HASH_next(HASH_ITER iterator, HASH_KEY_TYPE *key, HASH_VAL_TYPE *val) {
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

void HASH_end(HASH_ITER *iterator) {
    Iter *it = *iterator;
    (it->table->locked)--;
    free(it);
    *iterator = NULL;
}

void HASH_free(HASH *ptable) {
    Hash *table = *ptable;
    hash_free_impl(table);
    free(table);
    *ptable = NULL;
}

int rehash(Hash *table) {
    HASH new = HASH_init(table->elts * 2, table->thresh);
    if (!new) goto clean_0;

    HASH_ITER it = HASH_begin((HASH) table);
    if (!it) goto clean_1;
    HASH_KEY_TYPE key;
    HASH_VAL_TYPE val;
    while (HASH_next(it, &key, &val))
        if (HASH_set(new, key, val)) goto clean_2;
    HASH_end(&it);

    Hash *new_table = new;
    hash_free_impl(table);
    *table = *new_table;
    free(new_table);

    return 0;

clean_2:
    HASH_end(it);
clean_1:
    HASH_free(&new);
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

#undef HASH_free
#undef HASH_end
#undef HASH_next
#undef HASH_begin
#undef HASH_get
#undef HASH_unset
#undef HASH_set
#undef HASH_size
#undef HASH_init
#undef HASH_init_default
#undef HASH_ITER
#undef HASH
#undef HASH_CONC
#undef HASH_CONC_IMPL

#undef HASH_VAL_TYPE
#undef HASH_KEY_TYPE
#undef HASH_NAME

#endif // #ifdef HASH_VAL_TYPE
#endif // #ifdef HASH_KEY_TYPE
#endif // #ifdef HASH_NAME
