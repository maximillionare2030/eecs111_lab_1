#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <cstdio>
#include <cmath>
#include <unistd.h>
#include <sys/wait.h>

#include "p1_process.h"
#include "p1_threads.h"

using namespace std;

static void write_sorted_output(const vector<student> &sorted, const string &output_file_name);
static void write_stats_output(const vector<student> &sorted, const string &output_file_name);

// This file implements the multi-processing logic for the project
// This function should be called in each child process right after forking
// The input vector should be a subset of the original files vector
void process_classes(vector<string> classes, int num_threads) {
  printf("Child process is created. (pid: %d)\n", getpid());
  // Each process should use the sort function which you have defined
  // in the p1_threads.cpp for multithread sorting of the data.

  for (int i = 0; i < classes.size(); i++) {
    // get all the input/output file names here

    string class_name = classes[i];
    char buffer[256]; // increase buffer size

    snprintf(buffer, sizeof(buffer), "input/%s.csv", class_name.c_str());
    string input_file_name(buffer);

    snprintf(buffer, sizeof(buffer), "output/%s_sorted.csv", class_name.c_str());
    string output_sorted_file_name(buffer);

    snprintf(buffer, sizeof(buffer), "output/%s_stats.csv", class_name.c_str());
    string output_stats_file_name(buffer);

    // Read input from CSV
    vector<student> students; // uses id and grade attr
    FILE * file_input = fopen(input_file_name.c_str(), "r");

    if (!file_input) {
      printf("[ERROR] Input file invalid %s\n", input_file_name.c_str());
      continue;
    }

    // Processing lines
    char line[256];
    fgets(line, sizeof(line), file_input);

    while (fgets(line, sizeof(line), file_input)) {
      unsigned long id;
      double grade;

      if (sscanf(line, "%lu,%lf", &id, &grade) == 2) {
        students.push_back(student(id, grade));
      }
    }
    fclose(file_input);


    // Funciton sort
    ParallelMergeSorter sorter(students, num_threads);
    vector<student> sorted = sorter.run_sort();


    // handle output files
    write_sorted_output(sorted, output_sorted_file_name);
    write_stats_output(sorted, output_stats_file_name);
  }

  // child process done, exit the program
  printf("Child process is terminated. (pid: %d)\n", getpid());
  exit(0);
}

static void write_sorted_output(const vector<student> &sorted, const string &output_file_name) {
  FILE *file_sorted = fopen(output_file_name.c_str(), "w");
  if (!file_sorted) {
    printf("[ERROR] Failed to write sorted output %s\n", output_file_name.c_str());
    return;
  }

  fprintf(file_sorted, "Rank,Student ID,Grade\n");
  for (size_t j = 0; j < sorted.size(); j++) {
    fprintf(file_sorted, "%zu,%lu,%.3f\n", j + 1, sorted[j].id, sorted[j].grade);
  }

  fclose(file_sorted);
}

static void write_stats_output(const vector<student> &sorted, const string &output_file_name) {
  double avg = 0.0;
  double median = 0.0;
  double std_dev = 0.0;

  if (!sorted.empty()) {
    size_t n = sorted.size();

    double total = 0.0;
    for (size_t j = 0; j < n; j++) {
      total += sorted[j].grade;
    }
    avg = total / n;

    if (n % 2 == 0) {
      median = (sorted[n / 2 - 1].grade + sorted[n / 2].grade) / 2.0;
    } else {
      median = sorted[n / 2].grade;
    }

    double sum_sq = 0.0;
    for (size_t j = 0; j < n; j++) {
      double diff = sorted[j].grade - avg;
      sum_sq += diff * diff;
    }
    std_dev = sqrt(sum_sq / n);
  }

  FILE *file_stats = fopen(output_file_name.c_str(), "w");
  if (!file_stats) {
    printf("[ERROR] Failed to write stats output %s\n", output_file_name.c_str());
    return;
  }

  fprintf(file_stats, "Average,Median,Std. Dev\n");
  fprintf(file_stats, "%.3f,%.3f,%.3f\n", avg, median, std_dev);
  fclose(file_stats);
}


void create_processes_and_sort(vector<string> class_names, int num_processes, int num_threads) {
  int num_files = (int)class_names.size();

  // Cap process to avoid creating non-working processes on invalid files
  if (num_processes > num_files) {
    num_processes = num_files;
  }

  int base = num_files / num_processes;
  int remainder = num_files % num_processes;

  vector<pid_t> child_pids;
  int start = 0;

  for (int i = 0; i < num_processes; i++) {
    int count = base + (i < remainder ? 1 : 0);
    vector<string> subset(class_names.begin() + start, class_names.begin() + start + count);
    start += count;

    pid_t pid = fork();
    if (pid == 0) {
      // child process
      process_classes(subset, num_threads);
      // process_classes calls exit(0), never returns
    } else if (pid > 0) {
      child_pids.push_back(pid);
    } else {
      printf("[ERROR] fork failed\n");
    }
  }

  // Wait for all child processes to finish
  for (int i = 0; i < (int)child_pids.size(); i++) {
    waitpid(child_pids[i], NULL, 0);
  }
}
