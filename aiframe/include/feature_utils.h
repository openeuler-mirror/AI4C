#ifndef AI4C_GCC_FEATURE_UTILS_H
#define AI4C_GCC_FEATURE_UTILS_H
#include "gcc-plugin.h"
#include "plugin_utils.h"
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
 
#include "basic-block.h"
#ifdef __aarch64__
#include "insn_utils_aarch64.h"
#else
#include "insn_utils_x86_64.h"
#endif

namespace ai4c {

void count_rtx_types(
    rtx x, std::unordered_map<enum rtx_code, unsigned int> &type_counts) {
  if (!x) return;

  std::stack<rtx> s;
  s.push(x);

  while (!s.empty()) {
    rtx current = s.top();
    s.pop();

    if (!current) continue;

    enum rtx_code code = GET_CODE(current);
    type_counts[code]++;

    const char *fmt = GET_RTX_FORMAT(code);
    for (int i = 0; fmt[i] != '\0'; i++) {
      if (fmt[i] == 'e') {
        s.push(XEXP(current, i));
      } else if (fmt[i] == 'E') {
        int vec_length = XVECLEN(current, i);
        for (int j = 0; j < vec_length; j++) {
          s.push(XVECEXP(current, i, j));
        }
      }
    }
  }
}

std::vector<rtx_code> get_all_rtx_codes() {
  std::vector<rtx_code> all_codes;
  for (int code = 0; code < LAST_AND_UNUSED_RTX_CODE; ++code) {
    all_codes.push_back(static_cast<rtx_code>(code));
  }
  return all_codes;
}

static std::unordered_map<enum rtx_code, unsigned int> count_loop_insn_types(
    class loop *loop) {
  basic_block *bbs, bb;
  unsigned i;
  rtx_insn *insn;

  // Create an unordered_map to store the counts of each rtx_code
  std::unordered_map<enum rtx_code, unsigned int> insn_type_counts;

  std::vector<rtx_code> all_codes = get_all_rtx_codes();
  for (const auto &code : all_codes) {
    insn_type_counts[code] = 0;
  }

  bbs = get_loop_body(loop);
  for (i = 0; i < loop->num_nodes; i++) {
    bb = bbs[i];
    FOR_BB_INSNS(bb, insn) {
      if (NONDEBUG_INSN_P(insn)) {
        count_rtx_types(PATTERN(insn), insn_type_counts);
      }
    }
  }

  free(bbs);

  return insn_type_counts;
}

class RTXInsnUtil {
 public:
  RTXInsnUtil(rtx_insn *insn) : insn_(insn) {}
  virtual ~RTXInsnUtil() = default;

 private:
  rtx_insn *insn_;
};

class LoopUtil {
 public:
  LoopUtil(class loop *loop_) : loop_(loop_) {}

  virtual ~LoopUtil() = default;

