#ifndef AI4C_GCC_INSN_UTILS_AARCH64_H
#define AI4C_GCC_INSN_UTILS_AARCH64_H
#include "basic-block.h"
#include "insn-attr.h"
#include "insn-codes.h"
#include "insn-config.h"
#include "recog.h"

namespace ai4c {

// NEXT: Only very few SVE/SVE2 instructions are with attribute types.
// NEXT: attribute possible: SIGNED/UNSIGNED

enum precision {
  P_UNK,
  SINGLE,
  DOUBLE,
};

enum use_type {
  UT_UNK,
  MEMORY,
  COMPUTE,
};

enum calc_type {
  CT_UNK,
  INT,
  FP,
};

enum insn_set {
  IS_UNK,
  BASE,
  NEON,
  MVE,
  SVE,
  SVE2,
};

enum insn_gentype {
  IG_UNK,
  ADR,
  ARITH,
  BITFIELD,
  BRANCH,
  CALL,
  CMP,
  CRYPTO,
  EXTEND,
  FP_ARITH,
  FP_CVT,
  FP_INTRINSIC,
  JMP,
  LOAD,
  LOGIC,
  MOV,
  NOP,
  SHIFT,
  STORE,
  TRAP,
};

enum arith_type {
  AT_UNK,
  ADD,
  SUB,
  MUL,
  DIV,
  ABS,
  NEG,
  MINMAX,
  ROUND,
  MLA,
  SQRT
};

struct insn_attr_type {
  attr_type name;
  precision prec;
  use_type use;
  calc_type calc;
  insn_set set;
  insn_gentype gentype;
  arith_type arith;
};

static std::vector<struct insn_attr_type> iat_table = {
    {TYPE_ADC_IMM, P_UNK, COMPUTE, INT, BASE, ARITH, ADD},
    {TYPE_ADC_REG, P_UNK, COMPUTE, INT, BASE, ARITH, ADD},
    {TYPE_ADCS_IMM, P_UNK, COMPUTE, INT, BASE, ARITH, ADD},
    {TYPE_ADCS_REG, P_UNK, COMPUTE, INT, BASE, ARITH, ADD},
    {TYPE_ADR, P_UNK, UT_UNK, CT_UNK, BASE, ADR, AT_UNK},
    {TYPE_ALU_EXT, P_UNK, COMPUTE, INT, BASE, ARITH, AT_UNK},
    {TYPE_ALU_IMM, P_UNK, COMPUTE, INT, BASE, ARITH, AT_UNK},
    {TYPE_ALU_SREG, P_UNK, COMPUTE, INT, BASE, ARITH, AT_UNK},
    {TYPE_ALU_SHIFT_IMM_LSL_1TO4, P_UNK, COMPUTE, INT, BASE, ARITH, AT_UNK},
    {TYPE_ALU_SHIFT_IMM_OTHER, P_UNK, COMPUTE, INT, BASE, ARITH, AT_UNK},
    {TYPE_ALU_SHIFT_REG, P_UNK, COMPUTE, INT, BASE, ARITH, AT_UNK},
    {TYPE_ALU_DSP_REG, P_UNK, COMPUTE, INT, BASE, ARITH, AT_UNK},
    {TYPE_ALUS_EXT, P_UNK, COMPUTE, INT, BASE, ARITH, AT_UNK},
    {TYPE_ALUS_IMM, P_UNK, COMPUTE, INT, BASE, ARITH, AT_UNK},
    {TYPE_ALUS_SREG, P_UNK, COMPUTE, INT, BASE, ARITH, AT_UNK},
    {TYPE_ALUS_SHIFT_IMM, P_UNK, COMPUTE, INT, BASE, ARITH, AT_UNK},
    {TYPE_ALUS_SHIFT_REG, P_UNK, COMPUTE, INT, BASE, ARITH, AT_UNK},
    {TYPE_BFM, P_UNK, COMPUTE, INT, BASE, BITFIELD, AT_UNK}, /* compute? */
    {TYPE_BFX, P_UNK, COMPUTE, INT, BASE, BITFIELD, AT_UNK},
    {TYPE_BLOCK, P_UNK, UT_UNK, CT_UNK, BASE, IG_UNK, AT_UNK},
    {TYPE_BRANCH, P_UNK, UT_UNK, CT_UNK, BASE, BRANCH, AT_UNK},
    {TYPE_CALL, P_UNK, UT_UNK, CT_UNK, BASE, CALL, AT_UNK},
    {TYPE_CLZ, P_UNK, COMPUTE, INT, BASE, BITFIELD, AT_UNK},
    {TYPE_NO_INSN, P_UNK, UT_UNK, CT_UNK, BASE, NOP, AT_UNK},
    {TYPE_CSEL, P_UNK, MEMORY, INT, BASE, MOV, AT_UNK},
    {TYPE_CRC, P_UNK, COMPUTE, INT, BASE, IG_UNK, AT_UNK},
    {TYPE_EXTEND, P_UNK, COMPUTE, INT, BASE, EXTEND, AT_UNK},
    {TYPE_F_CVT, P_UNK, COMPUTE, FP, BASE, FP_CVT, AT_UNK},
    {TYPE_F_CVTF2I, P_UNK, COMPUTE, FP, BASE, FP_CVT, AT_UNK},
    {TYPE_F_CVTI2F, P_UNK, COMPUTE, FP, BASE, FP_CVT, AT_UNK},
    {TYPE_F_FLAG, P_UNK, UT_UNK, FP, BASE, IG_UNK, AT_UNK},
    {TYPE_F_LOADD, DOUBLE, MEMORY, FP, BASE, LOAD, AT_UNK},
    {TYPE_F_LOADS, SINGLE, MEMORY, FP, BASE, LOAD, AT_UNK},
    {TYPE_F_MCR, P_UNK, MEMORY, FP, BASE, MOV, AT_UNK},
    {TYPE_F_MCRR, P_UNK, MEMORY, FP, BASE, MOV, AT_UNK},
    {TYPE_F_MINMAXD, DOUBLE, COMPUTE, FP, BASE, FP_ARITH,
     MINMAX}, /* NEXT: how to distinguish MINMAX and CSEL (memory/compute,
                 mov/arith) */
    {TYPE_F_MINMAXS, SINGLE, COMPUTE, FP, BASE, FP_ARITH, MINMAX},
    {TYPE_F_MRC, P_UNK, MEMORY, FP, BASE, MOV, AT_UNK},
    {TYPE_F_MRRC, P_UNK, MEMORY, FP, BASE, MOV, AT_UNK},
    {TYPE_F_RINTD, DOUBLE, COMPUTE, FP, BASE, FP_INTRINSIC, ROUND},
    {TYPE_F_RINTS, SINGLE, COMPUTE, FP, BASE, FP_INTRINSIC, ROUND},
    {TYPE_F_STORED, DOUBLE, MEMORY, FP, BASE, STORE, AT_UNK},
    {TYPE_F_STORES, SINGLE, MEMORY, FP, BASE, STORE, AT_UNK},
    {TYPE_FADDD, DOUBLE, COMPUTE, FP, BASE, FP_ARITH, ADD},
    {TYPE_FADDS, SINGLE, COMPUTE, FP, BASE, FP_ARITH, ADD},
    {TYPE_FCCMPD, DOUBLE, COMPUTE, FP, BASE, CMP, AT_UNK},
    {TYPE_FCCMPS, SINGLE, COMPUTE, FP, BASE, CMP, AT_UNK},
    {TYPE_FCMPD, DOUBLE, COMPUTE, FP, BASE, CMP, AT_UNK},
    {TYPE_FCMPS, SINGLE, COMPUTE, FP, BASE, CMP, AT_UNK},
    {TYPE_FCONSTD, P_UNK, COMPUTE, FP, BASE, FP_ARITH, AT_UNK},
    {TYPE_FCONSTS, P_UNK, COMPUTE, FP, BASE, FP_ARITH, AT_UNK},
    {TYPE_FCSEL, P_UNK, MEMORY, FP, BASE, MOV, AT_UNK},
    {TYPE_FDIVD, DOUBLE, COMPUTE, FP, BASE, FP_ARITH, DIV},
    {TYPE_FDIVS, SINGLE, COMPUTE, FP, BASE, FP_ARITH, DIV},
    {TYPE_FFARITHD, DOUBLE, COMPUTE, FP, BASE, FP_ARITH, AT_UNK},
    {TYPE_FFARITHS, SINGLE, COMPUTE, FP, BASE, FP_ARITH, AT_UNK},
    {TYPE_FFMAD, DOUBLE, COMPUTE, FP, BASE, FP_INTRINSIC, MLA},
    {TYPE_FFMAS, SINGLE, COMPUTE, FP, BASE, FP_INTRINSIC, MLA},
    {TYPE_FLOAT, P_UNK, UT_UNK, FP, BASE, IG_UNK, AT_UNK},
    {TYPE_FMACD, DOUBLE, COMPUTE, FP, BASE, FP_INTRINSIC, MLA},
    {TYPE_FMACS, SINGLE, COMPUTE, FP, BASE, FP_INTRINSIC, MLA},
    {TYPE_FMOV, P_UNK, MEMORY, FP, BASE, MOV, AT_UNK},
    {TYPE_FMULD, DOUBLE, COMPUTE, FP, BASE, FP_ARITH, MUL},
    {TYPE_FMULS, SINGLE, COMPUTE, FP, BASE, FP_ARITH, MUL},
    {TYPE_FSQRTS, DOUBLE, COMPUTE, FP, BASE, FP_ARITH, SQRT},
    {TYPE_FSQRTD, SINGLE, COMPUTE, FP, BASE, FP_ARITH, SQRT},
    {TYPE_LOAD_ACQ, P_UNK, MEMORY, INT, BASE, LOAD, AT_UNK},
    {TYPE_LOAD_BYTE, P_UNK, MEMORY, INT, BASE, LOAD, AT_UNK},
    {TYPE_LOAD_4, P_UNK, MEMORY, INT, BASE, LOAD, AT_UNK},
    {TYPE_LOAD_8, P_UNK, MEMORY, INT, BASE, LOAD, AT_UNK},
    {TYPE_LOAD_12, P_UNK, MEMORY, INT, BASE, LOAD, AT_UNK},
    {TYPE_LOAD_16, P_UNK, MEMORY, INT, BASE, LOAD, AT_UNK},
    {TYPE_LOGIC_IMM, P_UNK, UT_UNK, INT, BASE, LOGIC, AT_UNK}, /* compute? */
    {TYPE_LOGIC_REG, P_UNK, UT_UNK, INT, BASE, LOGIC, AT_UNK},
    {TYPE_LOGIC_SHIFT_IMM, P_UNK, UT_UNK, INT, BASE, LOGIC, AT_UNK},
    {TYPE_LOGIC_SHIFT_REG, P_UNK, UT_UNK, INT, BASE, LOGIC, AT_UNK},
    {TYPE_LOGICS_IMM, P_UNK, UT_UNK, INT, BASE, LOGIC, AT_UNK},
    {TYPE_LOGICS_REG, P_UNK, UT_UNK, INT, BASE, LOGIC, AT_UNK},
    {TYPE_LOGICS_SHIFT_IMM, P_UNK, UT_UNK, INT, BASE, LOGIC, AT_UNK},
    {TYPE_LOGICS_SHIFT_REG, P_UNK, UT_UNK, INT, BASE, LOGIC, AT_UNK},
    {TYPE_MLA, P_UNK, COMPUTE, INT, BASE, ARITH, MLA},
    {TYPE_MLAS, P_UNK, COMPUTE, INT, BASE, ARITH, MLA},
    {TYPE_MOV_IMM, P_UNK, MEMORY, INT, BASE, MOV, AT_UNK},
    {TYPE_MOV_REG, P_UNK, MEMORY, INT, BASE, MOV, AT_UNK},
    {TYPE_MOV_SHIFT, P_UNK, MEMORY, INT, BASE, MOV, AT_UNK},
    {TYPE_MOV_SHIFT_REG, P_UNK, MEMORY, INT, BASE, MOV, AT_UNK},
    {TYPE_MRS, P_UNK, MEMORY, INT, BASE, MOV, AT_UNK},
    {TYPE_MUL, P_UNK, COMPUTE, INT, BASE, ARITH, MUL},
    {TYPE_MULS, P_UNK, COMPUTE, INT, BASE, ARITH, MUL},
    {TYPE_MULTIPLE, P_UNK, COMPUTE, INT, BASE, ARITH, MUL},
    {TYPE_MVN_IMM, P_UNK, MEMORY, INT, BASE, MOV, AT_UNK},
    {TYPE_MVN_REG, P_UNK, MEMORY, INT, BASE, MOV, AT_UNK},
    {TYPE_MVN_SHIFT, P_UNK, MEMORY, INT, BASE, MOV, AT_UNK},
    {TYPE_MVN_SHIFT_REG, P_UNK, MEMORY, INT, BASE, MOV, AT_UNK},
    {TYPE_NOP, P_UNK, UT_UNK, INT, BASE, NOP, AT_UNK},
    {TYPE_RBIT, P_UNK, COMPUTE, INT, BASE, BITFIELD, AT_UNK},
    {TYPE_REV, P_UNK, COMPUTE, INT, BASE, BITFIELD, AT_UNK},
    {TYPE_ROTATE_IMM, P_UNK, COMPUTE, INT, BASE, BITFIELD, AT_UNK},
    {TYPE_SDIV, P_UNK, COMPUTE, INT, BASE, ARITH, DIV},
    {TYPE_SHIFT_IMM, P_UNK, COMPUTE, INT, BASE, BITFIELD, AT_UNK},
    {TYPE_SHIFT_REG, P_UNK, COMPUTE, INT, BASE, BITFIELD, AT_UNK},
    {TYPE_SMLAD, P_UNK, COMPUTE, INT, BASE, ARITH, MLA},
    {TYPE_SMLADX, P_UNK, COMPUTE, INT, BASE, ARITH, MLA},
    {TYPE_SMLAL, P_UNK, COMPUTE, INT, BASE, ARITH, MLA},
    {TYPE_SMLALD, P_UNK, COMPUTE, INT, BASE, ARITH, MLA},
    {TYPE_SMLALS, P_UNK, COMPUTE, INT, BASE, ARITH, MLA},
    {TYPE_SMLALXY, P_UNK, COMPUTE, INT, BASE, ARITH, MLA},
    {TYPE_SMLAWX, P_UNK, COMPUTE, INT, BASE, ARITH, MLA},
    {TYPE_SMLAWY, P_UNK, COMPUTE, INT, BASE, ARITH, MLA},
    {TYPE_SMLAXY, P_UNK, COMPUTE, INT, BASE, ARITH, MLA},
    {TYPE_SMLSD, P_UNK, COMPUTE, INT, BASE, ARITH, MLA},
    {TYPE_SMLSDX, P_UNK, COMPUTE, INT, BASE, ARITH, MLA},
    {TYPE_SMLSLD, P_UNK, COMPUTE, INT, BASE, ARITH, MLA},
    {TYPE_SMMLA, P_UNK, COMPUTE, INT, BASE, ARITH, MLA},
    {TYPE_SMMUL, SINGLE, COMPUTE, INT, BASE, ARITH, MUL},
    {TYPE_SMMULR, SINGLE, COMPUTE, INT, BASE, ARITH, MUL},
    {TYPE_SMUAD, P_UNK, COMPUTE, INT, BASE, ARITH, MLA},
    {TYPE_SMUADX, P_UNK, COMPUTE, INT, BASE, ARITH, MLA},
    {TYPE_SMULL, P_UNK, COMPUTE, INT, BASE, ARITH, MUL},
    {TYPE_SMULLS, P_UNK, COMPUTE, INT, BASE, ARITH, MUL},
    {TYPE_SMULWY, P_UNK, COMPUTE, INT, BASE, ARITH, MUL},
    {TYPE_SMULXY, P_UNK, COMPUTE, INT, BASE, ARITH, MUL},
    {TYPE_SMUSD, P_UNK, COMPUTE, INT, BASE, ARITH, MLA},
    {TYPE_SMUSDX, P_UNK, COMPUTE, INT, BASE, ARITH, MLA},
    {TYPE_STORE_REL, P_UNK, MEMORY, INT, BASE, STORE, AT_UNK},
    {TYPE_STORE_4, P_UNK, MEMORY, INT, BASE, STORE, AT_UNK},
    {TYPE_STORE_8, P_UNK, MEMORY, INT, BASE, STORE, AT_UNK},
    {TYPE_STORE_12, P_UNK, MEMORY, INT, BASE, STORE, AT_UNK},
    {TYPE_STORE_16, P_UNK, MEMORY, INT, BASE, STORE, AT_UNK},
    {TYPE_TRAP, P_UNK, UT_UNK, INT, BASE, TRAP, AT_UNK},
    {TYPE_UDIV, P_UNK, COMPUTE, INT, BASE, ARITH, DIV},
    {TYPE_UMAAL, P_UNK, COMPUTE, INT, BASE, ARITH, MLA},
    {TYPE_UMLAL, P_UNK, COMPUTE, INT, BASE, ARITH, MLA},
    {TYPE_UMLALS, P_UNK, COMPUTE, INT, BASE, ARITH, MLA},
    {TYPE_UMULL, P_UNK, COMPUTE, INT, BASE, ARITH, MUL},
    {TYPE_UMULLS, P_UNK, COMPUTE, INT, BASE, ARITH, MUL},
    {TYPE_UNTYPED, P_UNK, UT_UNK, CT_UNK, BASE, IG_UNK, AT_UNK},
    {TYPE_WMMX_TANDC, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_TBCST, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_TEXTRC, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_TEXTRM, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_TINSR, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_TMCR, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_TMCRR, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_TMIA, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_TMIAPH, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_TMIAXY, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_TMRC, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_TMRRC, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_TMOVMSK, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_TORC, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_TORVSC, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WABS, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WABSDIFF, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WACC, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WADD, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WADDBHUS, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WADDSUBHX, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WALIGNI, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WALIGNR, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WAND, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WANDN, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WAVG2, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WAVG4, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WCMPEQ, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WCMPGT, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WMAC, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WMADD, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WMAX, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WMERGE, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WMIAWXY, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WMIAXY, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WMIN, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WMOV, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WMUL, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WMULW, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WLDR, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WOR, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WPACK, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WQMIAXY, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WQMULM, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WQMULWM, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WROR, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WSAD, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WSHUFH, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WSLL, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WSRA, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WSRL, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WSTR, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WSUB, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WSUBADDHX, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WUNPCKEH, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WUNPCKEL, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WUNPCKIH, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WUNPCKIL, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_WMMX_WXOR, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_NEON_ADD, P_UNK, COMPUTE, INT, NEON, ARITH, ADD},
    {TYPE_NEON_ADD_Q, P_UNK, COMPUTE, INT, NEON, ARITH, ADD},
    {TYPE_NEON_ADD_WIDEN, P_UNK, COMPUTE, INT, NEON, ARITH, ADD},
    {TYPE_NEON_ADD_LONG, P_UNK, COMPUTE, INT, NEON, ARITH, ADD},
    {TYPE_NEON_QADD, P_UNK, COMPUTE, INT, NEON, ARITH, ADD},
    {TYPE_NEON_QADD_Q, P_UNK, COMPUTE, INT, NEON, ARITH, ADD},
    {TYPE_NEON_ADD_HALVE, P_UNK, COMPUTE, INT, NEON, ARITH, ADD},
    {TYPE_NEON_ADD_HALVE_Q, P_UNK, COMPUTE, INT, NEON, ARITH, ADD},
    {TYPE_NEON_ADD_HALVE_NARROW_Q, P_UNK, COMPUTE, INT, NEON, ARITH, ADD},
    {TYPE_NEON_SUB, P_UNK, COMPUTE, INT, NEON, ARITH, SUB},
    {TYPE_NEON_SUB_Q, P_UNK, COMPUTE, INT, NEON, ARITH, SUB},
    {TYPE_NEON_SUB_WIDEN, P_UNK, COMPUTE, INT, NEON, ARITH, SUB},
    {TYPE_NEON_SUB_LONG, P_UNK, COMPUTE, INT, NEON, ARITH, SUB},
    {TYPE_NEON_QSUB, P_UNK, COMPUTE, INT, NEON, ARITH, SUB},
    {TYPE_NEON_QSUB_Q, P_UNK, COMPUTE, INT, NEON, ARITH, SUB},
    {TYPE_NEON_SUB_HALVE, P_UNK, COMPUTE, INT, NEON, ARITH, SUB},
    {TYPE_NEON_SUB_HALVE_Q, P_UNK, COMPUTE, INT, NEON, ARITH, SUB},
    {TYPE_NEON_SUB_HALVE_NARROW_Q, P_UNK, COMPUTE, INT, NEON, ARITH, SUB},
    {TYPE_NEON_FCADD, P_UNK, COMPUTE, INT, NEON, ARITH, ADD},
    {TYPE_NEON_FCMLA, P_UNK, COMPUTE, INT, NEON, ARITH, MLA},
    {TYPE_NEON_ABS, P_UNK, COMPUTE, INT, NEON, ARITH, ABS},
    {TYPE_NEON_ABS_Q, P_UNK, COMPUTE, INT, NEON, ARITH, ABS},
    {TYPE_NEON_DOT, P_UNK, COMPUTE, INT, NEON, ARITH, AT_UNK},
    {TYPE_NEON_DOT_Q, P_UNK, COMPUTE, INT, NEON, ARITH, AT_UNK},
    {TYPE_NEON_NEG, P_UNK, COMPUTE, INT, NEON, ARITH, NEG},
    {TYPE_NEON_NEG_Q, P_UNK, COMPUTE, INT, NEON, ARITH, NEG},
    {TYPE_NEON_QNEG, P_UNK, COMPUTE, INT, NEON, ARITH, NEG},
    {TYPE_NEON_QNEG_Q, P_UNK, COMPUTE, INT, NEON, ARITH, NEG},
    {TYPE_NEON_QABS, P_UNK, COMPUTE, INT, NEON, ARITH, ABS},
    {TYPE_NEON_QABS_Q, P_UNK, COMPUTE, INT, NEON, ARITH, ABS},
    {TYPE_NEON_ABD, P_UNK, COMPUTE, INT, NEON, ARITH, AT_UNK},
    {TYPE_NEON_ABD_Q, P_UNK, COMPUTE, INT, NEON, ARITH, AT_UNK},
    {TYPE_NEON_ABD_LONG, P_UNK, COMPUTE, INT, NEON, ARITH, AT_UNK},
    {TYPE_NEON_MINMAX, P_UNK, COMPUTE, INT, NEON, ARITH, MINMAX},
    {TYPE_NEON_MINMAX_Q, P_UNK, COMPUTE, INT, NEON, ARITH, MINMAX},
    {TYPE_NEON_COMPARE, P_UNK, COMPUTE, INT, NEON, CMP, AT_UNK},
    {TYPE_NEON_COMPARE_Q, P_UNK, COMPUTE, INT, NEON, CMP, AT_UNK},
    {TYPE_NEON_COMPARE_ZERO, P_UNK, COMPUTE, INT, NEON, CMP, AT_UNK},
    {TYPE_NEON_COMPARE_ZERO_Q, P_UNK, COMPUTE, INT, NEON, CMP, AT_UNK},
    {TYPE_NEON_ARITH_ACC, P_UNK, COMPUTE, INT, NEON, ARITH, AT_UNK},
    {TYPE_NEON_ARITH_ACC_Q, P_UNK, COMPUTE, INT, NEON, ARITH, AT_UNK},
    {TYPE_NEON_REDUC_ADD, P_UNK, COMPUTE, INT, NEON, ARITH, AT_UNK},
    {TYPE_NEON_REDUC_ADD_Q, P_UNK, COMPUTE, INT, NEON, ARITH, AT_UNK},
    {TYPE_NEON_REDUC_ADD_LONG, P_UNK, COMPUTE, INT, NEON, ARITH, AT_UNK},
    {TYPE_NEON_REDUC_ADD_ACC, P_UNK, COMPUTE, INT, NEON, ARITH, AT_UNK},
    {TYPE_NEON_REDUC_ADD_ACC_Q, P_UNK, COMPUTE, INT, NEON, ARITH, AT_UNK},
    {TYPE_NEON_REDUC_MINMAX, P_UNK, COMPUTE, INT, NEON, ARITH, AT_UNK},
    {TYPE_NEON_REDUC_MINMAX_Q, P_UNK, COMPUTE, INT, NEON, ARITH, AT_UNK},
    {TYPE_NEON_LOGIC, P_UNK, UT_UNK, INT, NEON, LOGIC, AT_UNK},
    {TYPE_NEON_LOGIC_Q, P_UNK, UT_UNK, INT, NEON, LOGIC, AT_UNK},
    {TYPE_NEON_TST, P_UNK, UT_UNK, INT, NEON, LOGIC, AT_UNK},
    {TYPE_NEON_TST_Q, P_UNK, UT_UNK, INT, NEON, LOGIC, AT_UNK},
    {TYPE_NEON_SHIFT_IMM, P_UNK, UT_UNK, INT, NEON, BITFIELD, AT_UNK},
    {TYPE_NEON_SHIFT_IMM_Q, P_UNK, UT_UNK, INT, NEON, BITFIELD, AT_UNK},
    {TYPE_NEON_SHIFT_IMM_NARROW_Q, P_UNK, UT_UNK, INT, NEON, BITFIELD, AT_UNK},
    {TYPE_NEON_SHIFT_IMM_LONG, P_UNK, UT_UNK, INT, NEON, BITFIELD, AT_UNK},
    {TYPE_NEON_SHIFT_REG, P_UNK, UT_UNK, INT, NEON, BITFIELD, AT_UNK},
    {TYPE_NEON_SHIFT_REG_Q, P_UNK, UT_UNK, INT, NEON, BITFIELD, AT_UNK},
    {TYPE_NEON_SHIFT_ACC, P_UNK, UT_UNK, INT, NEON, BITFIELD, AT_UNK},
    {TYPE_NEON_SHIFT_ACC_Q, P_UNK, UT_UNK, INT, NEON, BITFIELD, AT_UNK},
    {TYPE_NEON_SAT_SHIFT_IMM, P_UNK, UT_UNK, INT, NEON, BITFIELD, AT_UNK},
    {TYPE_NEON_SAT_SHIFT_IMM_Q, P_UNK, UT_UNK, INT, NEON, BITFIELD, AT_UNK},
    {TYPE_NEON_SAT_SHIFT_IMM_NARROW_Q, P_UNK, UT_UNK, INT, NEON, BITFIELD,
     AT_UNK},
    {TYPE_NEON_SAT_SHIFT_REG, P_UNK, UT_UNK, INT, NEON, BITFIELD, AT_UNK},
    {TYPE_NEON_SAT_SHIFT_REG_Q, P_UNK, UT_UNK, INT, NEON, BITFIELD, AT_UNK},
    {TYPE_NEON_INS, P_UNK, UT_UNK, INT, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_INS_Q, P_UNK, UT_UNK, INT, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_MOVE, P_UNK, MEMORY, INT, NEON, MOV, AT_UNK},
    {TYPE_NEON_MOVE_Q, P_UNK, MEMORY, INT, NEON, MOV, AT_UNK},
    {TYPE_NEON_MOVE_NARROW_Q, P_UNK, MEMORY, INT, NEON, MOV, AT_UNK},
    {TYPE_NEON_PERMUTE, P_UNK, UT_UNK, INT, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_PERMUTE_Q, P_UNK, UT_UNK, INT, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_ZIP, P_UNK, UT_UNK, INT, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_ZIP_Q, P_UNK, UT_UNK, INT, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_TBL1, P_UNK, UT_UNK, INT, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_TBL1_Q, P_UNK, UT_UNK, INT, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_TBL2, P_UNK, UT_UNK, INT, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_TBL2_Q, P_UNK, UT_UNK, INT, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_TBL3, P_UNK, UT_UNK, INT, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_TBL3_Q, P_UNK, UT_UNK, INT, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_TBL4, P_UNK, UT_UNK, INT, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_TBL4_Q, P_UNK, UT_UNK, INT, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_BSL, P_UNK, UT_UNK, INT, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_BSL_Q, P_UNK, UT_UNK, INT, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_CLS, P_UNK, UT_UNK, INT, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_CLS_Q, P_UNK, UT_UNK, INT, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_CNT, P_UNK, UT_UNK, INT, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_CNT_Q, P_UNK, UT_UNK, INT, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_DUP, P_UNK, UT_UNK, INT, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_DUP_Q, P_UNK, UT_UNK, INT, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_EXT, P_UNK, UT_UNK, INT, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_EXT_Q, P_UNK, UT_UNK, INT, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_RBIT, P_UNK, UT_UNK, INT, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_RBIT_Q, P_UNK, UT_UNK, INT, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_REV, P_UNK, UT_UNK, INT, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_REV_Q, P_UNK, UT_UNK, INT, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_MUL_B, P_UNK, COMPUTE, INT, NEON, ARITH, MUL},
    {TYPE_NEON_MUL_B_Q, P_UNK, COMPUTE, INT, NEON, ARITH, MUL},
    {TYPE_NEON_MUL_H, P_UNK, COMPUTE, INT, NEON, ARITH, MUL},
    {TYPE_NEON_MUL_H_Q, P_UNK, COMPUTE, INT, NEON, ARITH, MUL},
    {TYPE_NEON_MUL_S, P_UNK, COMPUTE, INT, NEON, ARITH, MUL},
    {TYPE_NEON_MUL_S_Q, P_UNK, COMPUTE, INT, NEON, ARITH, MUL},
    {TYPE_NEON_MUL_B_LONG, P_UNK, COMPUTE, INT, NEON, ARITH, MUL},
    {TYPE_NEON_MUL_H_LONG, P_UNK, COMPUTE, INT, NEON, ARITH, MUL},
    {TYPE_NEON_MUL_S_LONG, P_UNK, COMPUTE, INT, NEON, ARITH, MUL},
    {TYPE_NEON_MUL_D_LONG, P_UNK, COMPUTE, INT, NEON, ARITH, MUL},
    {TYPE_NEON_MUL_H_SCALAR, P_UNK, COMPUTE, INT, NEON, ARITH, MUL},
    {TYPE_NEON_MUL_H_SCALAR_Q, P_UNK, COMPUTE, INT, NEON, ARITH, MUL},
    {TYPE_NEON_MUL_S_SCALAR, P_UNK, COMPUTE, INT, NEON, ARITH, MUL},
    {TYPE_NEON_MUL_S_SCALAR_Q, P_UNK, COMPUTE, INT, NEON, ARITH, MUL},
    {TYPE_NEON_MUL_H_SCALAR_LONG, P_UNK, COMPUTE, INT, NEON, ARITH, MUL},
    {TYPE_NEON_MUL_S_SCALAR_LONG, P_UNK, COMPUTE, INT, NEON, ARITH, MUL},
    {TYPE_NEON_SAT_MUL_B, P_UNK, COMPUTE, INT, NEON, ARITH, MUL},
    {TYPE_NEON_SAT_MUL_B_Q, P_UNK, COMPUTE, INT, NEON, ARITH, MUL},
    {TYPE_NEON_SAT_MUL_H, P_UNK, COMPUTE, INT, NEON, ARITH, MUL},
    {TYPE_NEON_SAT_MUL_H_Q, P_UNK, COMPUTE, INT, NEON, ARITH, MUL},
    {TYPE_NEON_SAT_MUL_S, P_UNK, COMPUTE, INT, NEON, ARITH, MUL},
    {TYPE_NEON_SAT_MUL_S_Q, P_UNK, COMPUTE, INT, NEON, ARITH, MUL},
    {TYPE_NEON_SAT_MUL_B_LONG, P_UNK, COMPUTE, INT, NEON, ARITH, MUL},
    {TYPE_NEON_SAT_MUL_H_LONG, P_UNK, COMPUTE, INT, NEON, ARITH, MUL},
    {TYPE_NEON_SAT_MUL_S_LONG, P_UNK, COMPUTE, INT, NEON, ARITH, MUL},
    {TYPE_NEON_SAT_MUL_H_SCALAR, P_UNK, COMPUTE, INT, NEON, ARITH, MUL},
    {TYPE_NEON_SAT_MUL_H_SCALAR_Q, P_UNK, COMPUTE, INT, NEON, ARITH, MUL},
    {TYPE_NEON_SAT_MUL_S_SCALAR, P_UNK, COMPUTE, INT, NEON, ARITH, MUL},
    {TYPE_NEON_SAT_MUL_S_SCALAR_Q, P_UNK, COMPUTE, INT, NEON, ARITH, MUL},
    {TYPE_NEON_SAT_MUL_H_SCALAR_LONG, P_UNK, COMPUTE, INT, NEON, ARITH, MUL},
    {TYPE_NEON_SAT_MUL_S_SCALAR_LONG, P_UNK, COMPUTE, INT, NEON, ARITH, MUL},
    {TYPE_NEON_MLA_B, P_UNK, COMPUTE, INT, NEON, ARITH, MLA},
    {TYPE_NEON_MLA_B_Q, P_UNK, COMPUTE, INT, NEON, ARITH, MLA},
    {TYPE_NEON_MLA_H, P_UNK, COMPUTE, INT, NEON, ARITH, MLA},
    {TYPE_NEON_MLA_H_Q, P_UNK, COMPUTE, INT, NEON, ARITH, MLA},
    {TYPE_NEON_MLA_S, P_UNK, COMPUTE, INT, NEON, ARITH, MLA},
    {TYPE_NEON_MLA_S_Q, P_UNK, COMPUTE, INT, NEON, ARITH, MLA},
    {TYPE_NEON_MLA_B_LONG, P_UNK, COMPUTE, INT, NEON, ARITH, MLA},
    {TYPE_NEON_MLA_H_LONG, P_UNK, COMPUTE, INT, NEON, ARITH, MLA},
    {TYPE_NEON_MLA_S_LONG, P_UNK, COMPUTE, INT, NEON, ARITH, MLA},
    {TYPE_NEON_MLA_H_SCALAR, P_UNK, COMPUTE, INT, NEON, ARITH, MLA},
    {TYPE_NEON_MLA_H_SCALAR_Q, P_UNK, COMPUTE, INT, NEON, ARITH, MLA},
    {TYPE_NEON_MLA_S_SCALAR, P_UNK, COMPUTE, INT, NEON, ARITH, MLA},
    {TYPE_NEON_MLA_S_SCALAR_Q, P_UNK, COMPUTE, INT, NEON, ARITH, MLA},
    {TYPE_NEON_MLA_H_SCALAR_LONG, P_UNK, COMPUTE, INT, NEON, ARITH, MLA},
    {TYPE_NEON_MLA_S_SCALAR_LONG, P_UNK, COMPUTE, INT, NEON, ARITH, MLA},
    {TYPE_NEON_SAT_MLA_B_LONG, P_UNK, COMPUTE, INT, NEON, ARITH, MLA},
    {TYPE_NEON_SAT_MLA_H_LONG, P_UNK, COMPUTE, INT, NEON, ARITH, MLA},
    {TYPE_NEON_SAT_MLA_S_LONG, P_UNK, COMPUTE, INT, NEON, ARITH, MLA},
    {TYPE_NEON_SAT_MLA_H_SCALAR_LONG, P_UNK, COMPUTE, INT, NEON, ARITH, MLA},
    {TYPE_NEON_SAT_MLA_S_SCALAR_LONG, P_UNK, COMPUTE, INT, NEON, ARITH, MLA},
    {TYPE_NEON_TO_GP, P_UNK, UT_UNK, INT, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_TO_GP_Q, P_UNK, UT_UNK, INT, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_FROM_GP, P_UNK, UT_UNK, INT, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_FROM_GP_Q, P_UNK, UT_UNK, INT, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_LDR, P_UNK, MEMORY, INT, NEON, LOAD, AT_UNK},
    {TYPE_NEON_LDP, P_UNK, MEMORY, INT, NEON, LOAD, AT_UNK},
    {TYPE_NEON_LDP_Q, P_UNK, MEMORY, INT, NEON, LOAD, AT_UNK},
    {TYPE_NEON_LOAD1_1REG, P_UNK, MEMORY, INT, NEON, LOAD, AT_UNK},
    {TYPE_NEON_LOAD1_1REG_Q, P_UNK, MEMORY, INT, NEON, LOAD, AT_UNK},
    {TYPE_NEON_LOAD1_2REG, P_UNK, MEMORY, INT, NEON, LOAD, AT_UNK},
    {TYPE_NEON_LOAD1_2REG_Q, P_UNK, MEMORY, INT, NEON, LOAD, AT_UNK},
    {TYPE_NEON_LOAD1_3REG, P_UNK, MEMORY, INT, NEON, LOAD, AT_UNK},
    {TYPE_NEON_LOAD1_3REG_Q, P_UNK, MEMORY, INT, NEON, LOAD, AT_UNK},
    {TYPE_NEON_LOAD1_4REG, P_UNK, MEMORY, INT, NEON, LOAD, AT_UNK},
    {TYPE_NEON_LOAD1_4REG_Q, P_UNK, MEMORY, INT, NEON, LOAD, AT_UNK},
    {TYPE_NEON_LOAD1_ALL_LANES, P_UNK, MEMORY, INT, NEON, LOAD, AT_UNK},
    {TYPE_NEON_LOAD1_ALL_LANES_Q, P_UNK, MEMORY, INT, NEON, LOAD, AT_UNK},
    {TYPE_NEON_LOAD1_ONE_LANE, P_UNK, MEMORY, INT, NEON, LOAD, AT_UNK},
    {TYPE_NEON_LOAD1_ONE_LANE_Q, P_UNK, MEMORY, INT, NEON, LOAD, AT_UNK},
    {TYPE_NEON_LOAD2_2REG, P_UNK, MEMORY, INT, NEON, LOAD, AT_UNK},
    {TYPE_NEON_LOAD2_2REG_Q, P_UNK, MEMORY, INT, NEON, LOAD, AT_UNK},
    {TYPE_NEON_LOAD2_4REG, P_UNK, MEMORY, INT, NEON, LOAD, AT_UNK},
    {TYPE_NEON_LOAD2_4REG_Q, P_UNK, MEMORY, INT, NEON, LOAD, AT_UNK},
    {TYPE_NEON_LOAD2_ALL_LANES, P_UNK, MEMORY, INT, NEON, LOAD, AT_UNK},
    {TYPE_NEON_LOAD2_ALL_LANES_Q, P_UNK, MEMORY, INT, NEON, LOAD, AT_UNK},
    {TYPE_NEON_LOAD2_ONE_LANE, P_UNK, MEMORY, INT, NEON, LOAD, AT_UNK},
    {TYPE_NEON_LOAD2_ONE_LANE_Q, P_UNK, MEMORY, INT, NEON, LOAD, AT_UNK},
    {TYPE_NEON_LOAD3_3REG, P_UNK, MEMORY, INT, NEON, LOAD, AT_UNK},
    {TYPE_NEON_LOAD3_3REG_Q, P_UNK, MEMORY, INT, NEON, LOAD, AT_UNK},
    {TYPE_NEON_LOAD3_ALL_LANES, P_UNK, MEMORY, INT, NEON, LOAD, AT_UNK},
    {TYPE_NEON_LOAD3_ALL_LANES_Q, P_UNK, MEMORY, INT, NEON, LOAD, AT_UNK},
    {TYPE_NEON_LOAD3_ONE_LANE, P_UNK, MEMORY, INT, NEON, LOAD, AT_UNK},
    {TYPE_NEON_LOAD3_ONE_LANE_Q, P_UNK, MEMORY, INT, NEON, LOAD, AT_UNK},
    {TYPE_NEON_LOAD4_4REG, P_UNK, MEMORY, INT, NEON, LOAD, AT_UNK},
    {TYPE_NEON_LOAD4_4REG_Q, P_UNK, MEMORY, INT, NEON, LOAD, AT_UNK},
    {TYPE_NEON_LOAD4_ALL_LANES, P_UNK, MEMORY, INT, NEON, LOAD, AT_UNK},
    {TYPE_NEON_LOAD4_ALL_LANES_Q, P_UNK, MEMORY, INT, NEON, LOAD, AT_UNK},
    {TYPE_NEON_LOAD4_ONE_LANE, P_UNK, MEMORY, INT, NEON, LOAD, AT_UNK},
    {TYPE_NEON_LOAD4_ONE_LANE_Q, P_UNK, MEMORY, INT, NEON, LOAD, AT_UNK},
    {TYPE_NEON_STR, P_UNK, MEMORY, INT, NEON, STORE, AT_UNK},
    {TYPE_NEON_STP, P_UNK, MEMORY, INT, NEON, STORE, AT_UNK},
    {TYPE_NEON_STP_Q, P_UNK, MEMORY, INT, NEON, STORE, AT_UNK},
    {TYPE_NEON_STORE1_1REG, P_UNK, MEMORY, INT, NEON, STORE, AT_UNK},
    {TYPE_NEON_STORE1_1REG_Q, P_UNK, MEMORY, INT, NEON, STORE, AT_UNK},
    {TYPE_NEON_STORE1_2REG, P_UNK, MEMORY, INT, NEON, STORE, AT_UNK},
    {TYPE_NEON_STORE1_2REG_Q, P_UNK, MEMORY, INT, NEON, STORE, AT_UNK},
    {TYPE_NEON_STORE1_3REG, P_UNK, MEMORY, INT, NEON, STORE, AT_UNK},
    {TYPE_NEON_STORE1_3REG_Q, P_UNK, MEMORY, INT, NEON, STORE, AT_UNK},
    {TYPE_NEON_STORE1_4REG, P_UNK, MEMORY, INT, NEON, STORE, AT_UNK},
    {TYPE_NEON_STORE1_4REG_Q, P_UNK, MEMORY, INT, NEON, STORE, AT_UNK},
    {TYPE_NEON_STORE1_ONE_LANE, P_UNK, MEMORY, INT, NEON, STORE, AT_UNK},
    {TYPE_NEON_STORE1_ONE_LANE_Q, P_UNK, MEMORY, INT, NEON, STORE, AT_UNK},
    {TYPE_NEON_STORE2_2REG, P_UNK, MEMORY, INT, NEON, STORE, AT_UNK},
    {TYPE_NEON_STORE2_2REG_Q, P_UNK, MEMORY, INT, NEON, STORE, AT_UNK},
    {TYPE_NEON_STORE2_4REG, P_UNK, MEMORY, INT, NEON, STORE, AT_UNK},
    {TYPE_NEON_STORE2_4REG_Q, P_UNK, MEMORY, INT, NEON, STORE, AT_UNK},
    {TYPE_NEON_STORE2_ONE_LANE, P_UNK, MEMORY, INT, NEON, STORE, AT_UNK},
    {TYPE_NEON_STORE2_ONE_LANE_Q, P_UNK, MEMORY, INT, NEON, STORE, AT_UNK},
    {TYPE_NEON_STORE3_3REG, P_UNK, MEMORY, INT, NEON, STORE, AT_UNK},
    {TYPE_NEON_STORE3_3REG_Q, P_UNK, MEMORY, INT, NEON, STORE, AT_UNK},
    {TYPE_NEON_STORE3_ONE_LANE, P_UNK, MEMORY, INT, NEON, STORE, AT_UNK},
    {TYPE_NEON_STORE3_ONE_LANE_Q, P_UNK, MEMORY, INT, NEON, STORE, AT_UNK},
    {TYPE_NEON_STORE4_4REG, P_UNK, MEMORY, INT, NEON, STORE, AT_UNK},
    {TYPE_NEON_STORE4_4REG_Q, P_UNK, MEMORY, INT, NEON, STORE, AT_UNK},
    {TYPE_NEON_STORE4_ONE_LANE, P_UNK, MEMORY, INT, NEON, STORE, AT_UNK},
    {TYPE_NEON_STORE4_ONE_LANE_Q, P_UNK, MEMORY, INT, NEON, STORE, AT_UNK},
    {TYPE_NEON_FP_ABS_S, P_UNK, COMPUTE, FP, NEON, FP_ARITH, ABS},
    {TYPE_NEON_FP_ABS_S_Q, P_UNK, COMPUTE, FP, NEON, FP_ARITH, ABS},
    {TYPE_NEON_FP_ABS_D, P_UNK, COMPUTE, FP, NEON, FP_ARITH, ABS},
    {TYPE_NEON_FP_ABS_D_Q, P_UNK, COMPUTE, FP, NEON, FP_ARITH, ABS},
    {TYPE_NEON_FP_NEG_S, P_UNK, COMPUTE, FP, NEON, FP_ARITH, NEG},
    {TYPE_NEON_FP_NEG_S_Q, P_UNK, COMPUTE, FP, NEON, FP_ARITH, NEG},
    {TYPE_NEON_FP_NEG_D, P_UNK, COMPUTE, FP, NEON, FP_ARITH, NEG},
    {TYPE_NEON_FP_NEG_D_Q, P_UNK, COMPUTE, FP, NEON, FP_ARITH, NEG},
    {TYPE_NEON_FP_ABD_S, P_UNK, COMPUTE, FP, NEON, FP_ARITH, AT_UNK},
    {TYPE_NEON_FP_ABD_S_Q, P_UNK, COMPUTE, FP, NEON, FP_ARITH, AT_UNK},
    {TYPE_NEON_FP_ABD_D, P_UNK, COMPUTE, FP, NEON, FP_ARITH, AT_UNK},
    {TYPE_NEON_FP_ABD_D_Q, P_UNK, COMPUTE, FP, NEON, FP_ARITH, AT_UNK},
    {TYPE_NEON_FP_ADDSUB_S, P_UNK, COMPUTE, FP, NEON, FP_ARITH, AT_UNK},
    {TYPE_NEON_FP_ADDSUB_S_Q, P_UNK, COMPUTE, FP, NEON, FP_ARITH, AT_UNK},
    {TYPE_NEON_FP_ADDSUB_D, P_UNK, COMPUTE, FP, NEON, FP_ARITH, AT_UNK},
    {TYPE_NEON_FP_ADDSUB_D_Q, P_UNK, COMPUTE, FP, NEON, FP_ARITH, AT_UNK},
    {TYPE_NEON_FP_COMPARE_S, P_UNK, COMPUTE, FP, NEON, CMP, AT_UNK},
    {TYPE_NEON_FP_COMPARE_S_Q, P_UNK, COMPUTE, FP, NEON, CMP, AT_UNK},
    {TYPE_NEON_FP_COMPARE_D, P_UNK, COMPUTE, FP, NEON, CMP, AT_UNK},
    {TYPE_NEON_FP_COMPARE_D_Q, P_UNK, COMPUTE, FP, NEON, CMP, AT_UNK},
    {TYPE_NEON_FP_MINMAX_S, P_UNK, COMPUTE, FP, NEON, FP_ARITH, MINMAX},
    {TYPE_NEON_FP_MINMAX_S_Q, P_UNK, COMPUTE, FP, NEON, FP_ARITH, MINMAX},
    {TYPE_NEON_FP_MINMAX_D, P_UNK, COMPUTE, FP, NEON, FP_ARITH, MINMAX},
    {TYPE_NEON_FP_MINMAX_D_Q, P_UNK, COMPUTE, FP, NEON, FP_ARITH, MINMAX},
    {TYPE_NEON_FP_REDUC_ADD_S, P_UNK, COMPUTE, FP, NEON, FP_ARITH, AT_UNK},
    {TYPE_NEON_FP_REDUC_ADD_S_Q, P_UNK, COMPUTE, FP, NEON, FP_ARITH, AT_UNK},
    {TYPE_NEON_FP_REDUC_ADD_D, P_UNK, COMPUTE, FP, NEON, FP_ARITH, AT_UNK},
    {TYPE_NEON_FP_REDUC_ADD_D_Q, P_UNK, COMPUTE, FP, NEON, FP_ARITH, AT_UNK},
    {TYPE_NEON_FP_REDUC_MINMAX_S, P_UNK, COMPUTE, FP, NEON, FP_ARITH, AT_UNK},
    {TYPE_NEON_FP_REDUC_MINMAX_S_Q, P_UNK, COMPUTE, FP, NEON, FP_ARITH, AT_UNK},
    {TYPE_NEON_FP_REDUC_MINMAX_D, P_UNK, COMPUTE, FP, NEON, FP_ARITH, AT_UNK},
    {TYPE_NEON_FP_REDUC_MINMAX_D_Q, P_UNK, COMPUTE, FP, NEON, FP_ARITH, AT_UNK},
    {TYPE_NEON_FP_CVT_NARROW_S_Q, P_UNK, COMPUTE, FP, NEON, FP_CVT, AT_UNK},
    {TYPE_NEON_FP_CVT_NARROW_D_Q, P_UNK, COMPUTE, FP, NEON, FP_CVT, AT_UNK},
    {TYPE_NEON_FP_CVT_WIDEN_H, P_UNK, COMPUTE, FP, NEON, FP_CVT, AT_UNK},
    {TYPE_NEON_FP_CVT_WIDEN_S, P_UNK, COMPUTE, FP, NEON, FP_CVT, AT_UNK},
    {TYPE_NEON_FP_TO_INT_S, P_UNK, COMPUTE, FP, NEON, FP_CVT, AT_UNK},
    {TYPE_NEON_FP_TO_INT_S_Q, P_UNK, COMPUTE, FP, NEON, FP_CVT, AT_UNK},
    {TYPE_NEON_FP_TO_INT_D, P_UNK, COMPUTE, FP, NEON, FP_CVT, AT_UNK},
    {TYPE_NEON_FP_TO_INT_D_Q, P_UNK, COMPUTE, FP, NEON, FP_CVT, AT_UNK},
    {TYPE_NEON_INT_TO_FP_S, P_UNK, COMPUTE, FP, NEON, FP_CVT, AT_UNK},
    {TYPE_NEON_INT_TO_FP_S_Q, P_UNK, COMPUTE, FP, NEON, FP_CVT, AT_UNK},
    {TYPE_NEON_INT_TO_FP_D, P_UNK, COMPUTE, FP, NEON, FP_CVT, AT_UNK},
    {TYPE_NEON_INT_TO_FP_D_Q, P_UNK, COMPUTE, FP, NEON, FP_CVT, AT_UNK},
    {TYPE_NEON_FP_ROUND_S, P_UNK, COMPUTE, FP, NEON, FP_ARITH, ROUND},
    {TYPE_NEON_FP_ROUND_S_Q, P_UNK, COMPUTE, FP, NEON, FP_ARITH, ROUND},
    {TYPE_NEON_FP_ROUND_D, P_UNK, COMPUTE, FP, NEON, FP_ARITH, ROUND},
    {TYPE_NEON_FP_ROUND_D_Q, P_UNK, COMPUTE, FP, NEON, FP_ARITH, ROUND},
    {TYPE_NEON_FP_RECPE_S, P_UNK, COMPUTE, FP, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_FP_RECPE_S_Q, P_UNK, COMPUTE, FP, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_FP_RECPE_D, P_UNK, COMPUTE, FP, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_FP_RECPE_D_Q, P_UNK, COMPUTE, FP, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_FP_RECPS_S, P_UNK, COMPUTE, FP, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_FP_RECPS_S_Q, P_UNK, COMPUTE, FP, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_FP_RECPS_D, P_UNK, COMPUTE, FP, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_FP_RECPS_D_Q, P_UNK, COMPUTE, FP, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_FP_RECPX_S, P_UNK, COMPUTE, FP, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_FP_RECPX_S_Q, P_UNK, COMPUTE, FP, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_FP_RECPX_D, P_UNK, COMPUTE, FP, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_FP_RECPX_D_Q, P_UNK, COMPUTE, FP, NEON, IG_UNK, AT_UNK},
    {TYPE_NEON_FP_RSQRTE_S, P_UNK, COMPUTE, FP, NEON, FP_ARITH, SQRT},
    {TYPE_NEON_FP_RSQRTE_S_Q, P_UNK, COMPUTE, FP, NEON, FP_ARITH, SQRT},
    {TYPE_NEON_FP_RSQRTE_D, P_UNK, COMPUTE, FP, NEON, FP_ARITH, SQRT},
    {TYPE_NEON_FP_RSQRTE_D_Q, P_UNK, COMPUTE, FP, NEON, FP_ARITH, SQRT},
    {TYPE_NEON_FP_RSQRTS_S, P_UNK, COMPUTE, FP, NEON, FP_ARITH, SQRT},
    {TYPE_NEON_FP_RSQRTS_S_Q, P_UNK, COMPUTE, FP, NEON, FP_ARITH, SQRT},
    {TYPE_NEON_FP_RSQRTS_D, P_UNK, COMPUTE, FP, NEON, FP_ARITH, SQRT},
    {TYPE_NEON_FP_RSQRTS_D_Q, P_UNK, COMPUTE, FP, NEON, FP_ARITH, SQRT},
    {TYPE_NEON_FP_MUL_S, P_UNK, COMPUTE, FP, NEON, FP_ARITH, MUL},
    {TYPE_NEON_FP_MUL_S_Q, P_UNK, COMPUTE, FP, NEON, FP_ARITH, MUL},
    {TYPE_NEON_FP_MUL_S_SCALAR, P_UNK, COMPUTE, FP, NEON, FP_ARITH, MUL},
    {TYPE_NEON_FP_MUL_S_SCALAR_Q, P_UNK, COMPUTE, FP, NEON, FP_ARITH, MUL},
    {TYPE_NEON_FP_MUL_D, P_UNK, COMPUTE, FP, NEON, FP_ARITH, MUL},
    {TYPE_NEON_FP_MUL_D_Q, P_UNK, COMPUTE, FP, NEON, FP_ARITH, MUL},
    {TYPE_NEON_FP_MUL_D_SCALAR_Q, P_UNK, COMPUTE, FP, NEON, FP_ARITH, MUL},
    {TYPE_NEON_FP_MLA_S, P_UNK, COMPUTE, FP, NEON, FP_ARITH, MLA},
    {TYPE_NEON_FP_MLA_S_Q, P_UNK, COMPUTE, FP, NEON, FP_ARITH, MLA},
    {TYPE_NEON_FP_MLA_S_SCALAR, P_UNK, COMPUTE, FP, NEON, FP_ARITH, MLA},
    {TYPE_NEON_FP_MLA_S_SCALAR_Q, P_UNK, COMPUTE, FP, NEON, FP_ARITH, MLA},
    {TYPE_NEON_FP_MLA_D, P_UNK, COMPUTE, FP, NEON, FP_ARITH, MLA},
    {TYPE_NEON_FP_MLA_D_Q, P_UNK, COMPUTE, FP, NEON, FP_ARITH, MLA},
    {TYPE_NEON_FP_MLA_D_SCALAR_Q, P_UNK, COMPUTE, FP, NEON, FP_ARITH, MLA},
    {TYPE_NEON_FP_SQRT_S, P_UNK, COMPUTE, FP, NEON, FP_ARITH, SQRT},
    {TYPE_NEON_FP_SQRT_S_Q, P_UNK, COMPUTE, FP, NEON, FP_ARITH, SQRT},
    {TYPE_NEON_FP_SQRT_D, P_UNK, COMPUTE, FP, NEON, FP_ARITH, SQRT},
    {TYPE_NEON_FP_SQRT_D_Q, P_UNK, COMPUTE, FP, NEON, FP_ARITH, SQRT},
    {TYPE_NEON_FP_DIV_S, P_UNK, COMPUTE, FP, NEON, FP_ARITH, DIV},
    {TYPE_NEON_FP_DIV_S_Q, P_UNK, COMPUTE, FP, NEON, FP_ARITH, DIV},
    {TYPE_NEON_FP_DIV_D, P_UNK, COMPUTE, FP, NEON, FP_ARITH, DIV},
    {TYPE_NEON_FP_DIV_D_Q, P_UNK, COMPUTE, FP, NEON, FP_ARITH, DIV},
    {TYPE_CRYPTO_AESE, P_UNK, UT_UNK, CT_UNK, IS_UNK, CRYPTO, AT_UNK},
    {TYPE_CRYPTO_AESMC, P_UNK, UT_UNK, CT_UNK, IS_UNK, CRYPTO, AT_UNK},
    {TYPE_CRYPTO_SHA1_XOR, P_UNK, UT_UNK, CT_UNK, IS_UNK, CRYPTO, AT_UNK},
    {TYPE_CRYPTO_SHA1_FAST, P_UNK, UT_UNK, CT_UNK, IS_UNK, CRYPTO, AT_UNK},
    {TYPE_CRYPTO_SHA1_SLOW, P_UNK, UT_UNK, CT_UNK, IS_UNK, CRYPTO, AT_UNK},
    {TYPE_CRYPTO_SHA256_FAST, P_UNK, UT_UNK, CT_UNK, IS_UNK, CRYPTO, AT_UNK},
    {TYPE_CRYPTO_SHA256_SLOW, P_UNK, UT_UNK, CT_UNK, IS_UNK, CRYPTO, AT_UNK},
    {TYPE_CRYPTO_PMULL, P_UNK, UT_UNK, CT_UNK, IS_UNK, CRYPTO, AT_UNK},
    {TYPE_CRYPTO_SHA512, P_UNK, UT_UNK, CT_UNK, IS_UNK, CRYPTO, AT_UNK},
    {TYPE_CRYPTO_SHA3, P_UNK, UT_UNK, CT_UNK, IS_UNK, CRYPTO, AT_UNK},
    {TYPE_CRYPTO_SM3, P_UNK, UT_UNK, CT_UNK, IS_UNK, CRYPTO, AT_UNK},
    {TYPE_CRYPTO_SM4, P_UNK, UT_UNK, CT_UNK, IS_UNK, CRYPTO, AT_UNK},
    {TYPE_COPROC, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_TME, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_MEMTAG, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_LS64, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_MVE_MOVE, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_MVE_STORE, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK},
    {TYPE_MVE_LOAD, P_UNK, UT_UNK, CT_UNK, IS_UNK, IG_UNK, AT_UNK}};

/* Vector `iat_table` starts with index 0, recording every element
   of Enumeration `attr_type` in sequence. */
static const int IAT_IDX_OFFSET = 0;

static bool is_valid_insn_attr_type(rtx_insn* insn) {
  unsigned recog = recog_memoized(insn);
  if (recog >= NUM_INSN_CODES) return false;

  attr_type atype = get_attr_type(insn);
  if (atype - IAT_IDX_OFFSET < iat_table.size() &&
      iat_table[atype - IAT_IDX_OFFSET].name == atype) {
    return true;
  }
  return false;
}

static insn_attr_type get_insn_attr_type(rtx_insn* insn) {
  attr_type atype = get_attr_type(insn);
  return iat_table[atype - IAT_IDX_OFFSET];
}

static bool is_fp_compute_op(insn_attr_type iatype) {
  return iatype.use == COMPUTE && iatype.calc == FP;
}

static bool is_branch_op(insn_attr_type iatype) {
  return iatype.gentype == BRANCH;
}

static bool is_intrinsic_op(insn_attr_type iatype) {
  return iatype.gentype == FP_INTRINSIC || iatype.name == TYPE_F_CVTF2I;
}

static bool is_fcmp_op(insn_attr_type iatype) {
  return iatype.gentype == CMP && iatype.calc == FP;
}

static bool is_fdiv_op(insn_attr_type iatype) {
  return iatype.arith == DIV && iatype.calc == FP;
}

static bool is_fmul_op(insn_attr_type iatype) {
  return iatype.arith == MUL && iatype.calc == FP;
}

static bool is_fp2si_op(insn_attr_type iatype) {
  switch (iatype.name) {
    case TYPE_F_CVTF2I:
    case TYPE_NEON_FP_TO_INT_S:
    case TYPE_NEON_FP_TO_INT_S_Q:
    case TYPE_NEON_FP_TO_INT_D:
    case TYPE_NEON_FP_TO_INT_D_Q:
      return true;
    default:
      return false;
  }
}

static bool is_vect_op(insn_attr_type iatype) { return iatype.set != BASE; }

static bool is_unreachable_op(insn_attr_type iatype) {
  return iatype.gentype == NOP;
}

static bool is_load_op(insn_attr_type iatype) { return iatype.gentype == LOAD; }

static bool is_store_op(insn_attr_type iatype) {
  return iatype.gentype == STORE;
}

static bool is_memory_op(insn_attr_type iatype) {
  return iatype.gentype == LOAD || iatype.gentype == STORE;
}

static bool is_implicit_op(rtx_insn* insn, insn_attr_type iatype) {
  int insn_code = INSN_CODE(insn);
  switch (insn_code) {
    case 26: /* prefetch */
    case 27: /* prefetch_full */
      return true;
    default:
      return is_intrinsic_op(iatype);
  }
}

static bool is_fadd_op(rtx_insn* insn, insn_attr_type iatype) {
  if (iatype.calc == FP && iatype.arith == ADD) {
    return true;
  }

  int insn_code = INSN_CODE(insn);
  switch (insn_code) {
    case 1003: /* addhf3 */
    case 1004: /* addsf3 */
    case 1005: /* adddf3 */
      return true;
    default:
      return false;
  }
}

static bool is_fsub_op(rtx_insn* insn) {
  int insn_code = INSN_CODE(insn);
  switch (insn_code) {
    case 1006: /* subhf3 */
    case 1007: /* subsf3 */
    case 1008: /* subdf3 */
      return true;
    default:
      return false;
  }
}

static bool is_ret_op(rtx_insn* insn) {
  int insn_code = INSN_CODE(insn);
  switch (insn_code) {
    case 29: /* *do_return */
    case 30: /* simple_return */
      return true;
    default:
      return false;
  }
}

}  // namespace ai4c
#endif  // AI4C_GCC_INSN_UTILS_AARCH64_H
