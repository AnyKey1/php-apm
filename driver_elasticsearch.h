#ifndef DRIVER_ELASTICSEARCH_H
#define DRIVER_ELASTICSEARCH_H

#ifdef HAVE_CURL
void apm_driver_elasticsearch_process_event(int type, char *error_filename, uint error_lineno, char *msg, char *trace TSRMLS_DC);
void apm_driver_elasticsearch_process_stats(TSRMLS_D);
int apm_driver_elasticsearch_minit(int module_number TSRMLS_DC);
int apm_driver_elasticsearch_rshutdown(TSRMLS_D);
int apm_driver_elasticsearch_mshutdown(TSRMLS_D);
PHP_INI_MH(OnUpdateAPMelasticsearchErrorReporting);
#endif

#endif

