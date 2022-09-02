//
//  memcached_db.cc
//  YCSB-C
//

#include "db/memcached_db.h"

#include <cstring>

using namespace std;

int MemcachedDB::Read(const std::string &table, const std::string &key,
           const std::vector<std::string> *fields,
           std::vector<KVPair> &result) {
  memcached_return rc;
  char * retrieved_value;
  size_t value_length;
  uint32_t flags;

  retrieved_value = memcached_get(memc, key, strlen(key), &value_length, &flags, &rc);
  printf("Yay!\n");

  if (rc == MEMCACHED_SUCCESS) {
    cout << "Key retrieved successfully" << endl;
    cout << "The key " << key << " returned value " << retrieved_value << endl;
    free(retrieved_value);
  } else {
    cout << "Couldn't retrieve key: " << memcached_strerror(memc, rc) << endl;
  }
}

int MemcachedDB::Update(const string &table, const string &key,
           vector<KVPair> &values) {
  memcached_return rc;

  for (KVPair &p : values) {
    rc = memcached_set(memc, key, strlen(key), value, strlen(value), (time_t)0, (uint32_t)0);
    if (rc == MEMCACHED_SUCCESS) {
      cout << "Key stored successfully" << endl;
    } else {
      cout << "Couldn't update key: " << memcached_strerror(memc, rc) << endl;
    }
  }

  int MemcachedDB::Delete(const std::string &table, const std::string &key) {
    memcached_return rc;
    rc = memcached_delete(memc, key, strlen(key), (time_t)0);
    if (rc == MEMCACHED_SUCCESS) {
      cout << "Key deleted successfully" << endl;
    } else {
      cout << "Couldn't delete key: " << memcached_strerror(memc, rc) << endl;
    }
  }

}