  void analyze_insns(AutoTuning &auto_tuning) {
    basic_block *bbs, bb;
    bbs = get_loop_body(loop_);
    rtx_insn *insn;
    for (unsigned i = 0; i < loop_->num_nodes; i++) {
      bb = bbs[i];
      bb_num++;
      FOR_BB_INSNS(bb, insn) {
        if (!INSN_P(insn)) continue;

        if (!NONDEBUG_INSN_P(insn)) continue;

        if (!is_valid_insn_attr_type(insn)) continue;
        insn_attr_type ia_type = get_insn_attr_type(insn);

        if (is_load_op(ia_type)) {
          ld_insn_num++;
        }

        if (is_store_op(ia_type)) st_insn_num++;

        if (is_fp_compute_op(ia_type)) fp_compute_num++;

        if (is_fadd_op(insn, ia_type)) fadd_num++;

        if (is_fcmp_op(ia_type)) fcmp_num++;

        if (is_fdiv_op(ia_type)) fdiv_num++;

        if (is_fmul_op(ia_type)) fmul_num++;

        if (is_fp2si_op(ia_type)) fp2si_num++;

        if (is_fsub_op(insn)) fsub_num++;

        if (is_ret_op(insn)) ret_num++;

        if (is_unreachable_op(ia_type)) unreachable_num++;

        if (is_branch_op(ia_type)) br_num++;

        if (is_memory_op(ia_type)) mem_num++;
      }
    }
    free(bbs);

    auto_tuning.extra_features.fs.push_back(ExtraFeature("block_num", bb_num));
    auto_tuning.extra_features.fs.push_back(
        ExtraFeature("float_compute_insn", fp_compute_num));
    auto_tuning.extra_features.fs.push_back(
        ExtraFeature("load_insn", ld_insn_num));
    auto_tuning.extra_features.fs.push_back(
        ExtraFeature("store_insn", st_insn_num));
    auto_tuning.extra_features.fs.push_back(
        ExtraFeature("fadd_insn", fadd_num));
    auto_tuning.extra_features.fs.push_back(ExtraFeature("fcmp_num", fcmp_num));
    auto_tuning.extra_features.fs.push_back(ExtraFeature("fdiv_num", fdiv_num));
    auto_tuning.extra_features.fs.push_back(ExtraFeature("fmul_num", fmul_num));
    auto_tuning.extra_features.fs.push_back(
        ExtraFeature("fp2si_num", fp2si_num));
    auto_tuning.extra_features.fs.push_back(
        ExtraFeature("fsub_insn", fsub_num));
    auto_tuning.extra_features.fs.push_back(ExtraFeature("ret_insn", ret_num));
    auto_tuning.extra_features.fs.push_back(
        ExtraFeature("unreachable_insn", unreachable_num));
    auto_tuning.extra_features.fs.push_back(ExtraFeature("br_insn", br_num));
    auto_tuning.extra_features.fs.push_back(ExtraFeature("mem_insn", mem_num));
  }

  std::vector<float> analyze_insns() {
    std::vector<float> vec;
    if (loop_->simple_loop_desc) {
      niter = loop_->simple_loop_desc->niter;
    } else {
      niter = 0;
    }
    if (loop_->superloops) {
      depth = loop_->superloops->length();
    } else {
      depth = 1;
    }

    vec.push_back(static_cast<float>(loop_->lpt_decision.times));
    vec.push_back(static_cast<float>(loop_->ninsns));
    vec.push_back(static_cast<float>(loop_->lpt_decision.decision));
    vec.push_back(niter);
    vec.push_back(depth);
    std::unordered_set<std::string> parm_set = {
        "vec_select", "mult", "neg",       "minus",  "plus",    "mem",
        "subreg",     "reg",  "const_int", "unspec", "clobber", "sign_extend"};

    for (const auto &entry : res) {
      enum rtx_code code = entry.first;
      unsigned int count = entry.second;
      if (parm_set.find(GET_RTX_NAME(code)) != parm_set.end()) {
        vec.push_back(static_cast<float>(count));
      }
    }
    return vec;
  }

  void dump(const std::string &output_file) {
    basic_block *bbs = get_loop_body(loop_);
    dump_file = fopen(output_file.c_str(), "w");
    dump_basic_block(TDF_BLOCKS, *bbs, 0);
    free(bbs);
  }

 private:
  class loop *loop_;
  int64_t bb_num{0};
  int64_t fp_compute_num{0};
  int64_t ld_insn_num{0};
  int64_t st_insn_num{0};
  int64_t fadd_num{0};
  int64_t fcmp_num{0};
  int64_t fdiv_num{0};
  int64_t fmul_num{0};
  int64_t fp2si_num{0};
  int64_t fsub_num{0};
  int64_t ret_num{0};
  int64_t unreachable_num{0};
  int64_t br_num{0};
  int64_t mem_num{0};
  float niter{0};
  float depth{1};
  std::unordered_map<enum rtx_code, unsigned int> res =
      count_loop_insn_types(loop_);
};

class FunctionUtil;

}  // namespace ai4c
#endif  // AI4C_GCC_FEATURE_UTILS_H
