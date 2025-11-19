/*
 +----------------------------------------------------------------------+
 |  APM stands for Alternative PHP Monitor                              |
 +----------------------------------------------------------------------+
 | Copyright (c) 2008-2014  Davide Mendolia, Patrick Allaert            |
 +----------------------------------------------------------------------+
 | This source file is subject to version 3.01 of the PHP license,      |
 | that is bundled with this package in the file LICENSE, and is        |
 | available through the world-wide-web at the following url:           |
 | http://www.php.net/license/3_01.txt                                  |
 | If you did not receive a copy of the PHP license and are unable to   |
 | obtain it through the world-wide-web, please send a note to          |
 | license@php.net so we can mail you a copy immediately.               |
 +----------------------------------------------------------------------+
 | Authors: Patrick Allaert <patrickallaert@php.net>                    |
 +----------------------------------------------------------------------+
*/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <curl/curl.h>
#include "php_apm.h"
#include "php_ini.h"
#include "driver_elasticsearch.h"
#include "SAPI.h"

ZEND_EXTERN_MODULE_GLOBALS(apm);

APM_DRIVER_CREATE(elasticsearch)

/* Simple JSON string escape function */
static void append_json_string(smart_str *dest, const char *src)
{
	const char *p;
	if (!src) {
		smart_str_appends(dest, "\"\"");
		return;
	}

	smart_str_appendc(dest, '"');
	for (p = src; *p; p++) {
		switch (*p) {
			case '"':
				smart_str_appends(dest, "\\\"");
				break;
			case '\\':
				smart_str_appends(dest, "\\\\");
				break;
			case '\n':
				smart_str_appends(dest, "\\n");
				break;
			case '\r':
				smart_str_appends(dest, "\\r");
				break;
			case '\t':
				smart_str_appends(dest, "\\t");
				break;
			default:
				smart_str_appendc(dest, *p);
		}
	}
	smart_str_appendc(dest, '"');
}

/* Helper function to build JSON for events */
static void build_event_json(smart_str *json, int type, char *error_filename, uint error_lineno, char *msg, char *trace TSRMLS_DC)
{
	char timestamp[32];
	time_t now = time(NULL);
	const char *type_string;

	strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S", gmtime(&now));

	switch(type) {
		case E_EXCEPTION:
			type_string = "exception";
			break;
		case E_ERROR:
			type_string = "error";
			break;
		case E_WARNING:
			type_string = "warning";
			break;
		case E_PARSE:
			type_string = "parse_error";
			break;
		case E_NOTICE:
			type_string = "notice";
			break;
		case E_CORE_ERROR:
			type_string = "core_error";
			break;
		case E_CORE_WARNING:
			type_string = "core_warning";
			break;
		case E_COMPILE_ERROR:
			type_string = "compile_error";
			break;
		case E_COMPILE_WARNING:
			type_string = "compile_warning";
			break;
		case E_USER_ERROR:
			type_string = "user_error";
			break;
		case E_USER_WARNING:
			type_string = "user_warning";
			break;
		case E_USER_NOTICE:
			type_string = "user_notice";
			break;
		case E_STRICT:
			type_string = "strict";
			break;
		case E_RECOVERABLE_ERROR:
			type_string = "recoverable_error";
			break;
		case E_DEPRECATED:
			type_string = "deprecated";
			break;
		case E_USER_DEPRECATED:
			type_string = "user_deprecated";
			break;
		default:
			type_string = "unknown";
	}

	smart_str_appends(json, "{\"@timestamp\":\"");
	smart_str_appends(json, timestamp);
	smart_str_appends(json, "\",\"type\":\"event\",\"event_type\":\"");
	smart_str_appends(json, type_string);
	smart_str_appends(json, "\",\"severity\":");
	smart_str_append_long(json, type);
	
	if (APM_G(application_id)) {
		smart_str_appends(json, ",\"application_id\":\"");
		smart_str_appends(json, APM_G(application_id));
		smart_str_appends(json, "\"");
	}

	if (error_filename) {
		smart_str_appends(json, ",\"file\":\"");
		smart_str_appends(json, error_filename);
		smart_str_appends(json, "\"");
	}

	if (error_lineno > 0) {
		smart_str_appends(json, ",\"line\":");
		smart_str_append_long(json, error_lineno);
	}

	if (msg) {
		smart_str_appends(json, ",\"message\":");
		append_json_string(json, msg);
	}

	if (trace) {
		smart_str_appends(json, ",\"backtrace\":");
		append_json_string(json, trace);
	}

	smart_str_appends(json, "}");
}

