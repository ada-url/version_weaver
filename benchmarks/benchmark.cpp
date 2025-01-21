
#include "performancecounters/benchmarker.h"
#include "version_weaver.h"
#include <algorithm>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <stdlib.h>
#include <vector>

void pretty_print(size_t volume, size_t bytes, std::string name,
                  event_aggregate agg, bool display = true) {
  printf("%-40s : ", name.c_str());
  printf(" %5.2f Gi/s ", volume / agg.fastest_elapsed_ns());
  double range =
      (agg.elapsed_ns() - agg.fastest_elapsed_ns()) / agg.elapsed_ns() * 100.0;
  printf(" %5.2f GB/s (%2.0f %%) ", bytes / agg.fastest_elapsed_ns(), range);
  if (collector.has_events()) {
    printf(" %5.2f GHz ", agg.fastest_cycles() / agg.fastest_elapsed_ns());
    printf(" %5.2f c/b ", agg.fastest_cycles() / bytes);
    printf(" %5.2f i/b ", agg.fastest_instructions() / bytes);
    printf(" %5.2f i/c ", agg.fastest_instructions() / agg.fastest_cycles());
  }
  printf("\n");
}

void bench(const std::vector<std::string> &input) {
  size_t volume = input.size();
  size_t bytes = 0;
  for (size_t i = 0; i < volume; i++) {
    bytes += input[i].size();
  }
  std::cout << "volume      : " << volume << " strings" << std::endl;
  std::cout << "volume      : " << bytes / 1024 / 1024. << " MB" << std::endl;
  volatile size_t sum = 0;

  size_t min_repeat = 10;
  size_t min_time_ns = 10000000000;
  size_t max_repeat = 100000;
  pretty_print(volume, bytes, "validate",
               bench(
                   [&input, &sum]() {
                     for (std::string_view v : input) {
                       sum = sum + version_weaver::validate(v);
                     }
                   },
                   min_repeat, min_time_ns, max_repeat));
}

int main(int argc, char **argv) {
  bench({"1.2.4", "13.4.1"});
  return EXIT_SUCCESS;
}
