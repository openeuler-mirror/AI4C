#include <unistd.h>
#include <dlfcn.h>

#include "gcc-plugin.h"
#include "plugin-version.h"
#include "tree-pass.h"
#include "cfgloop.h"
#include "context.h"
#include "plugin_utils.h"

int plugin_is_GPL_compatible;

char* g_model_path;
char* g_infer_path;

/* Interfaces of AI4C Inference Engine */
void* g_infer_handle;

typedef void (*init_engine_t)(const char*);
init_engine_t initialize;

typedef void (*add_int64_input_t)(int64_t*, int);
add_int64_input_t add_int64_input;

typedef void (*add_int32_input_t)(int32_t*, int);
add_int32_input_t add_int32_input;

typedef void (*add_float_input_t)(float*, int);
add_float_input_t add_float_input;

typedef void (*add_string_input_t)(const char**, int);
add_string_input_t add_string_input;

typedef int (*inference_t)();
inference_t inference;

typedef int32_t* (*get_int32_output_t)(int);
get_int32_output_t get_int32_output;

typedef int64_t* (*get_int64_output_t)(int);
get_int64_output_t get_int64_output;

typedef float* (*get_float_output_t)(int);
get_float_output_t get_float_output;

typedef void (*free_engine_t)();
free_engine_t free_engine;

const pass_data rtl_unroll_opt_pass_data = {.type = RTL_PASS,
                                            .name = "rtl_unroll_opt_pass",
                                            .optinfo_flags = OPTGROUP_NONE,
                                            .tv_id = TV_NONE,
                                            .properties_required = 0,
                                            .properties_provided = 0,
                                            .properties_destroyed = 0,
                                            .todo_flags_start = 0,
                                            .todo_flags_finish = 0};

struct rtl_unroll_opt_pass : rtl_opt_pass {
 public:
  rtl_unroll_opt_pass() : rtl_opt_pass(rtl_unroll_opt_pass_data, g) {}

  virtual unsigned int execute(function* fun) override {
    initialize(g_model_path);

    // Get input feature {in1} via `function* fun`
    int64_t in1[128];
    add_int64_input(in1, 128);

    int err = inference();
    if (err) return err;

    int64_t* result = get_int64_output(0);

    // Use result to optimize fun params
    printf("Result --> [%ld]\n", result[0]);

    // Release the inference engine via `fress_engine()`
    // when the model inference occupies too much memory.
    // free_engine();
    return 0;
  }

  virtual rtl_unroll_opt_pass* clone() override { return this; }
};

struct register_pass_info rtl_unroll_opt_passinfo {
  .pass = new rtl_unroll_opt_pass(), .reference_pass_name = "loop2_unroll",
  .ref_pass_instance_number = 0, .pos_op = PASS_POS_INSERT_BEFORE
};

int plugin_init(struct plugin_name_args* plugin_info,
                struct plugin_gcc_version* version) {
  if (!plugin_default_version_check(version, &gcc_version)) {
    printf("incompatible gcc/plugin versions\n");
    return 1;
  }

  for (int i = 0; i < plugin_info->argc; i++) {
    std::string key(plugin_info->argv[i].key);
    std::string value(plugin_info->argv[i].value);
    if (std::strcmp(plugin_info->argv[i].key, "model") == 0) {
      g_model_path = plugin_info->argv[i].value;
    } else if (std::strcmp(plugin_info->argv[i].key, "engine") == 0) {
      g_infer_path = plugin_info->argv[i].value;
    }
  }

  if (!g_model_path || access(g_model_path, F_OK)) {
    fprintf(stderr, "Model '%s' not found\n", g_model_path);
    return -1;
  }

  g_infer_handle = dlopen(g_infer_path, RTLD_LAZY);
  if (!g_infer_handle) {
    fprintf(stderr, "%s\n", dlerror());
    return -1;
  }

  initialize = (init_engine_t)dlsym(g_infer_handle, "initialize");
  add_int64_input = (add_int64_input_t)dlsym(g_infer_handle, "add_int64_input");
  inference = (inference_t)dlsym(g_infer_handle, "inference");
  get_int64_output =
      (get_int64_output_t)dlsym(g_infer_handle, "get_int64_output");
  free_engine = (free_engine_t)dlsym(g_infer_handle, "free_engine");

  if (!initialize || !add_int64_input || !inference || !get_int64_output ||
      !free_engine) {
    fprintf(stderr, "Invalid inference engine '%s'\n", g_infer_path);
    return -1;
  }

  dlclose(g_infer_handle);

  const char* const plugin_name = plugin_info->base_name;
  register_callback(plugin_name, PLUGIN_PASS_MANAGER_SETUP, NULL,
                    &rtl_unroll_opt_passinfo);
  return 0;
}
