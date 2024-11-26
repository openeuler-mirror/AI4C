#include "gcc-plugin.h"
#include "plugin-version.h"
#include "tree-pass.h"
#include "context.h"
#include "config.h"
#include "system.h"
#include "tree.h"
#include "cgraph.h"
#include "plugin_utils.h"
#include "option_utils.h"

int plugin_is_GPL_compatible;

using namespace ai4c;

std::string config_path{};
OptionManager opt_mgr;

/* ===------------------------- Generate -------------------------=== */
const pass_data pass_data_coarse_option_generate = {
  .type = GIMPLE_PASS,  // GIMPLE_PASS, RTL_PASS, SIMPLE_IPA_PASS, IPA_PASS
  .name = "coarse_option_generate",
  .optinfo_flags = OPTGROUP_NONE,
  .tv_id = TV_NONE,
  .properties_required = PROP_gimple_any,
  .properties_provided = 0,
  .properties_destroyed = 0,
  .todo_flags_start = 0,
  .todo_flags_finish = 0
};

struct coarse_option_generate_pass : gimple_opt_pass {
 public:
  coarse_option_generate_pass()
      : gimple_opt_pass(pass_data_coarse_option_generate, g) {}

  virtual unsigned int execute(function *fun) override {
    const char *func_name = function_name(fun);
    printf("func_name: %s\n", func_name);
    location_t loc = DECL_SOURCE_LOCATION(fun->decl);

    AutoTuning auto_tuning;
    auto_tuning.pass = "coarse_option_generate";
    auto_tuning.name = func_name;
    auto_tuning.code_region_type = "function";
    auto_tuning.code_region_hash = codeRegionHash(loc, func_name);
    auto_tuning.debug_loc.file = LOCATION_FILE(loc);
    auto_tuning.function = func_name;

    std::string yaml_name =
        std::filesystem::path(LOCATION_FILE(loc)).filename();
    std::string opp_yaml_file = config_path + "/" + yaml_name + ".yaml";
    dump_opp_yaml(opp_yaml_file, auto_tuning, opt_mgr);

    return 0;
  }

  virtual coarse_option_generate_pass *clone() override { return this; }
};

struct register_pass_info coarse_option_generate_passinfo {
  .pass = new coarse_option_generate_pass(),
  .reference_pass_name = "*warn_unused_result",
  .ref_pass_instance_number = 0,
  .pos_op = PASS_POS_INSERT_BEFORE
};
/* ===------------------------- Generate -------------------------=== */

/* ===------------------------- Auto-Tuning -------------------------=== */
const pass_data pass_data_coarse_option_tuning = {
  .type = GIMPLE_PASS,  // GIMPLE_PASS, RTL_PASS, SIMPLE_IPA_PASS, IPA_PASS
  .name = "coarse_option_tuning",
  .optinfo_flags = OPTGROUP_NONE,
  .tv_id = TV_NONE,
  .properties_required = PROP_gimple_any,
  .properties_provided = 0,
  .properties_destroyed = 0,
  .todo_flags_start = 0,
  .todo_flags_finish = 0
};

struct coarse_option_tuning_pass : gimple_opt_pass {
 public:
  coarse_option_tuning_pass()
      : gimple_opt_pass(pass_data_coarse_option_tuning, g) {}

  virtual unsigned int execute(function *fun) override {
    const char *func_name = function_name(fun);
    location_t loc = DECL_SOURCE_LOCATION(fun->decl);
    size_t hash = codeRegionHash(loc, func_name);

    if (AutoTuneOptions.count(hash)) {
      struct cgraph_node *node = cgraph_node::get(current_function_decl);
      struct gcc_options opts = global_options;
      struct gcc_options opts_set = global_options_set;

      cl_optimization_restore(&opts, &opts_set, opts_for_fn(node->decl));

      std::map<std::string, int> opt_idx_map;
      for (int i = 0; i < int_opt_indices.size(); ++i) {
        opt_idx_map[int_opt_indices[i]] = i;
      }
      for (const auto &arg_pair : AutoTuneOptions[hash].args) {
        std::string key = arg_pair.first;
        int value = arg_pair.second;
        update(&opts, &opts_set, key, value, opt_idx_map);
      }

      DECL_FUNCTION_SPECIFIC_OPTIMIZATION(node->decl) =
          build_optimization_node(&opts, &opts_set);
    }
    return 0;
  }

  virtual coarse_option_tuning_pass *clone() override { return this; }
};

struct register_pass_info coarse_option_tuning_passinfo {
  .pass = new coarse_option_tuning_pass(),
  .reference_pass_name = "*warn_unused_result",
  .ref_pass_instance_number = 0,
  .pos_op = PASS_POS_INSERT_BEFORE
};
/* ===------------------------- Auto-Tuning -------------------------=== */

int plugin_init(struct plugin_name_args *plugin_info,
                struct plugin_gcc_version *version) {
  if (!plugin_default_version_check(version, &gcc_version)) {
    printf("incompatible gcc/plugin versions\n");
    return 1;
  }

  const char *const plugin_name = plugin_info->base_name;

  TuneMode mode{TuneMode::Unknown};

  for (int i = 0; i < plugin_info->argc; i++) {
    std::string key(plugin_info->argv[i].key);
    std::string value(plugin_info->argv[i].value);
    if (TuneOption.find(key) != TuneOption.end()) {
      mode = TuneOption[key];
    }
    if (key == "generate") {
      config_path = std::string(value);
      mode = TuneMode::Generate;
    } else if (key == "autotune") {
      config_path = std::string(value);
      mode = TuneMode::Tuning;
      load_config_yaml(config_path);
    } else if (key == "yaml") {
      std::string yaml_file = std::string(value);
      std::filesystem::path file_path_tmp(yaml_file);
      if (!std::filesystem::exists(file_path_tmp)) {
        printf("cannot find yaml file: %s\n", yaml_file.c_str());
        return 1;
      }
      load_coarse_option_yaml(yaml_file, opt_mgr);
    }
  }

  switch (mode) {
    case TuneMode::Tuning:
      register_callback(plugin_name, PLUGIN_PASS_MANAGER_SETUP, NULL,
                        &coarse_option_tuning_passinfo);
      break;
    case TuneMode::Generate:
      register_callback(plugin_name, PLUGIN_PASS_MANAGER_SETUP, NULL,
                        &coarse_option_generate_passinfo);
      break;
    default:
      break;
  }
  return 0;
}
