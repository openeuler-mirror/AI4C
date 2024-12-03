#include "gcc-plugin.h"
#include "plugin-version.h"
#include "tree-pass.h"
#include "cfgloop.h"
#include "context.h"
#include "dumpfile.h"
#include "tree.h"
#include "basic-block.h"
#include "gimple.h"
#include "rtl.h"
#include "insn-attr.h"
#include "plugin_utils.h"
#include "feature_utils.h"

int plugin_is_GPL_compatible;

using namespace ai4c;

std::string config_path{};

/* ===------------------------- Generate -------------------------=== */
const pass_data manual_unroll_generate_pass_data = {
    .type = RTL_PASS,  // GIMPLE_PASS, RTL_PASS, SIMPLE_IPA_PASS, IPA_PASS
    .name = "manual_unroll_generate_pass",
    .optinfo_flags = OPTGROUP_NONE,
    .tv_id = TV_NONE,
    .properties_required = PROP_gimple_any,
    .properties_provided = 0,
    .properties_destroyed = 0,
    .todo_flags_start = 0,
    .todo_flags_finish = 0};

struct manual_unroll_generate_pass : rtl_opt_pass {
 public:
  manual_unroll_generate_pass()
      : rtl_opt_pass(manual_unroll_generate_pass_data, g) {}

  virtual unsigned int execute(function* fun) override {
    const char* func_name = function_name(fun);
    for (auto loop : loops_list(fun, LI_FROM_INNERMOST)) {
      // loop cannot unroll
      if (loop->lpt_decision.decision == LPT_NONE) continue;

      dump_user_location_t locus = get_loop_location(loop);
      const location_t loc = locus.get_location_t();

      AutoTuning auto_tuning;
      auto_tuning.pass = "loop2_unroll";
      auto_tuning.name = func_name;
      auto_tuning.code_region_type = "loop";
      auto_tuning.invocation = loop->num;
      auto_tuning.code_region_hash =
          codeRegionHash(loc, func_name, loop->header->discriminator);
      auto_tuning.debug_loc.file = LOCATION_FILE(loc);
      auto_tuning.debug_loc.line = LOCATION_LINE(loc);
      auto_tuning.debug_loc.column = LOCATION_COLUMN(loc);
      auto_tuning.function = func_name;

      std::vector<int> unroll_values = {1, 2, 4, 8, 16, 32};
      OptionManager opt_mgr;
      opt_mgr.add_fine_grained_option("Uint", "FINE_GRAINED", "UnrollCount",
                                      unroll_values, loop->unroll);

      auto util = ai4c::LoopUtil(loop);
      util.analyze_insns(auto_tuning);

      std::string yaml_name =
          std::filesystem::path(LOCATION_FILE(loc)).filename();
      std::string opp_yaml_file = config_path + "/" + yaml_name + ".yaml";
      dump_opp_yaml(opp_yaml_file, auto_tuning, opt_mgr);
    }
    return 0;
  }

  virtual manual_unroll_generate_pass* clone() override { return this; }
};

struct register_pass_info manual_unroll_generate_passinfo {
  .pass = new manual_unroll_generate_pass(),
  .reference_pass_name = "loop2_unroll",
  // 0-each time call cfg-pass, plugin will be called
      .ref_pass_instance_number = 0,
  // PASS_POS_INSERT_AFTER, PASS_POS_INSERT_BEFORE, PASS_POS_REPLACE
      .pos_op = PASS_POS_INSERT_AFTER
};
/* ===------------------------- Generate -------------------------=== */

/* ===------------------------- Auto-Tuning -------------------------=== */
const pass_data manual_unroll_pass_data = {
    .type = RTL_PASS,  // GIMPLE_PASS, RTL_PASS, SIMPLE_IPA_PASS, IPA_PASS
    .name = "manual_unroll_pass",
    .optinfo_flags = OPTGROUP_NONE,
    .tv_id = TV_NONE,
    .properties_required = PROP_gimple_any,
    .properties_provided = 0,
    .properties_destroyed = 0,
    .todo_flags_start = 0,
    .todo_flags_finish = 0};

struct manual_unroll_pass : rtl_opt_pass {
 public:
  manual_unroll_pass() : rtl_opt_pass(manual_unroll_pass_data, g) {}

  virtual unsigned int execute(function* fun) override {
    const char* func_name = function_name(fun);
    for (auto loop : loops_list(fun, LI_FROM_INNERMOST)) {
      dump_user_location_t locus = get_loop_location(loop);
      const location_t loc = locus.get_location_t();
      size_t hash = codeRegionHash(loc, func_name, loop->header->discriminator);
      if (AutoTuneOptions.find(hash) != AutoTuneOptions.end()) {
        loop->unroll = AutoTuneOptions[hash].args["UnrollCount"];
      }
    }
    return 0;
  }

  virtual manual_unroll_pass* clone() override { return this; }
};

struct register_pass_info manual_unroll_passinfo {
  .pass = new manual_unroll_pass(), .reference_pass_name = "loop2_unroll",
  .ref_pass_instance_number = 0, .pos_op = PASS_POS_INSERT_BEFORE
};
/* ===------------------------- Auto-Tuning -------------------------=== */

int plugin_init(struct plugin_name_args* plugin_info,
                struct plugin_gcc_version* version) {
  if (!plugin_default_version_check(version, &gcc_version)) {
    printf("incompatible gcc/plugin versions\n");
    return 1;
  }

  const char* const plugin_name = plugin_info->base_name;

  TuneMode mode{TuneMode::Unknown};

  for (int i = 0; i < plugin_info->argc; i++) {
    std::string key(plugin_info->argv[i].key);
    std::string value(plugin_info->argv[i].value);
    if (TuneOption.find(key) != TuneOption.end()) {
      mode = TuneOption[key];
    }
    if (std::strcmp(plugin_info->argv[i].key, "generate") == 0) {
      mode = TuneMode::Generate;
      config_path = std::string(plugin_info->argv[i].value);
    } else if (std::strcmp(plugin_info->argv[i].key, "autotune") == 0) {
      mode = TuneMode::Tuning;
      config_path = std::string(plugin_info->argv[i].value);
      load_config_yaml(config_path);
    }
  }

  switch (mode) {
    case TuneMode::Tuning:
      register_callback(plugin_name, PLUGIN_PASS_MANAGER_SETUP, NULL,
                        &manual_unroll_passinfo);
      break;
    case TuneMode::Generate:
      register_callback(plugin_name, PLUGIN_PASS_MANAGER_SETUP, NULL,
                        &manual_unroll_generate_passinfo);
      break;
    default:
      printf("!!! Unknown tuning mode\n");
      break;
  }
  return 0;
}
