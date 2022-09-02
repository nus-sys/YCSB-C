//
//  memcached_db.cc
//  YCSB-C
//

#include "db/memcached_db.h"

#include <cstring>

using namespace std;

namespace ycsbc {

int MemcachedDB::Read(const std::string &table, const std::string &key,
           const std::vector<std::string> *fields,
           std::vector<KVPair> &result) {
  memcached_return rc;
  char * retrieved_value;
  size_t value_length;
  uint32_t flags;

  retrieved_value = memcached_get(memc_, key.c_str(), key.length(), &value_length, &flags, &rc);

  cout << pthread_self() << " read key: " << key << ", value: " << retrieved_value << endl;

  if (rc == MEMCACHED_SUCCESS) {
    result.push_back(make_pair(key, std::string(retrieved_value)));
    free(retrieved_value);
  }

  return DB::kOK;
}

int MemcachedDB::Update(const string &table, const string &key,
           vector<KVPair> &values) {
  memcached_return rc;

  for (KVPair &p : values) {
    rc = memcached_set(memc_, key.c_str(), key.length(), p.second.c_str(), p.second.length(), (time_t)0, (uint32_t)0);
  }
  return DB::kOK;
}

int MemcachedDB::Delete(const std::string &table, const std::string &key) {
  memcached_return rc;
  rc = memcached_delete(memc_, key.c_str(), key.length(), (time_t)0);
  return DB::kOK;
}

} // ycsbc
