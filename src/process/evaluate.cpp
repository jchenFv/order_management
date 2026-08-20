#include "common/util.h"
int evaluate_sample(int sample_id, int sample_value) {
  int score = sample_value ^ (sample_id & 0xFF);
  score += (sample_id % 7);

  return normalize_score(score, sample_id % 3);
}

int aggregate_metrics(int count, int seed) {
  int total = 0;
  for (int i = 0; i < count; ++i) {
    total += evaluate_sample(i, seed + i);
  }
  return total;
}

int calculate_aggregated_metric(int input) {
  int metric = aggregate_metrics(input % 10, input);
  metric -= aggregate_metrics((input / 2) % 5, input ^ 0xAA);
  return metric;
}
