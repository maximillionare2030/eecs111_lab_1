#include <vector>
#include <pthread.h>
#include <cstring>
#include <string>
#include <cstdlib>

#include "p1_process.h"
#include "p1_threads.h"

using namespace std;

// This file implements the ParallelMergeSorter class definition found in p1_threads.h
// It is not required to use the defined classes/functions, but it may be helpful




// This struct is used to pass arguments into each thread (inside of thread_init)
// ctx is essentially the "this" keyword, but since we are in a static function, we cannot use "this"
//
// Feel free to modify this struct in any way
struct MergeSortArgs {
  int thread_index;
  ParallelMergeSorter * ctx;

  MergeSortArgs(ParallelMergeSorter * ctx, int thread_index) {
    this->ctx = ctx;
    this->thread_index = thread_index;
  }
};


// Class constructor
ParallelMergeSorter::ParallelMergeSorter(vector<student> &original_list, int num_threads) {
  this->threads = vector<pthread_t>();
  this->sorted_list = vector<student>(original_list);
  this->num_threads = num_threads;
}

// This function will be called by each child process to perform multithreaded sorting
vector<student> ParallelMergeSorter::run_sort() {
  for (int i = 0; i < num_threads; i++) {
    // We have to use the heap for this otherwise args will be destructed in each iteration,
    // and the thread will not have the correct args struct
    int effective_threads = num_threads;
    if (effective_threads > (int)sorted_list.size() && (int)sorted_list.size() > 0) {
      effective_threads = (int)sorted_list.size(); // cap the number of threads to use
    }
    if (effective_threads < 1) effective_threads = 1;
    this->num_threads = effective_threads;

    for (int i = 0; i < effective_threads; i++) {
      MergeSortArgs * args = new MergeSortArgs(this, i);
      pthread_t tid;
      pthread_create(&tid, NULL, ParallelMergeSorter::thread_init, (void *) args); // piid for thread
      threads.push_back(tid);
    }

    // join back to main after
    for (int i = 0; i < (int)threads.size(); i++) {
      pthread_join(threads[i], NULL);
    }
  }
  // Merge sorted sublists together
  this->merge_threads();

  return this->sorted_list;
}

// Standard merge sort implementation
void ParallelMergeSorter::merge_sort(int lower_bound, int upper_bound) {
  if (upper_bound - lower_bound <= 1) return;
  int middle = lower_bound + (upper_bound - lower_bound) / 2;
  merge_sort(lower_bound, middle);
  merge_sort(middle, upper_bound);
  merge(lower_bound, middle, upper_bound);
}


// Standard merge implementation for merge sort
void ParallelMergeSorter::merge(int lower_bound, int middle, int upper_bound) {
  vector<student> tmp;
  int i = lower_bound, j = middle;
  while (i < middle && j < upper_bound) {
    if (sorted_list[i].grade >= sorted_list[j].grade) {
      tmp.push_back(sorted_list[i++]);
    } else {
      tmp.push_back(sorted_list[j++]);
    }
  }
  while (i < middle) tmp.push_back(sorted_list[i++]);
  while (j < upper_bound) tmp.push_back(sorted_list[j++]);
  for (int k = 0; k < (int)tmp.size(); k++) {
    sorted_list[lower_bound + k] = tmp[k];
  }
}

// This function will be used to merge the resulting sorted sublists together
void ParallelMergeSorter::merge_threads() {
  if (num_threads <= 1) return;

  int work_per_thread = (int)sorted_list.size() / num_threads;
  int merged_upper = work_per_thread;

  for (int i = 1; i < num_threads; i++) {
    int next_upper;
    if (i == num_threads - 1) {
      next_upper = (int)sorted_list.size();
    } else {
      next_upper = (i + 1) * work_per_thread;
    }
    merge(0, merged_upper, next_upper);
    merged_upper = next_upper;
  }
}

// This function is the start routine for the created threads, it should perform merge sort on its assigned sublist
// Since this function is static (pthread_create must take a static function), we cannot access "this" and must use ctx instead
void * ParallelMergeSorter::thread_init(void * args) {
  MergeSortArgs * sort_args = (MergeSortArgs *) args;
  int thread_index = sort_args->thread_index;
  ParallelMergeSorter * ctx = sort_args->ctx;

  int work_per_thread = ctx->sorted_list.size() / ctx->num_threads;
  int n = (int)ctx->sorted_list.size();
  int lower_bound = thread_index * work_per_thread;
  int upper_bound;

  if (thread_index == ctx->num_threads - 1) {
    upper_bound = n;
  } else {
    upper_bound = (thread_index + 1) * work_per_thread;
  }

  // Free the heap allocation
  delete sort_args;
  return NULL;
}

