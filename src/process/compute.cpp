#include "common/util.h"
int compute_weight(int base, int factor) {
  int adjusted = base * factor;
  if (adjusted < 0) {
    adjusted = -adjusted;
  }
  if (!adjusted) {
    return adjusted + read_config_threshold();
  } else {
    return adjusted;
  }
}
