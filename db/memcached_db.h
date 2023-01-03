//
//  memcached_db.h
//  YCSB-C
//

#ifndef YCSB_C_MEMCACHED_DB_H_
#define YCSB_C_MEMCACHED_DB_H_

#include "core/db.h"

#include <iostream>
#include <string>
#include "core/properties.h"
#include <libmemcached/memcached.h>

using std::cout;
using std::endl;

namespace ycsbc {

class MemcachedDB : public DB {
 public:
  MemcachedDB(const char * host) {
    memcached_return rc;
    
    memc_ = memcached_create(NULL);
    
    servers_ = NULL;
    servers_ = memcached_server_list_append(servers_, host, 11211, &rc);
    rc = memcached_server_push(memc_, servers_);
    if (rc == MEMCACHED_SUCCESS) {
      // cout << pthread_self() << " added server successfully" << endl;
    } else {
      cout << "Couldn't add server: " << memcached_strerror(memc_, rc) << endl;
    }
  }

  int Read(const std::string &table, const std::string &key,
           const std::vector<std::string> *fields,
           std::vector<KVPair> &result);

  int Scan(const std::string &table, const std::string &key,
           int len, const std::vector<std::string> *fields,
           std::vector<std::vector<KVPair>> &result) {
    throw "Scan: function not implemented!";
  }

  int Update(const std::string &table, const std::string &key,
             std::vector<KVPair> &values);

  int Insert(const std::string &table, const std::string &key,
             std::vector<KVPair> &values) {
    return Update(table, key, values);
  }

  int Delete(const std::string &table, const std::string &key);

  memcached_server_st * servers_;
  memcached_st * memc_;
};

} // ycsbc

#endif // YCSB_C_MEMCACHED_DB_H_