/* Helper function to build JSON for stats */
static void build_stats_json(smart_str *json TSRMLS_DC)
{
	char timestamp[32];
	time_t now = time(NULL);

	strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S", gmtime(&now));

	smart_str_appends(json, "{\"@timestamp\":\"");
	smart_str_appends(json, timestamp);
	smart_str_appends(json, "\",\"type\":\"stats\"");

	if (APM_G(application_id)) {
		smart_str_appends(json, ",\"application_id\":\"");
		smart_str_appends(json, APM_G(application_id));
		smart_str_appends(json, "\"");
	}

	smart_str_appends(json, ",\"duration\":");
	smart_str_append_long(json, (long)(APM_G(duration) / 1000));

#ifdef HAVE_GETRUSAGE
	smart_str_appends(json, ",\"user_cpu\":");
	smart_str_append_long(json, (long)(APM_G(user_cpu) / 1000));

	smart_str_appends(json, ",\"sys_cpu\":");
	smart_str_append_long(json, (long)(APM_G(sys_cpu) / 1000));
#endif

	smart_str_appends(json, ",\"mem_peak_usage\":");
	smart_str_append_long(json, APM_G(mem_peak_usage));

	smart_str_appends(json, ",\"response_code\":");
	smart_str_append_long(json, SG(sapi_headers).http_response_code);

	extract_data(TSRMLS_C);

	if (APM_RD(uri_found)) {
		smart_str_appends(json, ",\"uri\":");
		append_json_string(json, APM_RD_STRVAL(uri));
	}

	if (APM_RD(host_found)) {
		smart_str_appends(json, ",\"host\":");
		append_json_string(json, APM_RD_STRVAL(host));
	}

	if (APM_RD(method_found)) {
		smart_str_appends(json, ",\"method\":");
		append_json_string(json, APM_RD_STRVAL(method));
	}

	smart_str_appends(json, "}");
}

/* Flush buffer to Elasticsearch using Bulk API */
static int flush_buffer_to_elasticsearch(TSRMLS_D)
{
	CURL *curl;
	CURLcode res;
	char url[512];
	char auth[256];
	struct curl_slist *headers = NULL;
	int success = 0;

	if (APM_G(elasticsearch_buffer_count) == 0) {
		return 1; /* Nothing to send */
	}

#if PHP_VERSION_ID >= 70000
	if (!APM_G(elasticsearch_buffer).s || ZSTR_LEN(APM_G(elasticsearch_buffer).s) == 0) {
#else
	if (!APM_G(elasticsearch_buffer).c || APM_G(elasticsearch_buffer).len == 0) {
#endif
		return 1; /* Empty buffer */
	}

	smart_str_0(&APM_G(elasticsearch_buffer));

	curl = curl_easy_init();
	if (!curl) {
		APM_DEBUG("[Elasticsearch driver] Failed to initialize CURL\n");
		return 0;
	}

	/* Build URL for Bulk API: http://host:port/_bulk */
	snprintf(url, sizeof(url), "http://%s:%u/_bulk",
		APM_G(elasticsearch_host),
		APM_G(elasticsearch_port));

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_POST, 1L);
#if PHP_VERSION_ID >= 70000
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, ZSTR_VAL(APM_G(elasticsearch_buffer).s));
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, ZSTR_LEN(APM_G(elasticsearch_buffer).s));
#else
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, APM_G(elasticsearch_buffer).c);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, APM_G(elasticsearch_buffer).len);
#endif
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2L);

	/* Set Content-Type header for Bulk API */
	headers = curl_slist_append(headers, "Content-Type: application/x-ndjson");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

	/* Set basic auth if configured */
	if (APM_G(elasticsearch_username) && strlen(APM_G(elasticsearch_username)) > 0) {
		snprintf(auth, sizeof(auth), "%s:%s",
			APM_G(elasticsearch_username),
			APM_G(elasticsearch_password) ? APM_G(elasticsearch_password) : "");
		curl_easy_setopt(curl, CURLOPT_USERPWD, auth);
	}

	APM_DEBUG("[Elasticsearch driver] Sending batch of %d documents to %s\n", 
		APM_G(elasticsearch_buffer_count), url);

	res = curl_easy_perform(curl);
	if (res == CURLE_OK) {
		success = 1;
		APM_DEBUG("[Elasticsearch driver] Batch sent successfully\n");
	} else {
		APM_DEBUG("[Elasticsearch driver] CURL error: %s\n", curl_easy_strerror(res));
	}

	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);

	/* Clear buffer */
	smart_str_free(&APM_G(elasticsearch_buffer));
	memset(&APM_G(elasticsearch_buffer), 0, sizeof(smart_str));
	APM_G(elasticsearch_buffer_count) = 0;
	APM_G(elasticsearch_buffer_start_time) = 0;

	return success;
}

/* Add document to buffer */
static void add_to_buffer(smart_str *document TSRMLS_DC)
{
	char index_action[512];

	if (!document) {
		return;
	}

	smart_str_0(document);

	/* Bulk API format: action line, then document line */
	snprintf(index_action, sizeof(index_action), 
		"{\"index\":{\"_index\":\"%s\"}}\n", 
		APM_G(elasticsearch_index));

	smart_str_appends(&APM_G(elasticsearch_buffer), index_action);

#if PHP_VERSION_ID >= 70000
	if (document->s) {
		smart_str_append(&APM_G(elasticsearch_buffer), document->s);
	}
#else
	if (document->c) {
		smart_str_appendl(&APM_G(elasticsearch_buffer), document->c, document->len);
	}
#endif

	smart_str_appendc(&APM_G(elasticsearch_buffer), '\n');

	APM_G(elasticsearch_buffer_count)++;

	/* Set start time for timeout if this is the first document */
	if (APM_G(elasticsearch_buffer_count) == 1) {
		APM_G(elasticsearch_buffer_start_time) = time(NULL);
	}

	/* Check if we should flush based on batch size */
	if (APM_G(elasticsearch_buffer_count) >= APM_G(elasticsearch_batch_size)) {
		APM_DEBUG("[Elasticsearch driver] Batch size reached (%d), flushing\n", 
			APM_G(elasticsearch_buffer_count));
		flush_buffer_to_elasticsearch(TSRMLS_C);
	}
}

