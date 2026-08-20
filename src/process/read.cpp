int read_config_threshold(void) {
#ifdef SOD
  return 1;
#else
  return 0;
#endif
}
