//
//  ycsbc.cc
//  YCSB-C
//
//  Created by Jinglei Ren on 12/19/14.
//  Copyright (c) 2014 Jinglei Ren <jinglei@ren.systems>.
//

#include <cstring>
#include <string>
#include <iostream>
#include <vector>
#include <future>
#include "core/utils.h"
#include "core/timer.h"
#include "core/client.h"
#include "core/core_workload.h"
#include "db/db_factory.h"
#ifdef CYGNUS
extern "C"
{
#include <pthread.h>
#include "cygnus.h"
#include "pthl/pthl.h"
}
#endif

using namespace std;

void UsageMessage(const char *command);
bool StrStartWith(const char *str, const char *pre);
string ParseCommandLine(int argc, char *argv[], utils::Properties &props);

struct ClientArgs {
  ycsbc::DB *db;
  ycsbc::CoreWorkload *wl;
  int num_ops;
  bool is_loading;
  int oks;
};

void * DelegateClient(void * args) {
  ycsbc::DB *db = ((struct ClientArgs *)args)->db;
  ycsbc::CoreWorkload *wl = ((struct ClientArgs *)args)->wl;
  int num_ops = ((struct ClientArgs *)args)->num_ops;
  bool is_loading = ((struct ClientArgs *)args)->is_loading;

  int oks = 0;

  db->Init();
  ycsbc::Client client(*db, *wl);
  for (int i = 0; i < num_ops; ++i) {
    if (is_loading) {
      oks += client.DoInsert();
    } else {
      oks += client.DoTransaction();
    }
  }
  db->Close();

  ((struct ClientArgs *)args)->oks = oks;
  return NULL;
}

int main(int argc, char * argv[]) {
#ifdef CYGNUS
  cygnus_start();
#endif

  for (int i = 0; i < argc; i++) {
    cout << argv[i] << endl;
  }
  utils::Properties props;
  string file_name = ParseCommandLine(argc, argv, props);

  ycsbc::DB *db = ycsbc::DBFactory::CreateDB(props);
  if (!db) {
    cout << "Unknown database name " << props["dbname"] << endl;
    exit(0);
  }

  ycsbc::CoreWorkload wl;
  wl.Init(props);

  const int num_threads = stoi(props.GetProperty("threadcount", "1"));

  // Loads data
  // vector<future<int>> actual_ops;
  int total_ops = stoi(props[ycsbc::CoreWorkload::RECORD_COUNT_PROPERTY]);
  pthread_t pids[10];
  struct ClientArgs args[10];
  int err;
  for (int i = 0; i < num_threads; ++i) {
    // actual_ops.emplace_back(async(launch::async,
    //     DelegateClient, db, &wl, total_ops / num_threads, true));
    struct ClientArgs * arg = &args[i];
    arg->db = db;
    arg->wl = &wl;
    arg->num_ops = total_ops / num_threads;
    arg->is_loading = true;
    arg->oks = 0;
    err = pthread_create(&pids[i], NULL, DelegateClient, (void *)arg);
      if (err) {
        perror(" pthread_create consumer1 failed(2)!");
      }
  }
  // assert((int)actual_ops.size() == num_threads);

  for (int i = 0; i < num_threads; ++i) {
    pthread_join(pids[i], NULL);
  }

  int sum = 0;
  // for (auto &n : actual_ops) {
  //   assert(n.valid());
  //   sum += n.get();
  // }
  for (int i = 0; i < num_threads; ++i) {
    sum += args[i].oks;
  }
  cerr << "# Loading records:\t" << sum << endl;

  // Peforms transactions
  // actual_ops.clear();
  total_ops = stoi(props[ycsbc::CoreWorkload::OPERATION_COUNT_PROPERTY]);
  utils::Timer<double> timer;
  timer.Start();
  for (int i = 0; i < num_threads; ++i) {
    // actual_ops.emplace_back(async(launch::async,
    //     DelegateClient, db, &wl, total_ops / num_threads, false));
    struct ClientArgs * arg = &args[i];
    arg->db = db;
    arg->wl = &wl;
    arg->num_ops = total_ops / num_threads;
    arg->is_loading = false;
    arg->oks = 0;
    err = pthread_create(&pids[i], NULL, DelegateClient, (void *)arg);
      if (err) {
        perror(" pthread_create consumer1 failed(2)!");
      }
  }

  for (int i = 0; i < num_threads; ++i) {
    pthread_join(pids[i], NULL);
  }

  // assert((int)actual_ops.size() == num_threads);

  sum = 0;
  // for (auto &n : actual_ops) {
  //   assert(n.valid());
  //   sum += n.get();
  // }
  for (int i = 0; i < num_threads; ++i) {
    sum += args[i].oks;
  }
  double duration = timer.End();
  cerr << "# Transaction throughput (KTPS)" << endl;
  cerr << props["dbname"] << '\t' << file_name << '\t' << num_threads << '\t';
  cerr << total_ops / duration / 1000 << endl;


#ifdef CYGNUS
  cygnus_terminate();
#endif
  return 0;
}

string ParseCommandLine(int argc, char *argv[], utils::Properties &props) {
  int argindex = 1;
  string filename;
  while (argindex < argc && StrStartWith(argv[argindex], "-")) {
    if (strcmp(argv[argindex], "-threads") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("threadcount", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-db") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("dbname", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-host") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("host", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-port") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("port", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-slaves") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("slaves", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-P") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      filename.assign(argv[argindex]);
#if 0
      ifstream input(argv[argindex]);
      try {
        props.Load(input);
      } catch (const string &message) {
        cout << message << endl;
        exit(0);
      }
      input.close();
#endif
      props.Load(argv[argindex]);
      argindex++;
    } else {
      cout << "Unknown option '" << argv[argindex] << "'" << endl;
      exit(0);
    }
  }

  if (argindex == 1 || argindex != argc) {
    UsageMessage(argv[0]);
    exit(0);
  }

  return filename;
}

void UsageMessage(const char *command) {
  cout << "Usage: " << command << " [options]" << endl;
  cout << "Options:" << endl;
  cout << "  -threads n: execute using n threads (default: 1)" << endl;
  cout << "  -db dbname: specify the name of the DB to use (default: basic)" << endl;
  cout << "  -P propertyfile: load properties from the given file. Multiple files can" << endl;
  cout << "                   be specified, and will be processed in the order specified" << endl;
}

inline bool StrStartWith(const char *str, const char *pre) {
  return strncmp(str, pre, strlen(pre)) == 0;
}

