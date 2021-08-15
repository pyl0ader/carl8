#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>

#include "logError.h"
#include "util.h"

#define H1(K) (K % UTIL_TABLESIZE)
#define H2(K) (1 + (K % (UTIL_TABLESIZE - 1)) )
#define HASH(K, I) ( ( H1(K) + i * H2(K) ) % UTIL_TABLESIZE)

struct util_pair {
	unsigned long numericKey;
	char* textKey;
	int item;
};

struct util_linkedPair {
	unsigned long numericKey;
	char* textKey; 
	util_linkedList list;
};

static unsigned long strToLong(const char* str){
	unsigned long ret = 0;

	const char* ptr = str + strlen(str);
	for(int i = 0; --ptr != str - 1; i++){
		ret += *ptr * pow(128, i);
	}

	return ret;
}

int util_createDictionary(util_dictionary* d){
	*d = malloc(sizeof(struct util_pair*) * UTIL_TABLESIZE);

	if(*d == NULL){
		setError("malloc: %s", strerror(errno) );
		return -1;
	}

	memset(*d, 0, sizeof(struct util_pair*) * UTIL_TABLESIZE);
	return 0;
}

int util_insert(util_dictionary d, const char* key, int val){
	int i = 0;
	int j = 0;
	unsigned long k = strToLong(key);


	do {
		j = HASH(k, i);

		if(d[j] == NULL) {
			d[j] = malloc( sizeof(struct util_pair) );
            d[j]->textKey = strdup(key);
			d[j]->numericKey = k;
			d[j]->item = val;

			return 0;
		}
		else if( d[j]->numericKey == k ) {
			setError("A value with this key already exist");
			return -1;
		}
		else {
			i++;
			continue;
		}

	} while( i < UTIL_TABLESIZE);
	
	setError("Hash table full");
	return -1;
}

int util_search(util_dictionary d, const char* key, int* val){
	int i = 0;
	int j = 0;
	unsigned long k = strToLong(key);

	do { 
		j = HASH(k, i);
		if(d[j] != NULL && d[j]->numericKey == k){
			*val = d[j]->item;
			return 1;
		}
		else i++;

	} while(d[j] != NULL && i < UTIL_TABLESIZE);

	return 0;
}

int util_delete(util_dictionary d, const char* key){
	int i = 0;
	unsigned long k = strToLong(key);

	struct util_pair* p;

	do { 
		int dictionaryIndex = HASH(k, i);

		p = d[dictionaryIndex];
		if(p != NULL && p->numericKey == k){
			free(p->textKey);
			free(p);
			d[dictionaryIndex] = NULL;
			return 0;
		}
		else i++;

	} while(p != NULL && i < UTIL_TABLESIZE);

	setError("no Linked List inserted at %s", key);
	return -1;
}

int util_createLinkedDictionary(util_linkedDictionary* d){
	*d = malloc(sizeof(struct util_linkedPair*) * UTIL_TABLESIZE);

	if(*d == NULL){
		setError("malloc: %s", strerror(errno) );
		return -1;
	}

	memset(*d, 0, sizeof(struct util_linkedpair*) * UTIL_TABLESIZE);
	return 0;
}

int util_linkedInsert(util_linkedDictionary d, const char* key, int val){
	int i = 0;
	int j = 0;
	unsigned long k = strToLong(key);

	do {
		j = HASH(k, i);

		if(d[j] == NULL) {
			d[j] = malloc( sizeof(struct util_linkedPair) );

			if(d[j] == NULL){
				setError("malloc: %s", strerror(errno) );
				return -1;
			}
			d[j]->numericKey = k;
			d[j]->textKey = strdup(key);
			d[j]->list.item = val;
			d[j]->list.next = NULL;

			return 0;
		}
		else if( d[j]->numericKey == k ) {
			util_linkedList* vals;

			for(vals = &d[j]->list; vals->next != NULL; vals = vals->next);
			vals = vals->next = malloc( sizeof(util_linkedList) );

			if(vals == NULL){
				setError("malloc: %s", strerror(errno) );
				return -1;
			}
			vals->item = val;
			vals->next = NULL;

			return 0;
		}
		else {
			i++;
		}

	} while( i < UTIL_TABLESIZE);
	
	setError("Hash table full");
	return -1;
}

static struct util_linkedPair DELETED = {};

int util_linkedSearch(util_linkedDictionary d, const char* key, util_linkedList** vals){
	int i = 0;
	int j = 0;
	unsigned long k = strToLong(key);

	do { 
		j = HASH(k, i);
		if(d[j] != NULL && d[j] != &DELETED && d[j]->numericKey == k){
			*vals = &d[j]->list;
			return 1;
		}
		else i++;

	} while(d[j] != NULL && i < UTIL_TABLESIZE);

	return 0;
}

int util_linkedDelete(util_linkedDictionary d, const char* key){
	int i = 0;
	int j = 0;
	unsigned long k = strToLong(key);

	do { 
		j = HASH(k, i);
		if(d[j] != NULL && d[j]->numericKey == k){
			util_linkedList* vals = d[j]->list.next;
			util_linkedList* next;

			while(vals != NULL) {
				next = vals->next;
				free(vals);
				vals = next;
			}

			free(d[j]->textKey);
			free(d[j]);
			d[j] = &DELETED;
			return 0;
		}
		else i++;

	} while(d[j] != NULL && i < UTIL_TABLESIZE);

	setError("no Linked List inserted at %s", key);
	return -1;
}
