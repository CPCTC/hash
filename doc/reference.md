# Reference Documentation

Macro `HASH_UNAME`:
---
```c
// undefined
```
Specifies the uppercase name of the hash type to be defined by the header. If this macro is not defined before including hash.h, then the header has no effect. This macro is undefined by the header, unless HASH_IMPLEMENTATION is defined or the header would have no other effect. Throughout the rest of this documentation, `<HASH>` refers to the contents of this macro, even within code blocks.

Macro `HASH_LNAME`:
---
```c
// undefined
```
Specifies the lowercase name of the hash type to be defined by the header. If this macro is not defined before including hash.h, then the header has no effect. This macro is undefined by the header, unless HASH_IMPLEMENTATION is defined or the header would have no other effect. Throughout the rest of this documentation, `<hash>` refers to the contents of this macro, even within code blocks.

Macro `HASH_KEY_TYPE`:
---
```c
// undefined
```
Specifies the type used for keys in the hash table to be defined by the header. If this macro is not defined before including hash.h, then the header has no effect. This macro is undefined by the header, unless HASH_IMPLEMENTATION is defined or the header would have no other effect. Throughout the rest of this documentation, `<HASH_KEY_TYPE>` refers to the contents of this macro, even within code blocks.

Macro `HASH_VAL_TYPE`:
---
```c
// undefined
```
Specifies the type used for values in the hash table to be defined by the header. If this macro is not defined before including hash.h, then the header has no effect. This macro is undefined by the header, unless HASH_IMPLEMENTATION is defined or the header would have no other effect. Throughout the rest of this documentation, `<HASH_VAL_TYPE>` refers to the contents of this macro, even within code blocks.

Macro `HASH_IMPLEMENTATION`:
---
```c
// undefined
```
If this macro is defined before including hash.h, then the library's functions are defined by the header.

Function `<HASH>`:
---
```c
typedef void *<HASH>;
```
Opaque type. Used as a handle to an initialized hash table.

Function `<HASH>_ITER`:
---
```c
typedef void *<HASH>_ITER;
```
Opaque type. Used as a handle to an iterator over an initialized hash table.

Function `<hash>_init_default`:
---
```c
<HASH> <hash>_init_default(void);
```
Equivalent to `<hash>_init(0, 0.0f)`.

Function `<hash>_init`:
---
```c
<HASH> <hash>_init(uint32_t size, float thresh);
```
Allocates a new hash table. Returns NULL on failure.
 - *size* is the number of elements you expect to store. If this is 0, then the default value is used.
 - *thresh* is the the load factor rehash threshold. If this is 0.0f, then the default value is used.

Function `<hash>_size`:
---
```c
uint32_t <hash>_size(<HASH> table);
```
Returns the number of elements currently stored in the hash table *table*.

Function `<hash>_set`:
---
```c
int <hash>_set(<HASH> table, <HASH_KEY_TYPE> key, <HASH_VAL_TYPE> val);
```
Add an element with key *key* and value *val* to the hash table *table*. Or, if *table* already contains an element with key *key*, instead set the element's value to *val*. Will fail if *table* has any active iterators, or a rehash was triggered but did not succeed. Returns 1 on failure, and 0 otherwise.

Function `<hash>_unset`:
---
```c
int <hash>_unset(<HASH> table, <HASH_KEY_TYPE> key);
```
Remove the element with key *key* from *table*, if it exists. Will fail if *table* has any active iterators. Returns 1 on failure, and 0 otherwise.

Function `<hash>_get`:
---
```c
int <hash>_get(<HASH> table, <HASH_KEY_TYPE> key, <HASH_VAL_TYPE> *val);
```
If the hash table *table* contains an element with key *key*, then returns the element's value in `*val` and return 0. Otherwise, return 1.

Function `<hash>_begin`:
---
```c
<HASH>_ITER <hash>_begin(<HASH> table);
```
Allocate and return a new iterator, pointing to the first element of the hash table *table*. Returns NULL on failure.

Function `<hash>_next`:
---
```c
bool <hash>_next(<HASH>_ITER iterator, <HASH_KEY_TYPE> *key, <HASH_VAL_TYPE> *val);
```
Iterate over elements in a hash table. If there are elements left, then place the next element in `*key` and `*value`, advance the iterator, and return true. Otherwise, return false.

Function `<hash>_end`:
---
```c
void <hash>_end(<HASH>_ITER *iterator);
```
Deallocate an iterator.

Function `<hash>_free`:
---
```c
void <hash>_free(<HASH> *table);
```
Deallocate a hash table.

Function `hash_eq`:
---
```c
static bool hash_eq(<HASH_KEY_TYPE> a, <HASH_KEY_TYPE> b);
// undefined
```
User-defined function. Must be defined in the same translation unit that included hash.h with HASH_IMPLEMENTATION. Should return true if *a* is equivalent to *b*, and false otherwise.

Function `hash_fn`:
---
```c
static uint32_t hash_fn(<HASH_KEY_TYPE> obj, uint32_t maxhash);
// undefined
```
User-defined function. Must be defined in the same translation unit that included hash.h with HASH_IMPLEMENTATION. Should return a hash of *obj*, in the range [0, maxhash).
