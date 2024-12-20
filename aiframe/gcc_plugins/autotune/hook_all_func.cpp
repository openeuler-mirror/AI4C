#include <dlfcn.h>
#include <execinfo.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

extern "C" {

typedef struct {
  void* fptr;
  char* func_name;
  unsigned int counter;
  double start;
  double total_seconds;
} FunctionMap;

static int func_num = -1;
FunctionMap* function_map = NULL;

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
  function_map = (FunctionMap*)malloc(1 * sizeof(FunctionMap));
}

void __attribute__((destructor)) stop(void) {
  if (fp_trace != NULL) {
    for (int i = 0; i < func_num; i++) {
      fprintf(fp_trace, "%s,%d,%f\n", function_map[i].func_name,
              function_map[i].counter, function_map[i].total_seconds);
    }
    fclose(fp_trace);
  }
}

void __cyg_profile_func_exit(void* callee, void* callsite)
    __attribute__((no_instrument_function));

void __cyg_profile_func_enter(void* callee, void* callsite)
    __attribute__((no_instrument_function));

void __cyg_profile_func_enter(void* callee, void* callsite) {
  if (!fp_trace) return;
  int index = -1;
  for (int i = 0; i < func_num; i++) {
    if (callee == function_map[i].fptr) {
      index = i;
      break;
    }
  }
  if (index != -1) {
    function_map[index].start = (double)(clock()) / CLOCKS_PER_SEC;
  } else {
    char** p = backtrace_symbols(&callee, 20);
    for (int i = 0; i < func_num; i++) {
      if (strstr(*p, function_map[i].func_name) != NULL) {
        function_map[i].fptr = callee;
        index = i;
        break;
      }
    }
    if (index == -1) {
      if (func_num == -1) {
        func_num = 1;
      } else {
        func_num++;
        FunctionMap* new_function_map =
            (FunctionMap*)realloc(function_map, func_num * sizeof(FunctionMap));
        function_map = new_function_map;
      }
      index = func_num - 1;
    }
    function_map[index].func_name = (char*)malloc(strlen(*p) + 1);
    memcpy(function_map[index].func_name, *p, strlen(*p) + 1);
    function_map[index].fptr = callee;
    function_map[index].start = (double)(clock()) / CLOCKS_PER_SEC;
    free(p);
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
    function_map[index].counter++;
    double stop = (double)(clock()) / CLOCKS_PER_SEC;
    function_map[index].total_seconds += stop - function_map[index].start;
  }
}

}  // extern "C"