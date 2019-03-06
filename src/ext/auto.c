#include <php.h>
#include <time.h>

#include "ddtrace.h"
#include "auto.h"
#include "dispatch_compat.h"

ZEND_EXTERN_MODULE_GLOBALS(ddtrace);

#if PHP_VERSION_ID < 70000
#define _DTOR_PARAM_TYPE void
#define _DTOR_VALUE_CAST_FN_STATS(zv) (*(ddtrace_auto_stats_t **)zv)
#define _DTOR_VALUE_CAST_CLASS_STATS(zv) (*(HashTable **)zv)
#else
#define _DTOR_PARAM_TYPE zval
#define _DTOR_VALUE_CAST_FN_STATS(zv) (Z_PTR_P(zv))
#define _DTOR_VALUE_CAST_CLASS_STATS(zv) (Z_PTR_P(zv))
#endif

static void auto_fn_stats_dtor(_DTOR_PARAM_TYPE *zv) {
    ddtrace_auto_stats_t *stats = _DTOR_VALUE_CAST_FN_STATS(zv);
    pefree(stats, 1);
}

static void auto_class_stats_dtor(_DTOR_PARAM_TYPE *zv) {
    HashTable *ht = _DTOR_VALUE_CAST_CLASS_STATS(zv);
    zend_hash_destroy(ht);
    pefree(ht, 1);
}

void (*ddtrace_auto_execute_original_ex)(zend_execute_data *TSRMLS_DC);

void ddtrace_auto_execute(zend_execute_data *execute_data TSRMLS_DC) {
    ddtrace_auto_stats_t* stats = DDTRACE_G(auto_stats);
    struct timespec start_time, end_time;

    if (stats) {
        clock_gettime(CLOCK_MONOTONIC, &start_time);
    }

    if (ddtrace_auto_execute_original_ex) {
        ddtrace_auto_execute_original_ex(execute_data TSRMLS_CC);
    } else {
        execute_ex(execute_data TSRMLS_CC);
    }

    if (stats) {
        clock_gettime(CLOCK_MONOTONIC, &end_time);
        long duration = (end_time.tv_sec - start_time.tv_sec)*1000*1000 + (end_time.tv_nsec - start_time.tv_nsec)/1000;

        if (stats->avg_time > 0){
            stats->avg_time = (stats->avg_time + duration)/2;
        } else {
            stats->avg_time = duration;
        }
        stats->count++;
    }
}

void ddtrace_auto_inject(){
#if PHP_VERSION_ID >= 50600
    ddtrace_auto_execute_original_ex = zend_execute_ex;
    zend_execute_ex = ddtrace_auto_execute;
#endif
}

ddtrace_auto_stats_t* ddtrace_auto_record_fetch(zend_execute_data *ex, const char *function_name, size_t function_name_length TSRMLS_DC){
    zval* this = ddtrace_this(ex);
    HashTable *stats_lookup = NULL;
    ddtrace_auto_stats_t *auto_stats = NULL;

    if (!this) {
        stats_lookup = &DDTRACE_G(auto_function_lookup);
    }

    if (stats_lookup) {
        char *key = zend_str_tolower_dup(function_name, function_name_length);
        auto_stats = zend_hash_str_find_ptr(stats_lookup, key, function_name_length);

        if (!auto_stats){
            auto_stats = pecalloc(1, sizeof(ddtrace_auto_stats_t), 1);
            zend_hash_str_add_ptr(stats_lookup, function_name, function_name_length, auto_stats);
        }

        efree(key);
    }

    return auto_stats;
}

// void ddtrace_auto_find_and_mark(zend_execute_data *ex, const char *function_name, size_t function_name_length TSRMLS_DC) {
//     DDTRACE_G(auto_prev_stats) = ddtrace_auto_record_fetch(ex, function_name, function_name_length);
// }

void ddtrace_auto_minit(TSRMLS_D){
    DDTRACE_G(auto_stats) = NULL;
    zend_hash_init(&DDTRACE_G(auto_class_lookup), 128, NULL, (dtor_func_t)auto_class_stats_dtor, 1);
    zend_hash_init(&DDTRACE_G(auto_function_lookup), 128, NULL, (dtor_func_t)auto_fn_stats_dtor, 1);
}

void ddtrace_auto_mdestroy(TSRMLS_D){
    zend_hash_destroy(&DDTRACE_G(auto_class_lookup));
    zend_hash_destroy(&DDTRACE_G(auto_function_lookup));
}