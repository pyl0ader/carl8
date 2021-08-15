#define UTIL_TABLESIZE 239

struct util_linkedPair;
struct util_pair;
struct util_linkedList;

typedef struct util_linkedPair** util_linkedDictionary;
typedef struct util_pair** util_dictionary;

typedef struct util_linkedList util_linkedList;

struct util_linkedList {
    int item;
	util_linkedList* next;
};

/* A _util_dictionary_ is created where keys may be mapped to values.
 * Return value is -1 if errors occur, otherwise it's zero. */
extern int util_createDictionary(util_dictionary* d);

/* _val_ is inserted at _key_ in _d_.
 * Return value is -1 if errors occur, otherwise it's zero. */
extern int util_insert(util_dictionary d, const char* key, int val);

/* _val_ is set to the value at _key_ is _d_.
 * Return value is -1 if errors occur, 0 if no value is inserted,
 * otherwise it's 1. */
extern int util_search(util_dictionary d, const char* key, int* val);

/* The the value at _key_ in _d_ is erased. 
 * Return value is -1 if errors occur, otherwise it's zero */
extern int util_delete(util_dictionary d, const char* key);

/* A _util_linkedDictionary_ is created where keys may be mapped to a Linked List. 
 * Return value is -1 if errors occur, otherwise it's zero. */
extern int util_createLinkedDictionary(util_linkedDictionary* d);

/* _val_ is added to a _util_linkedList_ at _key_ in _d_ 
 * Return value is -1 if errors occur, otherwise it's zero. */
extern int util_linkedInsert(util_linkedDictionary d, const char* key, int val);

/* The _util_linkedList_ pointer pointed by _vals_ is set to point to that inserted at _key_ in _d_ .
 * If there is no _util_linkedList_ iserted, or there are errors, _vals_ is set to _NULL_ .
 * Return value is -1 if errors occur, 0 if no linked list is inserted, 
 * otherwise it's 1. */
extern int util_linkedSearch(util_linkedDictionary d, const char* key, util_linkedList** vals);

/* the _linkedList_ inserted at _key_ in _d_ is erased.
 * Return value is -1 if errors occur, otherwise it's zero. */
extern int util_linkedDelete(util_linkedDictionary d, const char* key);
