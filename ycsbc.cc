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

extern "C"
{
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
}

using namespace std;

void UsageMessage(const char *command);
bool StrStartWith(const char *str, const char *pre);
string ParseCommandLine(int argc, char *argv[], utils::Properties &props);

struct ClientArgs {
  // ycsbc::DB *db;
  // ycsbc::CoreWorkload *wl;
  int core;
  int num_record;
  int num_ops;
  // bool is_loading;
  int oks;
  struct timeval start;
  struct timeval end;
} __attribute__((aligned(64)));

utils::Properties props;
pthread_barrier_t barrier;

void * ClientMain(void * arg) {
  struct ClientArgs * args = (struct ClientArgs *)arg;
  ycsbc::DB *db = ycsbc::DBFactory::CreateDB(props);
  if (!db) {
    cout << "Unknown database name " << props["dbname"] << endl;
    exit(0);
  }

  ycsbc::CoreWorkload wl;
  wl.Init(props);

  int oks = 0;

  db->Init();
  ycsbc::Client client(*db, wl);

  for (int i = 0; i < args->num_record; ++i) {
    oks += client.DoInsert();
  }

  cerr << "# Loading records:\t" << oks << endl;

  pthread_barrier_wait(&barrier);

  oks = 0;

  /* Actual transaction */

  gettimeofday(&args->start, NULL);
  for (int i = 0; i < args->num_ops; ++i) {
    oks += client.DoTransaction();
  }
  gettimeofday(&args->end, NULL);

  args->oks = oks;

  db->Close();

  return NULL;
}

void * DelegateClient(void * arg) {
  struct ClientArgs * args = (struct ClientArgs *)arg;
  int err;
  cpu_set_t cpu;
  pthread_t pids[64];
  struct ClientArgs pargs[64];
  pthread_attr_t pattr;
  const int num_threads = stoi(props.GetProperty("threadcount", "1"));

  err = pthread_attr_init(&pattr);

  /* Binding all child threads to the same core */
  CPU_ZERO(&cpu);
  CPU_SET(sched_getcpu(), &cpu);
  err = pthread_attr_setaffinity_np(&pattr, sizeof(cpu_set_t), &cpu);

  for (int i = 0; i < num_threads; ++i) {
    pargs[i].num_record = args->num_record / num_threads;
    pargs[i].num_ops = args->num_ops / num_threads;
    pargs[i].oks = 0;
    err = pthread_create(&pids[i], &pattr, ClientMain, &pargs[i]);
      if (err) {
        printf(" [%s on core %d] pthread_create return %d!\n", __func__, sched_getcpu(), err);
      } else {
        fprintf(stderr, " Created new worker thread %lu on core %d!\n", pids[i], sched_getcpu());
      }
  }

  for (int i = 0; i < num_threads; ++i) {
    pthread_join(pids[i], NULL);
  }

  for (int i = 0; i < num_threads; ++i) {
    args->oks += pargs[i].oks;
  }

  struct timeval begin, done;
  begin = pargs[0].start;
  done = pargs[0].end;
  for (int i = 1; i < num_threads; i++) {
    if (!timercmp(&begin, &pargs[i].start, <=)) {
      begin = pargs[i].start;
    }
    if (!timercmp(&done, &pargs[i].end, >=)) {
      done = pargs[i].end;
    }
  }

  args->start = begin;
  args->end = done;

  return NULL;
}

int main(int argc, char * argv[]) {
  int err;
  cpu_set_t cpu;

  for (int i = 0; i < argc; i++) {
    cout << argv[i] << endl;
  }

  string file_name = ParseCommandLine(argc, argv, props);

  const int num_cores = stoi(props.GetProperty("corecount", "1"));
  const int num_threads = stoi(props.GetProperty("threadcount", "1"));

  int total_record = stoi(props.GetProperty("recordcount", "1000"));
  int total_ops = stoi(props.GetProperty("operationcount", "1000"));

  CPU_ZERO(&cpu);
  CPU_SET(4, &cpu);
  err = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpu);

  pthread_barrier_init(&barrier, NULL, num_threads * num_cores);

  pthread_t pids[64];
  pthread_attr_t pattr;
  struct ClientArgs args[64];
  fprintf(stderr, " Main thread running on core %d\n", sched_getcpu());
  for (int i = 0; i < num_cores; ++i) {
    // actual_ops.emplace_back(async(launch::async,
    //     DelegateClient, db, &wl, total_ops / num_threads, true));
    err = pthread_attr_init(&pattr);
    err = pthread_attr_setaffinity_np(&pattr, sizeof(cpu_set_t), &cpu);

    CPU_ZERO(&cpu);
    CPU_SET(i + 1, &cpu);
    args[i].core = i + 1;

    args[i].num_record = total_record / num_cores;
    args[i].num_ops = total_ops / num_cores;
    args[i].oks = 0;
    err = pthread_create(&pids[i], &pattr, DelegateClient, &args[i]);
      if (err) {
        printf(" [%s on core %d] pthread_create for core %d return %d!\n", __func__, sched_getcpu(), i + sched_getcpu() + 1, err);
      }
  }

  for (int i = 0; i < num_cores; ++i) {
    pthread_join(pids[i], NULL);
  }

  struct timeval begin, done;
  begin = args[0].start;
  done = args[0].end;
  for (int i = 1; i < num_cores; i++) {
    if (!timercmp(&begin, &args[i].start, <=)) {
      begin = args[i].start;
    }
    if (!timercmp(&done, &args[i].end, >=)) {
      done = args[i].end;
    }
  }

  int sum = 0;
  double thp = 0.0;
  for (int i = 0; i < num_cores; i++) {
    sum += args[i].oks;
    thp += args[i].oks / ((args[i].end.tv_sec - args[i].start.tv_sec) + (args[i].end.tv_usec - args[i].start.tv_usec) / 1.0e6);
    fprintf(stderr, " Core %d\tops\t%u\ttime\t%8.2f\tthp\t%8.2f\n", \
            args[i].core, args[i].oks, ((args[i].end.tv_sec - args[i].start.tv_sec) + (args[i].end.tv_usec - args[i].start.tv_usec) / 1.0e6),  \
            args[i].oks / ((args[i].end.tv_sec - args[i].start.tv_sec) + (args[i].end.tv_usec - args[i].start.tv_usec) / 1.0e6));
  }

  double duration = (done.tv_sec - begin.tv_sec) + (done.tv_usec - begin.tv_usec) / 1.0e6;

  cerr << "# Transaction throughput (KTPS)" << endl;
  // cerr << props->GetProperty("") << '\t' << file_name << '\t' << num_threads << '\t';
  cerr << props["dbname"] << '\t' << file_name << '\t';
  cerr << sum << '\t' << duration << '\t' << sum / duration / 1000 <<  '\t' << thp / 1000 << endl;

  return 0;
}

string ParseCommandLine(int argc, char *argv[], utils::Properties &props) {
  int argindex = 1;
  string filename;
  while (argindex < argc && StrStartWith(argv[argindex], "-")) {
    if (strcmp(argv[argindex], "-cores") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("corecount", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-threads") == 0) {
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

