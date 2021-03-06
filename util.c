#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>

#include "logError.h"
#include "util.h"

#define M 239

#define H1(K) (K % M)
#define H2(K) (1 + (K % (M - 1)) )
#define HASH(K, I) ( ( H1(K) + i * H2(K) ) % M )

struct util_pair {
	unsigned long key;
	int val;
};

struct util_linkedPair {
	unsigned long key;
	util_linkedList vals;
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
	*d = malloc(sizeof(struct util_pair*) * M);

	if(*d == NULL){
		setError("malloc: %s", strerror(errno) );
		return -1;
	}

	memset(*d, 0, sizeof(struct util_pair*) * M);
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
			d[j]->key = k;
			d[j]->val = val;

			return 0;
		}
		else if( d[j]->key == k ) {
			setError("A value with this key already exist");
			return -1;
		}
		else {
			i++;
			continue;
		}

	} while( i < M);
	
	setError("Hash table full");
	return -1;
}
   
int util_search(util_dictionary d, const char* key, int* val){
	int i = 0;
	int j = 0;
	unsigned long k = strToLong(key);

	do { 
		j = HASH(k, i);
		if(d[j] != NULL && d[j]->key == k){
			*val = d[j]->val;
			return 1;
		}
		else i++;

	} while(d[j] != NULL && i < M);

	return 0;
}

int util_delete(util_dictionary d, const char* key){
	int i = 0;
	unsigned long k = strToLong(key);

	struct util_pair* p;

	do { 
		int dictionaryIndex = HASH(k, i);

		p = d[dictionaryIndex];
		if(p != NULL && p->key == k){
			free(p);
			d[dictionaryIndex] = NULL;
			return 0;
		}
		else i++;

	} while(p != NULL && i < M);

	setError("no Linked List inserted at %s", key);
	return -1;
}

int util_createLinkedDictionary(util_linkedDictionary* d){
	*d = malloc(sizeof(struct util_linkedPair*) * M);

	if(*d == NULL){
		setError("malloc: %s", strerror(errno) );
		return -1;
	}

	memset(*d, 0, sizeof(struct util_linkedpair*) * M);
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
			d[j]->key = k;
			d[j]->vals.val = val;
			d[j]->vals.next = NULL;

			return 0;
		}
		else if( d[j]->key == k ) {
			util_linkedList* vals;

			for(vals = &d[j]->vals; vals->next != NULL; vals = vals->next);
			vals = vals->next = malloc( sizeof(util_linkedList) );

			if(vals == NULL){
				setError("malloc: %s", strerror(errno) );
				return -1;
			}
			vals->val = val;
			vals->next = NULL;

			return 0;
		}
		else {
			i++;
		}

	} while( i < M);
	
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
		if(d[j] != NULL && d[j] != &DELETED && d[j]->key == k){
			*vals = &d[j]->vals;
			return 1;
		}
		else i++;

	} while(d[j] != NULL && i < M);

	return 0;
}

int util_linkedDelete(util_linkedDictionary d, const char* key){
	int i = 0;
	int j = 0;
	unsigned long k = strToLong(key);

	do { 
		j = HASH(k, i);
		if(d[j] != NULL && d[j]->key == k){
			util_linkedList* vals = d[j]->vals.next;
			util_linkedList* next;

			while(vals != NULL) {
				next = vals->next;
				free(vals);
				vals = next;
			}

			free(d[j]);
			d[j] = &DELETED;
			return 0;
		}
		else i++;

	} while(d[j] != NULL && i < M);

	setError("no Linked List inserted at %s", key);
	return -1;
}
