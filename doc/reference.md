# Reference Documentation

Type `HASH`:
---
```c
typedef void *HASH;
```
Opaque type. Used as a handle to an initialized hash table.

Type `HASH_ITER`:
---
```c
typedef void *HASH_ITER;
```
Opaque type. Used as a handle to an iterator over an initialized hash table.

Type `HASH_eq_fn`:
---
```c
typedef bool HASH_eq_fn(void *a, void *b);
```
Callback type. Used by *hash_init* as the equality function when constructing a hash table. Should return true if *a* is equivalent to *b*, and false otherwise.

Type `HASH_fn`:
---
```c
typedef uint32_t HASH_fn(void *obj, uint32_t maxhash);
```
Callback type. Used by *hash_init* as the hash function when constructing a hash table. Should return a hash of *obj*, in the range [0, maxhash).

Function `hash_init_default`:
---
```c
HASH hash_init_default(void);
```
Equivalent to `hash_init(0, 0.0f, NULL, NULL)`.

Function `hash_init`:
---
```c
HASH hash_init(uint32_t size, float thresh, HASH_eq_fn *eq, HASH_fn *hash);
```
Allocates a new hash table. Returns NULL on failure.
 - *size* is the number of elements you expect to store. If this is 0, then the default value is used.
 - *thresh* is the the load factor rehash threshold. If this is 0.0f, then the default value is used.
 - *eq* is the equality function for objects stored in the hash table. If this is NULL, then objects *a* and *b* are considered equal if and only if `&a == &b`.
 - *hash* is the hash function used by the hash table. If this is NULL, then `&obj` is used as a hash of a given object *obj*.

Function `hash_size`:
---
```c
uint32_t hash_size(HASH table);
```
Returns the number of elements currently stored in the hash table *table*.

Function `hash_set`:
---
```c
int hash_set(HASH table, void *key, void *val);
```
Add an element with key *key* and value *val* to the hash table *table*. Or, if *table* already contains an element with key *key*, instead set the element's value to *val*. Will fail if *table* has any active iterators, or a rehash was triggered but did not succeed. Returns 1 on failure, and 0 otherwise.

Function `hash_get`:
---
```c
int hash_get(HASH table, void *key, void **val);
```
If the hash table *table* contains an element with key *key*, then returns the element's value in `*val` and return 0. Otherwise, return 1.

Function `hash_begin`:
---
```c
HASH_ITER hash_begin(HASH table);
```
Allocate and return a new iterator, pointing to the first element of the hash table *table*. Returns NULL on failure.

Function `hash_next`:
---
```c
bool hash_next(HASH_ITER iterator, void **key, void **val);
```
Iterate over elements in a hash table. If there are elements left, then place the next element in `*key` and `*value`, advance the iterator, and return true. Otherwise, return false.

Function `hash_end`:
---
```c
void hash_end(HASH_ITER *iterator);
```
Deallocate an iterator.

Function `hash_free`:
---
```c
void hash_free(HASH *table);
```
Deallocate a hash table.

Macro `HASH_IMPLEMENTATION`:
---
```c
// undefined
```
If this macro is defined before including hash.h, then the library's functions are defined by the header.
