#include <execinfo.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

extern "C" {

typedef struct {
  void* fptr;
  unsigned int counter;
  double start;
  double total_seconds;
} FunctionMap;

FunctionMap function_map[2];
static char functions[2][128] = {"core_list_mergesort", "matrix_mul_const"};
static int func_num = 2;

static FILE* fp_trace;

void __attribute__((constructor)) init(void) {
  FILE* tmp = fopen("instrument.out", "r");
  if (tmp != NULL) {
    fclose(tmp);
    fp_trace = fopen("instrument.out", "a");
  } else {
    fp_trace = fopen("instrument.out", "w");
    if (fp_trace != NULL) {
      fprintf(fp_trace, "name,counter,total_time\n");
    }
  }
  for (int i = 0; i < func_num; i++) {
    function_map[i].fptr = NULL;
    function_map[i].counter = 0;
    function_map[i].start = 0;
    function_map[i].total_seconds = 0;
  }
}

void __attribute__((destructor)) stop(void) {
  if (fp_trace != NULL) {
    for (int i = 0; i < func_num; i++) {
      fprintf(fp_trace, "%s,%d,%f\n", functions[i], function_map[i].counter,
              function_map[i].total_seconds);
    }
    fclose(fp_trace);
  }
}

#define PRINT_FUNCTION(fname, func, flag)        \
  double t = (double)(clock()) / CLOCKS_PER_SEC; \
  fprintf(fp_trace, "%s [%p] %s %f\n", flag, func, fname, t);

void __cyg_profile_func_exit(void* callee, void* callsite)
    __attribute__((no_instrument_function));

void __cyg_profile_func_enter(void* callee, void* callsite)
    __attribute__((no_instrument_function));

// file, line_num, func_name
void __cyg_profile_func_enter(void* callee, void* callsite) {
  int index = -1;
  int check = 0;
  for (int i = 0; i < func_num; i++) {
    if (function_map[i].fptr == NULL) {
      check = 1;
    } else if (callee == function_map[i].fptr) {
      index = i;
      break;
    }
  }
  if (index != -1) {
    // PRINT_FUNCTION(functions[index], callee, ">");
    function_map[index].start = (double)(clock()) / CLOCKS_PER_SEC;
  } else if (check) {
    char** p = backtrace_symbols(&callee, 1);
    for (int i = 0; i < func_num; i++) {
      if (strstr(*p, functions[i]) != NULL) {
        function_map[i].fptr = callee;
        index = i;
        break;
      }
    }
    free(p);
    if (index >= 0) {
      // PRINT_FUNCTION(functions[index], callee, ">");
      function_map[index].start = (double)(clock()) / CLOCKS_PER_SEC;
    }
  }
}

void __cyg_profile_func_exit(void* callee, void* callsite) {
  int index = -1;
  for (int i = 0; i < func_num; i++) {
    if (callee == function_map[i].fptr) {
      index = i;
      break;
    }
  }
  if (index != -1) {
    // PRINT_FUNCTION(functions[index], callee, "<");
    function_map[index].counter++;
    double stop = (double)(clock()) / CLOCKS_PER_SEC;
    function_map[index].total_seconds += stop - function_map[index].start;
  }
}

}  // extern "C"