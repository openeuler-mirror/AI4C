#ifndef AI4C_GCC_FEATURE_UTILS_H
#define AI4C_GCC_FEATURE_UTILS_H
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "basic-block.h"
#include "gcc-plugin.h"
#include "insn-attr.h"
#include "insn_utils.h"
#include "plugin_utils.h"

namespace ai4c {

class RTXInsnUtil {
 public:
  RTXInsnUtil(rtx_insn* insn) : insn_(insn) {}
  virtual ~RTXInsnUtil() = default;

  enum attr_type get_insn_type() { return get_attr_type(insn_); }

 private:
  rtx_insn* insn_;
};

class LoopUtil {
 public:
  LoopUtil(loop* loop_) : loop_(loop_) {}

  virtual ~LoopUtil() = default;

  void analyze_insns(AutoTuning& auto_tuning) {
    basic_block *bbs, bb;
    bbs = get_loop_body(loop_);
    rtx_insn* insn;
    for (unsigned i = 0; i < loop_->num_nodes; i++) {
      bb = bbs[i];
      bb_num_++;
      FOR_BB_INSNS(bb, insn) {
        if (!NONDEBUG_INSN_P(insn)) continue;

        if (!is_valid_insn_attr_type(insn)) continue;
        insn_attr_type ia_type = get_insn_attr_type(insn);

        if (is_load_op(ia_type)) {
          ld_insn_num_++;
        }

        if (is_store_op(ia_type)) st_insn_num_++;

        if (is_fp_compute_op(ia_type)) fp_compute_num_++;
      }
    }
    free(bbs);

    auto_tuning.extra_features.fs.push_back(ExtraFeature("block_num", bb_num_));
    auto_tuning.extra_features.fs.push_back(
        ExtraFeature("float_compute_insn", fp_compute_num_));
    auto_tuning.extra_features.fs.push_back(
        ExtraFeature("load_insn", ld_insn_num_));
    auto_tuning.extra_features.fs.push_back(
        ExtraFeature("store_insn", st_insn_num_));
  }

  void dump(const std::string& output_file) {
    basic_block* bbs = get_loop_body(loop_);
    dump_file = fopen(output_file.c_str(), "w");
    dump_basic_block(TDF_BLOCKS, *bbs, 0);
    free(bbs);
  }

 private:
  loop* loop_;
  int64_t bb_num_{0};
  int64_t fp_compute_num_{0};
  int64_t ld_insn_num_{0};
  int64_t st_insn_num_{0};
};

class FunctionUtil;

}  // namespace ai4c
#endif  // AI4C_GCC_FEATURE_UTILS_H