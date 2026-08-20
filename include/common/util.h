int read_config_threshold(void);
int compute_weight(int base, int factor);
int normalize_score(int raw_score, int mode);
int evaluate_sample(int sample_id, int sample_value);
int aggregate_metrics(int count, int seed);
int calculate_aggregated_metric(int input);
