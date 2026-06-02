#include "php_apm.h"
#ifdef HAVE_CURL
#include <curl/curl.h>
#include "SAPI.h"
#include <sys/time.h>

ZEND_EXTERN_MODULE_GLOBALS(apm);

PHP_INI_MH(OnUpdateAPMelasticsearchErrorReporting)
{
    APM_G(elasticsearch_error_reporting) = (apm_error_reporting_new_value : APM_E_ALL);
    return SUCCESS;
}

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    return size * nmemb;
}

static int send_to_elasticsearch(TSRMLS_D) {
    CURL *curl;
    CURLcode res;
    char url[1024];
    struct curl_slist *headers = NULL;
    char *user = NULL;
    char *pass = NULL;
    long timeout;
    
    if (!APM_G(elasticsearch_enabled) || APM_G(elasticsearch_batch_buffer).c == NULL || APM_G(elasticsearch_batch_buffer).len == 0) {
        return 0;
    }
    
    snprintf(url, sizeof(url), "http://%s:%ld/_bulk", 
             APM_G(elasticsearch_host), APM_G(elasticsearch_port));

    curl = curl_easy_init();
    if (!curl) return 0;

    headers = curl_slist_append(headers, "Content-Type: application/x-ndjson");
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    
    /* Ensure null termination */
    smart_str_0(&APM_G(elasticsearch_batch_buffer));
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, APM_G(elasticsearch_batch_buffer).c);
    
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    
    /* Use the configured timeout */
    timeout = APM_G(elasticsearch_batch_timeout) > 0 ? APM_G(elasticsearch_batch_timeout) : 5L;
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);

    /* Use username/password if configured, fallback to user/pass */
    user = (APM_G(elasticsearch_username) && strlen(APM_G(elasticsearch_username)) > 0) ? APM_G(elasticsearch_username) : APM_G(elasticsearch_user);
    pass = (APM_G(elasticsearch_password) && strlen(APM_G(elasticsearch_password)) > 0) ? APM_G(elasticsearch_password) : APM_G(elasticsearch_pass);

    if (user && strlen(user) > 0) {
        char auth[512];
        snprintf(auth, sizeof(auth), "%s:%s", user, pass ? pass : "");
        curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
        curl_easy_setopt(curl, CURLOPT_USERPWD, auth);
    }

    res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    /* Reset batch state */
    smart_str_free(&APM_G(elasticsearch_batch_buffer));
    APM_G(elasticsearch_batch_buffer).c = NULL;
    APM_G(elasticsearch_batch_buffer).len = 0;
    APM_G(elasticsearch_batch_buffer).a = 0;
    APM_G(elasticsearch_batch_count) = 0;
    
    return (res == CURLE_OK) ? 1 : 0;
}

static void append_to_batch(const char *json TSRMLS_DC) {
    char bulk_index_header[512];
    
    snprintf(bulk_index_header, sizeof(bulk_index_header), "{\"index\":{\"_index\":\"%s\"}}\n", APM_G(elasticsearch_index));
    smart_str_appends(&APM_G(elasticsearch_batch_buffer), bulk_index_header);
    smart_str_appends(&APM_G(elasticsearch_batch_buffer), json);
    smart_str_appends(&APM_G(elasticsearch_batch_buffer), "\n");
    APM_G(elasticsearch_batch_count)++;
    
    if (APM_G(elasticsearch_batch_size) > 0 && APM_G(elasticsearch_batch_count) >= APM_G(elasticsearch_batch_size)) {
        send_to_elasticsearch(TSRMLS_C);
    }
}

void apm_driver_elasticsearch_process_event(int type, char *error_filename, uint error_lineno, char *msg, char *trace TSRMLS_DC) {
    char json[8192];
    char timestamp[32];
    time_t now;
    struct tm *tm_info;
    int response_code = SG(sapi_headers).http_response_code;
    
    if (!APM_G(elasticsearch_enabled)) return;
    
    /* If the response code is 200 (or not set) but we caught a fatal error, override it with 500 */
    if ((type == E_ERROR || type == E_PARSE || type == E_CORE_ERROR || type == E_COMPILE_ERROR || type == E_USER_ERROR || type == E_EXCEPTION) && 
        (response_code == 0 || response_code == 200)) {
        response_code = 500;
    }
    
    time(&now);
    tm_info = gmtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", tm_info);
    
    snprintf(json, sizeof(json),
        "{\"@timestamp\":\"%s\",\"type\":\"error\",\"error_type\":%d,\"message\":\"%s\",\"file\":\"%s\",\"line\":%u,\"trace\":\"%s\",\"application\":\"%s\",\"response_code\":%d}",
        timestamp, type, msg ? msg : "", error_filename ? error_filename : "", error_lineno, trace ? trace : "", APM_G(application_id) ? APM_G(application_id) : "default", response_code);
    
    append_to_batch(json TSRMLS_CC);
}

void apm_driver_elasticsearch_process_stats(TSRMLS_D) {
    char json[4096];
    char timestamp[32];
    time_t now;
    struct tm *tm_info;
    struct timeval end_tp;
    double duration = 0.0;
    extern struct timeval begin_tp;
    
    if (!APM_G(elasticsearch_enabled) || !APM_G(elasticsearch_stats_enabled)) return;
    
    extract_data(TSRMLS_C);
    
    gettimeofday(&end_tp, NULL);
    duration = (double) (SEC_TO_USEC(end_tp.tv_sec - begin_tp.tv_sec) + end_tp.tv_usec - begin_tp.tv_usec);
    
    time(&now);
    tm_info = gmtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", tm_info);
    
    snprintf(json, sizeof(json),
        "{\"@timestamp\":\"%s\",\"type\":\"stats\",\"memory_usage\":%ld,\"memory_peak\":%ld,\"application\":\"%s\",\"response_code\":%d,\"duration_ms\":%.2f,\"uri\":\"%s\",\"method\":\"%s\"}",
        timestamp, zend_memory_usage(0 TSRMLS_CC), zend_memory_peak_usage(0 TSRMLS_CC), APM_G(application_id) ? APM_G(application_id) : "default",
        SG(sapi_headers).http_response_code, duration / 1000.0,
        APM_RD(uri_found) ? APM_RD_STRVAL(uri) : "",
        APM_RD(method_found) ? APM_RD_STRVAL(method) : "");
    
    append_to_batch(json TSRMLS_CC);
}

int apm_driver_elasticsearch_minit(int module_number TSRMLS_DC) {
    curl_global_init(CURL_GLOBAL_ALL);
    return SUCCESS;
}

int apm_driver_elasticsearch_rshutdown(TSRMLS_D) {
    /* Flush any remaining events at the end of the request */
    if (APM_G(elasticsearch_batch_count) > 0) {
        send_to_elasticsearch(TSRMLS_C);
    }
    return SUCCESS;
}

int apm_driver_elasticsearch_mshutdown(TSRMLS_D) {
    curl_global_cleanup();
    return SUCCESS;
}
#endif
