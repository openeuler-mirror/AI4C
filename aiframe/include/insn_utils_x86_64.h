#ifndef AI4C_GCC_INSN_UTILS_X86_64_H
#define AI4C_GCC_INSN_UTILS_X86_64_H
#include "basic-block.h"
#include "insn-attr.h"

namespace ai4c {

enum precision { P_UNK };

enum use_type { UT_UNK };

enum calc_type { CT_UNK };

enum insn_set { IS_UNK };

enum insn_gentype { IG_UNK };

enum arith_type { AT_UNK };

struct insn_attr_type {
  int name;
  precision prec;
  use_type use;
  calc_type calc;
  insn_set set;
  insn_gentype gentype;
  arith_type arith;
};

static bool is_valid_insn_attr_type(rtx_insn* insn) { return false; }

static insn_attr_type get_insn_attr_type(rtx_insn* insn) {
  insn_attr_type ia_type;
  return ia_type;
}

static bool is_fp_compute_op(insn_attr_type iatype) { return false; }

static bool is_branch_op(insn_attr_type iatype) { return false; }

static bool is_intrinsic_op(insn_attr_type iatype) { return false; }

static bool is_fcmp_op(insn_attr_type iatype) { return false; }

static bool is_fdiv_op(insn_attr_type iatype) { return false; }

static bool is_fmul_op(insn_attr_type iatype) { return false; }

static bool is_fp2si_op(insn_attr_type iatype) { return false; }

static bool is_vect_op(insn_attr_type iatype) { return false; }

static bool is_unreachable_op(insn_attr_type iatype) { return false; }

static bool is_load_op(insn_attr_type iatype) { return false; }

static bool is_store_op(insn_attr_type iatype) { return false; }

static bool is_memory_op(insn_attr_type iatype) { return false; }

static bool is_implicit_op(rtx_insn* insn, insn_attr_type iatype) {
  return false;
}

static bool is_fadd_op(rtx_insn* insn, insn_attr_type iatype) { return false; }

static bool is_fsub_op(rtx_insn* insn) { return false; }

static bool is_ret_op(rtx_insn* insn) { return false; }

}  // namespace ai4c
#endif  // AI4C_GCC_INSN_UTILS_X86_64_H
