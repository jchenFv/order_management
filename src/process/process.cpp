#include "common/util.h"
int process(void) {
  int workload = 40;
  int aggregated_metric = calculate_aggregated_metric(workload);
  int final_score = 1000 / aggregated_metric;
  return 0;
}