/* Check and flush buffer if timeout reached */
static void check_buffer_timeout(TSRMLS_D)
{
	time_t now;

	if (APM_G(elasticsearch_buffer_count) == 0) {
		return;
	}

	now = time(NULL);
	if ((now - APM_G(elasticsearch_buffer_start_time)) >= APM_G(elasticsearch_batch_timeout)) {
		APM_DEBUG("[Elasticsearch driver] Batch timeout reached (%ld seconds), flushing\n", 
			(long)(now - APM_G(elasticsearch_buffer_start_time)));
		flush_buffer_to_elasticsearch(TSRMLS_C);
	}
}

/* Insert an event in Elasticsearch */
void apm_driver_elasticsearch_process_event(PROCESS_EVENT_ARGS)
{
	smart_str document = {0};

	build_event_json(&document, type, error_filename, error_lineno, msg, trace TSRMLS_CC);
	add_to_buffer(&document TSRMLS_CC);
	smart_str_free(&document);

	/* Check timeout after adding */
	check_buffer_timeout(TSRMLS_C);
}

/* Module initialization */
int apm_driver_elasticsearch_minit(int module_number TSRMLS_DC)
{
	if (!(APM_G(enabled) && APM_G(elasticsearch_enabled))) {
		return SUCCESS;
	}

	/* Initialize CURL globally */
	curl_global_init(CURL_GLOBAL_DEFAULT);

	/* Validate configuration */
	if (!APM_G(elasticsearch_host) || strlen(APM_G(elasticsearch_host)) == 0) {
		zend_error(E_CORE_WARNING, "APM Elasticsearch driver: elasticsearch_host is not configured");
		APM_G(elasticsearch_enabled) = 0;
		return SUCCESS;
	}

	if (!APM_G(elasticsearch_index) || strlen(APM_G(elasticsearch_index)) == 0) {
		zend_error(E_CORE_WARNING, "APM Elasticsearch driver: elasticsearch_index is not configured");
		APM_G(elasticsearch_enabled) = 0;
		return SUCCESS;
	}

	/* Validate batch configuration */
	if (APM_G(elasticsearch_batch_size) < 1) {
		APM_G(elasticsearch_batch_size) = 1;
	}
	if (APM_G(elasticsearch_batch_timeout) < 1) {
		APM_G(elasticsearch_batch_timeout) = 1;
	}

	APM_DEBUG("[Elasticsearch driver] Initialized with host=%s:%u, index=%s, batch_size=%ld, batch_timeout=%ld\n",
		APM_G(elasticsearch_host), APM_G(elasticsearch_port), APM_G(elasticsearch_index),
		APM_G(elasticsearch_batch_size), APM_G(elasticsearch_batch_timeout));

	return SUCCESS;
}

/* Request initialization */
int apm_driver_elasticsearch_rinit(TSRMLS_D)
{
	/* Initialize buffer */
	memset(&APM_G(elasticsearch_buffer), 0, sizeof(smart_str));
	APM_G(elasticsearch_buffer_count) = 0;
	APM_G(elasticsearch_buffer_start_time) = 0;

	return SUCCESS;
}

/* Module shutdown */
int apm_driver_elasticsearch_mshutdown(SHUTDOWN_FUNC_ARGS)
{
	if (!(APM_G(enabled) && APM_G(elasticsearch_enabled))) {
		return SUCCESS;
	}

	curl_global_cleanup();

	return SUCCESS;
}

/* Request shutdown */
int apm_driver_elasticsearch_rshutdown(TSRMLS_D)
{
	/* Flush any remaining buffered data */
	if (APM_G(elasticsearch_buffer_count) > 0) {
		APM_DEBUG("[Elasticsearch driver] Flushing remaining %d documents at request shutdown\n",
			APM_G(elasticsearch_buffer_count));
		flush_buffer_to_elasticsearch(TSRMLS_C);
	}

	/* Clean up buffer */
	smart_str_free(&APM_G(elasticsearch_buffer));

	return SUCCESS;
}

/* Process statistics */
void apm_driver_elasticsearch_process_stats(TSRMLS_D)
{
	smart_str document = {0};

	build_stats_json(&document TSRMLS_CC);
	add_to_buffer(&document TSRMLS_CC);
	smart_str_free(&document);

	/* Check timeout after adding */
	check_buffer_timeout(TSRMLS_C);
}
