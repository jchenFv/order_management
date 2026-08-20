#include "common/util.h"
int normalize_score(int raw_score, int mode) {
  int weight = compute_weight(raw_score, 3);
  int normalized = weight * mode;
  return normalized;
}
