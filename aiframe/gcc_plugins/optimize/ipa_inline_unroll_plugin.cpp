#include "gcc-plugin.h"
#include "plugin-version.h"
#include "context.h"
#include <cstdint>
#include <unistd.h>
#include <string>
#include <dlfcn.h>

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "backend.h"
#include "target.h"
#include "rtl.h"
#include "tree.h"
#include "gimple.h"
#include "alloc-pool.h"
#include "tree-pass.h"
#include "gimple-ssa.h"
#include "cgraph.h"
#include "lto-streamer.h"
#include "trans-mem.h"
#include "calls.h"
#include "tree-inline.h"
#include "profile.h"
#include "symbol-summary.h"
#include "tree-vrp.h"
#include "ipa-prop.h"
#include "ipa-fnsummary.h"
#include "ipa-inline.h"
#include "ipa-utils.h"
#include "sreal.h"
#include "auto-profile.h"
#include "builtins.h"
#include "fibonacci_heap.h"
#include "stringpool.h"
#include "attribs.h"
#include "asan.h"

#include "gimple-iterator.h"
#include <unordered_map>
#include <cstdio>
#include <vector>

#include <iostream>
#include <chrono>

#include "cfgloop.h"
#include "plugin_utils.h"
#include "feature_utils.h"
#include <map>
#include "function.h"
#include "cfghooks.h"
#include "df.h"
#include "regs.h"
#include "cfgcleanup.h"
#include "tree-ssa-loop-niter.h"
#include "tree-scalar-evolution.h"
#include "tree-cfgcleanup.h"
#include "memmodel.h"
#include "optabs.h"
#include "emit-rtl.h"
#include "recog.h"
#include "cfgrtl.h"
#include "dojump.h"
#include "expr.h"
#include "dumpfile.h"
#include <fstream>

using namespace ai4c;

int plugin_is_GPL_compatible;

/* ===---- AI4Compiler Models and Inference Engine Interface ----=== */
char *g_inline_model_path;
char *g_unroll_model_path;
char *g_infer_path;

/* Add new variable.  */
int64_t simultaneous_prefetches[1] = {param_simultaneous_prefetches};
int64_t l1_cache_size[1] = {param_l1_cache_size};
int64_t l1_cache_line_size[1] = {param_l1_cache_line_size};
int64_t l2_cache_size[1] = {param_l2_cache_size};
int64_t prefetch_latency[1] = {param_prefetch_latency};
int64_t ipa_prefetch_distance_factor[1] = {param_ipa_prefetch_distance_factor};
static char hash[65];
const char *var_str[1];
const char *native_tune = getenv("GCC_AI4C_TUNE_INFO");

/* Interfaces of AI4C Inference Engine */
void *g_infer_handle;

typedef void (*init_engine_t)(const char *);
init_engine_t initialize;

typedef void (*add_int64_input_t)(int64_t *, int);
add_int64_input_t add_int64_input;

typedef void (*add_int32_input_t)(int32_t *, int);
add_int32_input_t add_int32_input;

typedef void (*add_float_input_t)(float *, int);
add_float_input_t add_float_input;

typedef void (*add_double_input_t)(double *, int);
add_double_input_t add_double_input;

typedef void (*add_string_input_t)(const char **, int);
add_string_input_t add_string_input;

typedef int (*inference_t)();
inference_t inference;

typedef int32_t *(*get_int32_output_t)(int);
get_int32_output_t get_int32_output;

typedef int64_t *(*get_int64_output_t)(int);
get_int64_output_t get_int64_output;

typedef float *(*get_float_output_t)(int);
get_float_output_t get_float_output;

typedef void (*free_engine_t)();
free_engine_t free_engine;
/* ===---- AI4Compiler Models and Inference Engine Interface ----=== */

typedef fibonacci_heap<sreal, cgraph_edge> edge_heap_t;
typedef fibonacci_node<sreal, cgraph_edge> edge_heap_node_t;

/* Statistics we collect about inlining algorithm.  */
static int overall_size;
static profile_count max_count;
static profile_count spec_rem;

/* AI4Compiler: Declaration of newly-defined functions and variables.  */
void execute_sha256(const char *input, char *output, size_t output_size) {
  char command[256];
  snprintf(command, sizeof(command), "echo -n \"%s\" | sha256sum", input);

  FILE *pipe = popen(command, "r");
  if (pipe == NULL) {
    perror("Failed to run command.");
    return;
  }

  fgets(output, output_size, pipe);
  pclose(pipe);
}

const char *get_optimize_decision() {
  static char input[64];
  if (native_tune && (strchr(native_tune, '+') != NULL)) {
    const char prefix = '=';
    const char *start = strchr(native_tune, prefix);
    if (start) {
      start += 1;
      const char *end = strchr(start, '+');
      if (!end) {
        end = native_tune + strlen(native_tune);
      }
      size_t len = end - start;
      if (len >= sizeof(input)) len = sizeof(input) - 1;
      strncpy(input, start, len);
      input[len] = '\0';
    } else
      input[0] = '\0';
  }
  return input;
}

void get_basic_block_info(struct cgraph_node *node, int &bb_count,
                          int &cond_bb_count);
int get_function_parameters_info(struct cgraph_node *node);
void get_caller_callee_info(struct cgraph_node *node, int &caller_count,
                            int &callee_count);
void get_edge_info(struct cgraph_edge *edge, int &edge_count,
                   double &edge_freq);
struct StatementInfo {
  int total_stmt_count;
  std::unordered_map<enum gimple_code, int> stmt_counts;
};
static StatementInfo count_stmt_types_fn(struct cgraph_node *node);
void extract_node_features(struct cgraph_node *node,
                           std::vector<double> &features);
void get_features(struct cgraph_node *node, struct cgraph_edge *edge,
                  std::vector<double> &features);
std::unordered_map<cgraph_edge *, bool> edge_ignore_limits_map;
std::unordered_map<cgraph_node *, std::pair<int, int>> node_caller_callee_count;

std::map<std::pair<std::string, int64_t>, int64_t> function_map;

/* ===------------------------- IPA-Inline-Pass -------------------------=== */
/* Return false when inlining edge E would lead to violating
   limits on function unit growth or stack usage growth.

   The relative function body growth limit is present generally
   to avoid problems with non-linear behavior of the compiler.
   To allow inlining huge functions into tiny wrapper, the limit
   is always based on the bigger of the two functions considered.

   For stack growth limits we always base the growth in stack usage
   of the callers.  We want to prevent applications from segfaulting
   on stack overflow when functions with huge stack frames gets
   inlined. */

static bool caller_growth_limits(struct cgraph_edge *e) {
  struct cgraph_node *to = e->caller;
  struct cgraph_node *what = e->callee->ultimate_alias_target();
  int newsize;
  int limit = 0;
  HOST_WIDE_INT stack_size_limit = 0, inlined_stack;
  ipa_size_summary *outer_info = ipa_size_summaries->get(to);

  /* Look for function e->caller is inlined to.  While doing
     so work out the largest function body on the way.  As
     described above, we want to base our function growth
     limits based on that.  Not on the self size of the
     outer function, not on the self size of inline code
     we immediately inline to.  This is the most relaxed
     interpretation of the rule "do not grow large functions
     too much in order to prevent compiler from exploding".  */
  while (true) {
    ipa_size_summary *size_info = ipa_size_summaries->get(to);
    if (limit < size_info->self_size) limit = size_info->self_size;
    if (stack_size_limit < size_info->estimated_self_stack_size)
      stack_size_limit = size_info->estimated_self_stack_size;
    if (to->inlined_to)
      to = to->callers->caller;
    else
      break;
  }

  ipa_fn_summary *what_info = ipa_fn_summaries->get(what);
  ipa_size_summary *what_size_info = ipa_size_summaries->get(what);

  if (limit < what_size_info->self_size) limit = what_size_info->self_size;

  limit += limit * opt_for_fn(to->decl, param_large_function_growth) / 100;

  /* Check the size after inlining against the function limits.  But allow
     the function to shrink if it went over the limits by forced inlining.  */
  newsize = estimate_size_after_inlining(to, e);
  if (newsize >= ipa_size_summaries->get(what)->size &&
      newsize > opt_for_fn(to->decl, param_large_function_insns) &&
      (newsize > limit &&
       (edge_ignore_limits_map.find(e) == edge_ignore_limits_map.end() ||
        !edge_ignore_limits_map[e]))) {
    e->inline_failed = CIF_LARGE_FUNCTION_GROWTH_LIMIT;
    return false;
  }

  if (!what_info->estimated_stack_size) return true;

  /* FIXME: Stack size limit often prevents inlining in Fortran programs
     due to large i/o datastructures used by the Fortran front-end.
     We ought to ignore this limit when we know that the edge is executed
     on every invocation of the caller (i.e. its call statement dominates
     exit block).  We do not track this information, yet.  */
  stack_size_limit += ((gcov_type)stack_size_limit *
                       opt_for_fn(to->decl, param_stack_frame_growth) / 100);

  inlined_stack =
      (ipa_get_stack_frame_offset(to) + outer_info->estimated_self_stack_size +
       what_info->estimated_stack_size);
  /* Check new stack consumption with stack consumption at the place
     stack is used.  */
  if (inlined_stack > stack_size_limit
      /* If function already has large stack usage from sibling
     inline call, we can inline, too.
     This bit overoptimistically assume that we are good at stack
     packing.  */
      && inlined_stack > ipa_fn_summaries->get(to)->estimated_stack_size &&
      (inlined_stack > opt_for_fn(to->decl, param_large_stack_frame) &&
       (edge_ignore_limits_map.find(e) == edge_ignore_limits_map.end() ||
        !edge_ignore_limits_map[e]))) {
    e->inline_failed = CIF_LARGE_STACK_FRAME_GROWTH_LIMIT;
    return false;
  }
  return true;
}

/* Dump info about why inlining has failed.  */

static void report_inline_failed_reason(struct cgraph_edge *e) {
  if (dump_enabled_p()) {
    dump_printf_loc(MSG_MISSED_OPTIMIZATION, e->call_stmt,
                    "  not inlinable: %C -> %C, %s\n", e->caller, e->callee,
                    cgraph_inline_failed_string(e->inline_failed));
    if ((e->inline_failed == CIF_TARGET_OPTION_MISMATCH ||
         e->inline_failed == CIF_OPTIMIZATION_MISMATCH) &&
        e->caller->lto_file_data &&
        e->callee->ultimate_alias_target()->lto_file_data) {
      dump_printf_loc(
          MSG_MISSED_OPTIMIZATION, e->call_stmt, "  LTO objects: %s, %s\n",
          e->caller->lto_file_data->file_name,
          e->callee->ultimate_alias_target()->lto_file_data->file_name);
    }
    if (e->inline_failed == CIF_TARGET_OPTION_MISMATCH)
      if (dump_file)
        cl_target_option_print_diff(
            dump_file, 2, target_opts_for_fn(e->caller->decl),
            target_opts_for_fn(e->callee->ultimate_alias_target()->decl));
    if (e->inline_failed == CIF_OPTIMIZATION_MISMATCH)
      if (dump_file)
        cl_optimization_print_diff(
            dump_file, 2, opts_for_fn(e->caller->decl),
            opts_for_fn(e->callee->ultimate_alias_target()->decl));
  }
}

/* Decide whether sanitizer-related attributes allow inlining. */

static bool sanitize_attrs_match_for_inline_p(const_tree caller,
                                              const_tree callee) {
  if (!caller || !callee) return true;

  /* Follow clang and allow inlining for always_inline functions.  */
  if (lookup_attribute("always_inline", DECL_ATTRIBUTES(callee))) return true;

  const sanitize_code codes[] = {
      SANITIZE_ADDRESS,         SANITIZE_THREAD,
      SANITIZE_UNDEFINED,       SANITIZE_UNDEFINED_NONDEFAULT,
      SANITIZE_POINTER_COMPARE, SANITIZE_POINTER_SUBTRACT};

  for (unsigned i = 0; i < sizeof(codes) / sizeof(codes[0]); i++)
    if (sanitize_flags_p(codes[i], caller) !=
        sanitize_flags_p(codes[i], callee))
      return false;

  if (sanitize_coverage_p(caller) != sanitize_coverage_p(callee)) return false;

  return true;
}

/* Used for flags where it is safe to inline when caller's value is
   grater than callee's.  */
#define check_maybe_up(flag)                                \
  (opts_for_fn(caller->decl)->x_##flag !=                   \
       opts_for_fn(callee->decl)->x_##flag &&               \
   (!always_inline || opts_for_fn(caller->decl)->x_##flag < \
                          opts_for_fn(callee->decl)->x_##flag))
/* Used for flags where it is safe to inline when caller's value is
   smaller than callee's.  */
#define check_maybe_down(flag)                              \
  (opts_for_fn(caller->decl)->x_##flag !=                   \
       opts_for_fn(callee->decl)->x_##flag &&               \
   (!always_inline || opts_for_fn(caller->decl)->x_##flag > \
                          opts_for_fn(callee->decl)->x_##flag))
/* Used for flags where exact match is needed for correctness.  */
#define check_match(flag) \
  (opts_for_fn(caller->decl)->x_##flag != opts_for_fn(callee->decl)->x_##flag)

/* Decide if we can inline the edge and possibly update
   inline_failed reason.
   We check whether inlining is possible at all and whether
   caller growth limits allow doing so.

   if REPORT is true, output reason to the dump file. */

static bool can_inline_edge_p(struct cgraph_edge *e, bool report,
                              bool early = false) {
  gcc_checking_assert(e->inline_failed);

  if (cgraph_inline_failed_type(e->inline_failed) == CIF_FINAL_ERROR) {
    if (report) report_inline_failed_reason(e);
    return false;
  }

  bool inlinable = true;
  enum availability avail;
  cgraph_node *caller =
      (e->caller->inlined_to ? e->caller->inlined_to : e->caller);
  cgraph_node *callee = e->callee->ultimate_alias_target(&avail, caller);

  if (!callee->definition) {
    e->inline_failed = CIF_BODY_NOT_AVAILABLE;
    inlinable = false;
  }
  if (!early && (!opt_for_fn(callee->decl, optimize) ||
                 !opt_for_fn(caller->decl, optimize))) {
    e->inline_failed = CIF_FUNCTION_NOT_OPTIMIZED;
    inlinable = false;
  } else if (callee->calls_comdat_local) {
    e->inline_failed = CIF_USES_COMDAT_LOCAL;
    inlinable = false;
  } else if (avail <= AVAIL_INTERPOSABLE) {
    e->inline_failed = CIF_OVERWRITABLE;
    inlinable = false;
  }
  /* All edges with call_stmt_cannot_inline_p should have inline_failed
     initialized to one of FINAL_ERROR reasons.  */
  else if (e->call_stmt_cannot_inline_p)
    gcc_unreachable();
  /* Don't inline if the functions have different EH personalities.  */
  else if (DECL_FUNCTION_PERSONALITY(caller->decl) &&
           DECL_FUNCTION_PERSONALITY(callee->decl) &&
           (DECL_FUNCTION_PERSONALITY(caller->decl) !=
            DECL_FUNCTION_PERSONALITY(callee->decl))) {
    e->inline_failed = CIF_EH_PERSONALITY;
    inlinable = false;
  }
  /* TM pure functions should not be inlined into non-TM_pure
     functions.  */
  else if (is_tm_pure(callee->decl) && !is_tm_pure(caller->decl)) {
    e->inline_failed = CIF_UNSPECIFIED;
    inlinable = false;
  }
  /* Check compatibility of target optimization options.  */
  else if (!targetm.target_option.can_inline_p(caller->decl, callee->decl)) {
    e->inline_failed = CIF_TARGET_OPTION_MISMATCH;
    inlinable = false;
  } else if (ipa_fn_summaries->get(callee) == NULL ||
             !ipa_fn_summaries->get(callee)->inlinable) {
    e->inline_failed = CIF_FUNCTION_NOT_INLINABLE;
    inlinable = false;
  }
  /* Don't inline a function with mismatched sanitization attributes. */
  else if (!sanitize_attrs_match_for_inline_p(caller->decl, callee->decl)) {
    e->inline_failed = CIF_SANITIZE_ATTRIBUTE_MISMATCH;
    inlinable = false;
  }

  if (!inlinable && report) report_inline_failed_reason(e);
  return inlinable;
}

/* Return inlining_insns_single limit for function N.  If HINT or HINT2 is true
   scale up the bound.  */

static int inline_insns_single(cgraph_node *n, bool hint, bool hint2) {
  if (hint && hint2) {
    int64_t spd = opt_for_fn(n->decl, param_inline_heuristics_hint_percent);
    spd = spd * spd;
    if (spd > 1000000) spd = 1000000;
    return opt_for_fn(n->decl, param_max_inline_insns_single) * spd / 100;
  }
  if (hint || hint2)
    return opt_for_fn(n->decl, param_max_inline_insns_single) *
           opt_for_fn(n->decl, param_inline_heuristics_hint_percent) / 100;
  return opt_for_fn(n->decl, param_max_inline_insns_single);
}

/* Return inlining_insns_auto limit for function N.  If HINT or HINT2 is true
   scale up the bound.   */

static int inline_insns_auto(cgraph_node *n, bool hint, bool hint2) {
  int max_inline_insns_auto = opt_for_fn(n->decl, param_max_inline_insns_auto);
  if (hint && hint2) {
    int64_t spd = opt_for_fn(n->decl, param_inline_heuristics_hint_percent);
    spd = spd * spd;
    if (spd > 1000000) spd = 1000000;
    return max_inline_insns_auto * spd / 100;
  }
  if (hint || hint2)
    return max_inline_insns_auto *
           opt_for_fn(n->decl, param_inline_heuristics_hint_percent) / 100;
  return max_inline_insns_auto;
}

/* Decide if we can inline the edge and possibly update
   inline_failed reason.
   We check whether inlining is possible at all and whether
   caller growth limits allow doing so.

   if REPORT is true, output reason to the dump file.

   if DISREGARD_LIMITS is true, ignore size limits.  */

static bool can_inline_edge_by_limits_p(struct cgraph_edge *e, bool report,
                                        bool disregard_limits = false,
                                        bool early = false) {
  gcc_checking_assert(e->inline_failed);

  if (cgraph_inline_failed_type(e->inline_failed) == CIF_FINAL_ERROR) {
    if (report) report_inline_failed_reason(e);
    return false;
  }

  bool inlinable = true;
  enum availability avail;
  cgraph_node *caller =
      (e->caller->inlined_to ? e->caller->inlined_to : e->caller);
  cgraph_node *callee = e->callee->ultimate_alias_target(&avail, caller);
  tree caller_tree = DECL_FUNCTION_SPECIFIC_OPTIMIZATION(caller->decl);
  tree callee_tree =
      callee ? DECL_FUNCTION_SPECIFIC_OPTIMIZATION(callee->decl) : NULL;
  /* Check if caller growth allows the inlining.  */
  if (!DECL_DISREGARD_INLINE_LIMITS(callee->decl) && !disregard_limits &&
      !lookup_attribute("flatten", DECL_ATTRIBUTES(caller->decl)) &&
      !caller_growth_limits(e))
    inlinable = false;
  else if (callee->externally_visible &&
           !DECL_DISREGARD_INLINE_LIMITS(callee->decl) &&
           flag_live_patching == LIVE_PATCHING_INLINE_ONLY_STATIC) {
    e->inline_failed = CIF_EXTERN_LIVE_ONLY_STATIC;
    inlinable = false;
  }
  /* Don't inline a function with a higher optimization level than the
     caller.  FIXME: this is really just tip of iceberg of handling
     optimization attribute.  */
  else if (caller_tree != callee_tree) {
    bool always_inline =
        (DECL_DISREGARD_INLINE_LIMITS(callee->decl) &&
         lookup_attribute("always_inline", DECL_ATTRIBUTES(callee->decl)));
    ipa_fn_summary *caller_info = ipa_fn_summaries->get(caller);
    ipa_fn_summary *callee_info = ipa_fn_summaries->get(callee);

    /* Until GCC 4.9 we did not check the semantics-altering flags
   below and inlined across optimization boundaries.
   Enabling checks below breaks several packages by refusing
   to inline library always_inline functions. See PR65873.
   Disable the check for early inlining for now until better solution
   is found.  */
    if (always_inline && early)
      ;
    /* There are some options that change IL semantics which means
       we cannot inline in these cases for correctness reason.
   Not even for always_inline declared functions.  */
    else if (check_match(flag_wrapv) || check_match(flag_trapv) ||
             check_match(flag_pcc_struct_return) ||
             check_maybe_down(optimize_debug)
             /* When caller or callee does FP math, be sure FP codegen flags
            compatible.  */
             || ((caller_info->fp_expressions && callee_info->fp_expressions) &&
                 (check_maybe_up(flag_rounding_math) ||
                  check_maybe_up(flag_trapping_math) ||
                  check_maybe_down(flag_unsafe_math_optimizations) ||
                  check_maybe_down(flag_finite_math_only) ||
                  check_maybe_up(flag_signaling_nans) ||
                  check_maybe_down(flag_cx_limited_range) ||
                  check_maybe_up(flag_signed_zeros) ||
                  check_maybe_down(flag_associative_math) ||
                  check_maybe_down(flag_reciprocal_math) ||
                  check_maybe_down(flag_fp_int_builtin_inexact)
                  /* Strictly speaking only when the callee contains function
                 calls that may end up setting errno.  */
                  || check_maybe_up(flag_errno_math)))
             /* We do not want to make code compiled with exceptions to be
            brought into a non-EH function unless we know that the callee
            does not throw.
            This is tracked by DECL_FUNCTION_PERSONALITY.  */
             || (check_maybe_up(flag_non_call_exceptions) &&
                 DECL_FUNCTION_PERSONALITY(callee->decl)) ||
             (check_maybe_up(flag_exceptions) &&
              DECL_FUNCTION_PERSONALITY(callee->decl))
             /* When devirtualization is disabled for callee, it is not safe
            to inline it as we possibly mangled the type info.
            Allow early inlining of always inlines.  */
             || (!early && check_maybe_down(flag_devirtualize))) {
      e->inline_failed = CIF_OPTIMIZATION_MISMATCH;
      inlinable = false;
    }
    /* gcc.dg/pr43564.c.  Apply user-forced inline even at -O0.  */
    else if (always_inline)
      ;
    /* When user added an attribute to the callee honor it.  */
    else if (lookup_attribute("optimize", DECL_ATTRIBUTES(callee->decl)) &&
             opts_for_fn(caller->decl) != opts_for_fn(callee->decl)) {
      e->inline_failed = CIF_OPTIMIZATION_MISMATCH;
      inlinable = false;
    }
    /* If explicit optimize attribute are not used, the mismatch is caused
   by different command line options used to build different units.
   Do not care about COMDAT functions - those are intended to be
       optimized with the optimization flags of module they are used in.
   Also do not care about mixing up size/speed optimization when
   DECL_DISREGARD_INLINE_LIMITS is set.  */
    else if ((callee->merged_comdat &&
              !lookup_attribute("optimize", DECL_ATTRIBUTES(caller->decl))) ||
             DECL_DISREGARD_INLINE_LIMITS(callee->decl))
      ;
    /* If mismatch is caused by merging two LTO units with different
   optimization flags we want to be bit nicer.  However never inline
   if one of functions is not optimized at all.  */
    else if (!opt_for_fn(callee->decl, optimize) ||
             !opt_for_fn(caller->decl, optimize)) {
      e->inline_failed = CIF_OPTIMIZATION_MISMATCH;
      inlinable = false;
    }
    /* If callee is optimized for size and caller is not, allow inlining if
   code shrinks or we are in param_max_inline_insns_single limit and
   callee is inline (and thus likely an unified comdat).
   This will allow caller to run faster.  */
    else if (opt_for_fn(callee->decl, optimize_size) >
             opt_for_fn(caller->decl, optimize_size)) {
      int growth = estimate_edge_growth(e);
      if (growth > opt_for_fn(caller->decl, param_max_inline_insns_size) &&
          (!DECL_DECLARED_INLINE_P(callee->decl) &&
           (growth >= MAX(inline_insns_single(caller, false, false),
                          inline_insns_auto(caller, false, false)) &&
            (edge_ignore_limits_map.find(e) == edge_ignore_limits_map.end() ||
             !edge_ignore_limits_map[e])))) {
        e->inline_failed = CIF_OPTIMIZATION_MISMATCH;
        inlinable = false;
      }
    }
    /* If callee is more aggressively optimized for performance than caller,
   we generally want to inline only cheap (runtime wise) functions.  */
    else if (opt_for_fn(callee->decl, optimize_size) <
                 opt_for_fn(caller->decl, optimize_size) ||
             (opt_for_fn(callee->decl, optimize) >
              opt_for_fn(caller->decl, optimize))) {
      if (estimate_edge_time(e) >=
          20 + ipa_call_summaries->get(e)->call_stmt_time) {
        e->inline_failed = CIF_OPTIMIZATION_MISMATCH;
        inlinable = false;
      }
    }
  }

  if (!inlinable && report) report_inline_failed_reason(e);
  return inlinable;
}

/* Return true if the edge E is inlinable during early inlining.  */

static bool can_early_inline_edge_p(struct cgraph_edge *e) {
  cgraph_node *caller =
      (e->caller->inlined_to ? e->caller->inlined_to : e->caller);
  struct cgraph_node *callee = e->callee->ultimate_alias_target();
  /* Early inliner might get called at WPA stage when IPA pass adds new
     function.  In this case we cannot really do any of early inlining
     because function bodies are missing.  */
  if (cgraph_inline_failed_type(e->inline_failed) == CIF_FINAL_ERROR)
    return false;
  if (!gimple_has_body_p(callee->decl)) {
    e->inline_failed = CIF_BODY_NOT_AVAILABLE;
    return false;
  }
  /* In early inliner some of callees may not be in SSA form yet
     (i.e. the callgraph is cyclic and we did not process
     the callee by early inliner, yet).  We don't have CIF code for this
     case; later we will re-do the decision in the real inliner.  */
  if (!gimple_in_ssa_p(DECL_STRUCT_FUNCTION(e->caller->decl)) ||
      !gimple_in_ssa_p(DECL_STRUCT_FUNCTION(callee->decl))) {
    if (dump_enabled_p())
      dump_printf_loc(MSG_MISSED_OPTIMIZATION, e->call_stmt,
                      "  edge not inlinable: not in SSA form\n");
    return false;
  } else if (profile_arc_flag &&
             ((lookup_attribute("no_profile_instrument_function",
                                DECL_ATTRIBUTES(caller->decl)) == NULL_TREE) !=
              (lookup_attribute("no_profile_instrument_function",
                                DECL_ATTRIBUTES(callee->decl)) == NULL_TREE)))
    return false;

  if (!can_inline_edge_p(e, true, true) ||
      !can_inline_edge_by_limits_p(e, true, false, true))
    return false;
  return true;
}

/* Return number of calls in N.  Ignore cheap builtins.  */

static int num_calls(struct cgraph_node *n) {
  struct cgraph_edge *e;
  int num = 0;

  for (e = n->callees; e; e = e->next_callee)
    if (!is_inexpensive_builtin(e->callee->decl)) num++;
  return num;
}

/* Compute time of the edge->caller + edge->callee execution when inlining
   does not happen.  */

inline sreal compute_uninlined_call_time(struct cgraph_edge *edge,
                                         sreal uninlined_call_time,
                                         sreal freq) {
  cgraph_node *caller =
      (edge->caller->inlined_to ? edge->caller->inlined_to : edge->caller);

  if (freq > 0)
    uninlined_call_time *= freq;
  else
    uninlined_call_time = uninlined_call_time >> 11;

  sreal caller_time = ipa_fn_summaries->get(caller)->time;
  return uninlined_call_time + caller_time;
}

/* Same as compute_uinlined_call_time but compute time when inlining
   does happen.  */

inline sreal compute_inlined_call_time(struct cgraph_edge *edge, sreal time,
                                       sreal freq) {
  cgraph_node *caller =
      (edge->caller->inlined_to ? edge->caller->inlined_to : edge->caller);
  sreal caller_time = ipa_fn_summaries->get(caller)->time;

  if (freq > 0)
    time *= freq;
  else
    time = time >> 11;

  /* This calculation should match one in ipa-inline-analysis.cc
     (estimate_edge_size_and_time).  */
  time -= (sreal)ipa_call_summaries->get(edge)->call_stmt_time * freq;
  time += caller_time;
  if (time <= 0) time = ((sreal)1) >> 8;
  gcc_checking_assert(time >= 0);
  return time;
}

/* Determine time saved by inlining EDGE of frequency FREQ
   where callee's runtime w/o inlining is UNINLINED_TYPE
   and with inlined is INLINED_TYPE.  */

inline sreal inlining_speedup(struct cgraph_edge *edge, sreal freq,
                              sreal uninlined_time, sreal inlined_time) {
  sreal speedup = uninlined_time - inlined_time;
  /* Handling of call_time should match one in ipa-inline-fnsummary.c
     (estimate_edge_size_and_time).  */
  sreal call_time = ipa_call_summaries->get(edge)->call_stmt_time;

  if (freq > 0) {
    speedup = (speedup + call_time);
    if (freq != 1) speedup = speedup * freq;
  } else if (freq == 0)
    speedup = speedup >> 11;
  gcc_checking_assert(speedup >= 0);
  return speedup;
}

/* Return true if the speedup for inlining E is bigger than
   param_inline_min_speedup.  */

static bool big_speedup_p(struct cgraph_edge *e) {
  sreal unspec_time;
  sreal spec_time = estimate_edge_time(e, &unspec_time);
  sreal freq = e->sreal_frequency();
  sreal time = compute_uninlined_call_time(e, unspec_time, freq);
  sreal inlined_time = compute_inlined_call_time(e, spec_time, freq);
  cgraph_node *caller =
      (e->caller->inlined_to ? e->caller->inlined_to : e->caller);
  int limit = opt_for_fn(caller->decl, param_inline_min_speedup);

  if ((time - inlined_time) * 100 > time * limit) return true;
  return false;
}

/* Return true if we are interested in inlining small function.
   When REPORT is true, report reason to dump file.  */

static bool want_inline_small_function_p(struct cgraph_edge *e, bool report) {
  bool want_inline = true;
  struct cgraph_node *callee = e->callee->ultimate_alias_target();
  cgraph_node *to = (e->caller->inlined_to ? e->caller->inlined_to : e->caller);

  /* Allow this function to be called before can_inline_edge_p,
     since it's usually cheaper.  */
  if (cgraph_inline_failed_type(e->inline_failed) == CIF_FINAL_ERROR)
    want_inline = false;
  else if (DECL_DISREGARD_INLINE_LIMITS(callee->decl))
    ;
  else if (!DECL_DECLARED_INLINE_P(callee->decl) &&
           !opt_for_fn(e->caller->decl, flag_inline_small_functions)) {
    e->inline_failed = CIF_FUNCTION_NOT_INLINE_CANDIDATE;
    want_inline = false;
  }
  /* Do fast and conservative check if the function can be good
     inline candidate.  */
  else if ((!DECL_DECLARED_INLINE_P(callee->decl) &&
            (!e->count.ipa().initialized_p() || !e->maybe_hot_p())) &&
           (ipa_fn_summaries->get(callee)->min_size -
                    ipa_call_summaries->get(e)->call_stmt_size >
                inline_insns_auto(e->caller, true, true) &&
            (edge_ignore_limits_map.find(e) == edge_ignore_limits_map.end() ||
             !edge_ignore_limits_map[e]))) {
    e->inline_failed = CIF_MAX_INLINE_INSNS_AUTO_LIMIT;
    want_inline = false;
  } else if ((DECL_DECLARED_INLINE_P(callee->decl) ||
              e->count.ipa().nonzero_p()) &&
             (ipa_fn_summaries->get(callee)->min_size -
                  ipa_call_summaries->get(e)->call_stmt_size >
              inline_insns_single(e->caller, true, true))) {
    e->inline_failed = (DECL_DECLARED_INLINE_P(callee->decl)
                            ? CIF_MAX_INLINE_INSNS_SINGLE_LIMIT
                            : CIF_MAX_INLINE_INSNS_AUTO_LIMIT);
    want_inline = false;
  } else {
    int growth = estimate_edge_growth(e);
    ipa_hints hints = estimate_edge_hints(e);
    /* We have two independent groups of hints.  If one matches in each
   of groups the limits are inreased.  If both groups matches, limit
   is increased even more.  */
    bool apply_hints =
        (hints & (INLINE_HINT_indirect_call | INLINE_HINT_known_hot |
                  INLINE_HINT_loop_iterations | INLINE_HINT_loop_stride));
    bool apply_hints2 = (hints & INLINE_HINT_builtin_constant_p);

    if (growth <= opt_for_fn(to->decl, param_max_inline_insns_size))
      ;
    /* Apply param_max_inline_insns_single limit.  Do not do so when
   hints suggests that inlining given function is very profitable.
   Avoid computation of big_speedup_p when not necessary to change
   outcome of decision.  */
    else if (DECL_DECLARED_INLINE_P(callee->decl) &&
             (growth >=
              inline_insns_single(e->caller, apply_hints, apply_hints2)) &&
             (apply_hints || apply_hints2 ||
              growth >= inline_insns_single(e->caller, true, apply_hints2) ||
              !big_speedup_p(e))) {
      e->inline_failed = CIF_MAX_INLINE_INSNS_SINGLE_LIMIT;
      want_inline = false;
    } else if (!DECL_DECLARED_INLINE_P(callee->decl) &&
               !opt_for_fn(e->caller->decl, flag_inline_functions) &&
               growth >= opt_for_fn(to->decl, param_max_inline_insns_small)) {
      /* growth_positive_p is expensive, always test it last.  */
      if ((growth >= inline_insns_single(e->caller, false, false)) ||
          growth_positive_p(callee, e, growth)) {
        e->inline_failed = CIF_NOT_DECLARED_INLINED;
        want_inline = false;
      }
    }
    /* Apply param_max_inline_insns_auto limit for functions not declared
   inline.  Bypass the limit when speedup seems big.  */
    else if (!DECL_DECLARED_INLINE_P(callee->decl) &&
             (growth >=
                  inline_insns_auto(e->caller, apply_hints, apply_hints2) &&
              (edge_ignore_limits_map.find(e) == edge_ignore_limits_map.end() ||
               !edge_ignore_limits_map[e])) &&
             (apply_hints || apply_hints2 ||
              (growth >= inline_insns_auto(e->caller, true, apply_hints2) &&
               (edge_ignore_limits_map.find(e) ==
                    edge_ignore_limits_map.end() ||
                !edge_ignore_limits_map[e])) ||
              !big_speedup_p(e))) {
      /* growth_positive_p is expensive, always test it last.  */
      if ((growth >= inline_insns_single(e->caller, false, false)) ||
          growth_positive_p(callee, e, growth)) {
        e->inline_failed = CIF_MAX_INLINE_INSNS_AUTO_LIMIT;
        want_inline = false;
      }
    }
    /* If call is cold, do not inline when function body would grow. */
    else if (!e->maybe_hot_p() &&
             ((growth >= inline_insns_single(e->caller, false, false)) ||
              growth_positive_p(callee, e, growth)) &&
             !e->count.ipa().to_gcov_type() > param_hot_bb_count_fraction) {
      e->inline_failed = CIF_UNLIKELY_CALL;
      want_inline = false;
    }
  }
  if (!want_inline && report) report_inline_failed_reason(e);
  return want_inline;
}

/* EDGE is self recursive edge.
   We handle two cases - when function A is inlining into itself
   or when function A is being inlined into another inliner copy of function
   A within function B.

   In first case OUTER_NODE points to the toplevel copy of A, while
   in the second case OUTER_NODE points to the outermost copy of A in B.

   In both cases we want to be extra selective since
   inlining the call will just introduce new recursive calls to appear.  */

static bool want_inline_self_recursive_call_p(struct cgraph_edge *edge,
                                              struct cgraph_node *outer_node,
                                              bool peeling, int depth) {
  char const *reason = NULL;
  bool want_inline = true;
  sreal caller_freq = 1;
  int max_depth =
      opt_for_fn(outer_node->decl, param_max_inline_recursive_depth_auto);

  if (DECL_DECLARED_INLINE_P(edge->caller->decl))
    max_depth = opt_for_fn(outer_node->decl, param_max_inline_recursive_depth);

  if (!edge->maybe_hot_p()) {
    reason = "recursive call is cold";
    want_inline = false;
  } else if (depth > max_depth) {
    reason = "--param max-inline-recursive-depth exceeded.";
    want_inline = false;
  } else if (outer_node->inlined_to &&
             (caller_freq = outer_node->callers->sreal_frequency()) == 0) {
    reason = "caller frequency is 0";
    want_inline = false;
  }

  if (!want_inline)
    ;
  /* Inlining of self recursive function into copy of itself within other
     function is transformation similar to loop peeling.

     Peeling is profitable if we can inline enough copies to make probability
     of actual call to the self recursive function very small.  Be sure that
     the probability of recursion is small.

     We ensure that the frequency of recursing is at most 1 - (1/max_depth).
     This way the expected number of recursion is at most max_depth.  */
  else if (peeling) {
    sreal max_prob = (sreal)1 - ((sreal)1 / (sreal)max_depth);
    int i;
    for (i = 1; i < depth; i++) max_prob = max_prob * max_prob;
    if (edge->sreal_frequency() >= max_prob * caller_freq) {
      reason = "frequency of recursive call is too large";
      want_inline = false;
    }
  }
  /* Recursive inlining, i.e. equivalent of unrolling, is profitable if
     recursion depth is large.  We reduce function call overhead and increase
     chances that things fit in hardware return predictor.

     Recursive inlining might however increase cost of stack frame setup
     actually slowing down functions whose recursion tree is wide rather than
     deep.

     Deciding reliably on when to do recursive inlining without profile feedback
     is tricky.  For now we disable recursive inlining when probability of self
     recursion is low.

     Recursive inlining of self recursive call within loop also results in
     large loop depths that generally optimize badly.  We may want to throttle
     down inlining in those cases.  In particular this seems to happen in one
     of libstdc++ rb tree methods.  */
  else {
    if (edge->sreal_frequency() * 100 <=
        caller_freq * opt_for_fn(outer_node->decl,
                                 param_min_inline_recursive_probability)) {
      reason = "frequency of recursive call is too small";
      want_inline = false;
    }
  }
  if (!want_inline && dump_enabled_p())
    dump_printf_loc(MSG_MISSED_OPTIMIZATION, edge->call_stmt,
                    "   not inlining recursively: %s\n", reason);
  return want_inline;
}

/* Return true when NODE has uninlinable caller;
   set HAS_HOT_CALL if it has hot call.
   Worker for cgraph_for_node_and_aliases.  */

static bool check_callers(struct cgraph_node *node, void *has_hot_call) {
  struct cgraph_edge *e;
  for (e = node->callers; e; e = e->next_caller) {
    if (!opt_for_fn(e->caller->decl, flag_inline_functions_called_once) ||
        !opt_for_fn(e->caller->decl, optimize))
      return true;
    if (!can_inline_edge_p(e, true)) return true;
    if (e->recursive_p()) return true;
    if (!can_inline_edge_by_limits_p(e, true)) return true;
    /* Inlining large functions to large loop depth is often harmful because
   of register pressure it implies.  */
    if ((int)ipa_call_summaries->get(e)->loop_depth >
        param_inline_functions_called_once_loop_depth)
      return true;
    /* Do not produce gigantic functions.  */
    if (estimate_size_after_inlining(
            e->caller->inlined_to ? e->caller->inlined_to : e->caller, e) >
        param_inline_functions_called_once_insns)
      return true;
    if (!(*(bool *)has_hot_call) && e->maybe_hot_p())
      *(bool *)has_hot_call = true;
  }
  return false;
}

/* If NODE has a caller, return true.  */

static bool has_caller_p(struct cgraph_node *node,
                         void *data ATTRIBUTE_UNUSED) {
  if (node->callers) return true;
  return false;
}

/* Decide if inlining NODE would reduce unit size by eliminating
   the offline copy of function.
   When COLD is true the cold calls are considered, too.  */

static bool want_inline_function_to_all_callers_p(struct cgraph_node *node,
                                                  bool cold) {
  bool has_hot_call = false;

  /* Aliases gets inlined along with the function they alias.  */
  if (node->alias) return false;
  /* Already inlined?  */
  if (node->inlined_to) return false;
  /* Does it have callers?  */
  if (!node->call_for_symbol_and_aliases(has_caller_p, NULL, true))
    return false;
  /* Inlining into all callers would increase size?  */
  if (growth_positive_p(node, NULL, INT_MIN) > 0) return false;
  /* All inlines must be possible.  */
  if (node->call_for_symbol_and_aliases(check_callers, &has_hot_call, true))
    return false;
  if (!cold && !has_hot_call) return false;
  return true;
}

/* Return true if WHERE of SIZE is a possible candidate for wrapper heuristics
   in estimate_edge_badness.  */

static bool wrapper_heuristics_may_apply(struct cgraph_node *where, int size) {
  return size < (DECL_DECLARED_INLINE_P(where->decl)
                     ? inline_insns_single(where, false, false)
                     : inline_insns_auto(where, false, false));
}

/* A cost model driving the inlining heuristics in a way so the edges with
   smallest badness are inlined first.  After each inlining is performed
   the costs of all caller edges of nodes affected are recomputed so the
   metrics may accurately depend on values such as number of inlinable callers
   of the function or function body size.  */

static sreal edge_badness(struct cgraph_edge *edge, bool dump) {
  sreal badness;
  int growth;
  sreal edge_time, unspec_edge_time;
  struct cgraph_node *callee = edge->callee->ultimate_alias_target();
  class ipa_fn_summary *callee_info = ipa_fn_summaries->get(callee);
  ipa_hints hints;
  cgraph_node *caller =
      (edge->caller->inlined_to ? edge->caller->inlined_to : edge->caller);

  growth = estimate_edge_growth(edge);
  edge_time = estimate_edge_time(edge, &unspec_edge_time);
  hints = estimate_edge_hints(edge);
  gcc_checking_assert(edge_time >= 0);
  /* Check that inlined time is better, but tolerate some roundoff issues.
     FIXME: When callee profile drops to 0 we account calls more.  This
     should be fixed by never doing that.  */
  gcc_checking_assert((edge_time * 100 - callee_info->time * 101).to_int() <=
                          0 ||
                      callee->count.ipa().initialized_p());
  gcc_checking_assert(growth <= ipa_size_summaries->get(callee)->size);

  if (dump) {
    fprintf(dump_file, "    Badness calculation for %s -> %s\n",
            edge->caller->dump_name(), edge->callee->dump_name());
    fprintf(dump_file, "      size growth %i, time %f unspec %f ", growth,
            edge_time.to_double(), unspec_edge_time.to_double());
    ipa_dump_hints(dump_file, hints);
    if (big_speedup_p(edge)) fprintf(dump_file, " big_speedup");
    fprintf(dump_file, "\n");
  }

  /* Always prefer inlining saving code size.  */
  if (growth <= 0) {
    badness = (sreal)(-SREAL_MIN_SIG + growth) << (SREAL_MAX_EXP / 256);
    if (dump)
      fprintf(dump_file, "      %f: Growth %d <= 0\n", badness.to_double(),
              growth);
  }
  /* Inlining into EXTERNAL functions is not going to change anything unless
     they are themselves inlined.  */
  else if (DECL_EXTERNAL(caller->decl)) {
    if (dump) fprintf(dump_file, "      max: function is external\n");
    return sreal::max();
  }
  /* When profile is available. Compute badness as:

                 time_saved * caller_count
     goodness =  -------------------------------------------------
             growth_of_caller * overall_growth * combined_size

     badness = - goodness

     Again use negative value to make calls with profile appear hotter
     then calls without.
  */
  else if (opt_for_fn(caller->decl, flag_guess_branch_prob) ||
           caller->count.ipa().nonzero_p()) {
    sreal numerator, denominator;
    int overall_growth;
    sreal freq = edge->sreal_frequency();

    numerator = inlining_speedup(edge, freq, unspec_edge_time, edge_time);
    if (numerator <= 0) numerator = ((sreal)1 >> 8);
    if (caller->count.ipa().nonzero_p())
      numerator *= caller->count.ipa().to_gcov_type();
    else if (caller->count.ipa().initialized_p())
      numerator = numerator >> 11;
    denominator = growth;

    overall_growth = callee_info->growth;

    /* Look for inliner wrappers of the form:

   inline_caller ()
     {
       do_fast_job...
       if (need_more_work)
         noninline_callee ();
     }
   Without penalizing this case, we usually inline noninline_callee
   into the inline_caller because overall_growth is small preventing
   further inlining of inline_caller.

   Penalize only callgraph edges to functions with small overall
   growth ...
  */
    if (growth > overall_growth
        /* ... and having only one caller which is not inlined ... */
        && callee_info->single_caller &&
        !edge->caller->inlined_to
        /* ... and edges executed only conditionally ... */
        && freq < 1
        /* ... consider case where callee is not inline but caller is ... */
        &&
        ((!DECL_DECLARED_INLINE_P(edge->callee->decl) &&
          DECL_DECLARED_INLINE_P(caller->decl))
         /* ... or when early optimizers decided to split and edge
        frequency still indicates splitting is a win ... */
         || (callee->split_part && !caller->split_part &&
             freq * 100 < opt_for_fn(caller->decl,
                                     param_partial_inlining_entry_probability)
             /* ... and do not overwrite user specified hints.   */
             && (!DECL_DECLARED_INLINE_P(edge->callee->decl) ||
                 DECL_DECLARED_INLINE_P(caller->decl))))) {
      ipa_fn_summary *caller_info = ipa_fn_summaries->get(caller);
      int caller_growth = caller_info->growth;

      /* Only apply the penalty when caller looks like inline candidate,
         and it is not called once.  */
      if (!caller_info->single_caller && overall_growth < caller_growth &&
          caller_info->inlinable &&
          wrapper_heuristics_may_apply(caller,
                                       ipa_size_summaries->get(caller)->size)) {
        if (dump)
          fprintf(dump_file,
                  "     Wrapper penalty. Increasing growth %i to %i\n",
                  overall_growth, caller_growth);
        overall_growth = caller_growth;
      }
    }
    if (overall_growth > 0) {
      /* Strongly prefer functions with few callers that can be inlined
         fully.  The square root here leads to smaller binaries at average.
         Watch however for extreme cases and return to linear function
         when growth is large.  */
      if (overall_growth < 256)
        overall_growth *= overall_growth;
      else
        overall_growth += 256 * 256 - 256;
      denominator *= overall_growth;
    }
    denominator *= ipa_size_summaries->get(caller)->size + growth;

    badness = -numerator / denominator;

    if (dump) {
      fprintf(
          dump_file,
          "      %f: guessed profile. frequency %f, count %" PRId64
          " caller count %" PRId64
          " time saved %f"
          " overall growth %i (current) %i (original)"
          " %i (compensated)\n",
          badness.to_double(), freq.to_double(),
          edge->count.ipa().initialized_p() ? edge->count.ipa().to_gcov_type()
                                            : -1,
          caller->count.ipa().initialized_p()
              ? caller->count.ipa().to_gcov_type()
              : -1,
          inlining_speedup(edge, freq, unspec_edge_time, edge_time).to_double(),
          estimate_growth(callee), callee_info->growth, overall_growth);
    }
  }
  /* When function local profile is not available or it does not give
     useful information (i.e. frequency is zero), base the cost on
     loop nest and overall size growth, so we optimize for overall number
     of functions fully inlined in program.  */
  else {
    int nest = MIN(ipa_call_summaries->get(edge)->loop_depth, 8);
    badness = growth;

    /* Decrease badness if call is nested.  */
    if (badness > 0)
      badness = badness >> nest;
    else
      badness = badness << nest;
    if (dump)
      fprintf(dump_file, "      %f: no profile. nest %i\n", badness.to_double(),
              nest);
  }
  gcc_checking_assert(badness != 0);

  if (edge->recursive_p()) badness = badness.shift(badness > 0 ? 4 : -4);
  if ((hints & (INLINE_HINT_indirect_call | INLINE_HINT_loop_iterations |
                INLINE_HINT_loop_stride)) ||
      callee_info->growth <= 0)
    badness = badness.shift(badness > 0 ? -2 : 2);
  if (hints & INLINE_HINT_builtin_constant_p)
    badness = badness.shift(badness > 0 ? -4 : 4);
  if (hints & (INLINE_HINT_same_scc))
    badness = badness.shift(badness > 0 ? 3 : -3);
  else if (hints & (INLINE_HINT_in_scc))
    badness = badness.shift(badness > 0 ? 2 : -2);
  else if (hints & (INLINE_HINT_cross_module))
    badness = badness.shift(badness > 0 ? 1 : -1);
  if (DECL_DISREGARD_INLINE_LIMITS(callee->decl))
    badness = badness.shift(badness > 0 ? -4 : 4);
  else if ((hints & INLINE_HINT_declared_inline))
    badness = badness.shift(badness > 0 ? -3 : 3);
  if (dump)
    fprintf(dump_file, "      Adjusted by hints %f\n", badness.to_double());
  return badness;
}

/* Recompute badness of EDGE and update its key in HEAP if needed.  */
static inline void update_edge_key(edge_heap_t *heap,
                                   struct cgraph_edge *edge) {
  sreal badness = edge_badness(edge, false);
  if (edge->aux) {
    edge_heap_node_t *n = (edge_heap_node_t *)edge->aux;
    gcc_checking_assert(n->get_data() == edge);

    /* fibonacci_heap::replace_key does busy updating of the
   heap that is unnecessarily expensive.
   We do lazy increases: after extracting minimum if the key
   turns out to be out of date, it is re-inserted into heap
   with correct value.  */
    if (badness < n->get_key()) {
      if (dump_file && (dump_flags & TDF_DETAILS)) {
        fprintf(dump_file, "  decreasing badness %s -> %s, %f to %f\n",
                edge->caller->dump_name(), edge->callee->dump_name(),
                n->get_key().to_double(), badness.to_double());
      }
      heap->decrease_key(n, badness);
    }
  } else {
    if (dump_file && (dump_flags & TDF_DETAILS)) {
      fprintf(dump_file, "  enqueuing call %s -> %s, badness %f\n",
              edge->caller->dump_name(), edge->callee->dump_name(),
              badness.to_double());
    }
    edge->aux = heap->insert(badness, edge);
  }
}

/* NODE was inlined.
   All caller edges needs to be reset because
   size estimates change. Similarly callees needs reset
   because better context may be known.  */

static void reset_edge_caches(struct cgraph_node *node) {
  struct cgraph_edge *edge;
  struct cgraph_edge *e = node->callees;
  struct cgraph_node *where = node;
  struct ipa_ref *ref;

  if (where->inlined_to) where = where->inlined_to;

  reset_node_cache(where);

  if (edge_growth_cache != NULL)
    for (edge = where->callers; edge; edge = edge->next_caller)
      if (edge->inline_failed) edge_growth_cache->remove(edge);

  FOR_EACH_ALIAS(where, ref)
  reset_edge_caches(dyn_cast<cgraph_node *>(ref->referring));

  if (!e) return;

  while (true)
    if (!e->inline_failed && e->callee->callees)
      e = e->callee->callees;
    else {
      if (edge_growth_cache != NULL && e->inline_failed)
        edge_growth_cache->remove(e);
      if (e->next_callee)
        e = e->next_callee;
      else {
        do {
          if (e->caller == node) return;
          e = e->caller->callers;
        } while (!e->next_callee);
        e = e->next_callee;
      }
    }
}

/* Recompute HEAP nodes for each of caller of NODE.
   UPDATED_NODES track nodes we already visited, to avoid redundant work.
   When CHECK_INLINABLITY_FOR is set, re-check for specified edge that
   it is inlinable. Otherwise check all edges.  */

static void update_caller_keys(edge_heap_t *heap, struct cgraph_node *node,
                               bitmap updated_nodes,
                               struct cgraph_edge *check_inlinablity_for) {
  struct cgraph_edge *edge;
  struct ipa_ref *ref;

  if ((!node->alias && !ipa_fn_summaries->get(node)->inlinable) ||
      node->inlined_to)
    return;
  if (!bitmap_set_bit(updated_nodes, node->get_uid())) return;

  FOR_EACH_ALIAS(node, ref) {
    struct cgraph_node *alias = dyn_cast<cgraph_node *>(ref->referring);
    update_caller_keys(heap, alias, updated_nodes, check_inlinablity_for);
  }

  for (edge = node->callers; edge; edge = edge->next_caller)
    if (edge->inline_failed) {
      if (!check_inlinablity_for || check_inlinablity_for == edge) {
        if (can_inline_edge_p(edge, false) &&
            want_inline_small_function_p(edge, false) &&
            can_inline_edge_by_limits_p(edge, false))
          update_edge_key(heap, edge);
        else if (edge->aux) {
          report_inline_failed_reason(edge);
          heap->delete_node((edge_heap_node_t *)edge->aux);
          edge->aux = NULL;
        }
      } else if (edge->aux)
        update_edge_key(heap, edge);
    }
}

/* Recompute HEAP nodes for each uninlined call in NODE
   If UPDATE_SINCE is non-NULL check if edges called within that function
   are inlinable (typically UPDATE_SINCE is the inline clone we introduced
   where all edges have new context).

   This is used when we know that edge badnesses are going only to increase
   (we introduced new call site) and thus all we need is to insert newly
   created edges into heap.  */

static void update_callee_keys(edge_heap_t *heap, struct cgraph_node *node,
                               struct cgraph_node *update_since,
                               bitmap updated_nodes) {
  struct cgraph_edge *e = node->callees;
  bool check_inlinability = update_since == node;

  if (!e) return;
  while (true)
    if (!e->inline_failed && e->callee->callees) {
      if (e->callee == update_since) check_inlinability = true;
      e = e->callee->callees;
    } else {
      enum availability avail;
      struct cgraph_node *callee;
      if (!check_inlinability) {
        if (e->aux &&
            !bitmap_bit_p(
                updated_nodes,
                e->callee->ultimate_alias_target(&avail, e->caller)->get_uid()))
          update_edge_key(heap, e);
      }
      /* We do not reset callee growth cache here.  Since we added a new call,
         growth should have just increased and consequently badness metric
             don't need updating.  */
      else if (e->inline_failed &&
               (callee = e->callee->ultimate_alias_target(&avail, e->caller)) &&
               avail >= AVAIL_AVAILABLE &&
               ipa_fn_summaries->get(callee) != NULL &&
               ipa_fn_summaries->get(callee)->inlinable &&
               !bitmap_bit_p(updated_nodes, callee->get_uid())) {
        if (can_inline_edge_p(e, false) &&
            want_inline_small_function_p(e, false) &&
            can_inline_edge_by_limits_p(e, false)) {
          gcc_checking_assert(check_inlinability ||
                              can_inline_edge_p(e, false));
          gcc_checking_assert(check_inlinability || e->aux);
          update_edge_key(heap, e);
        } else if (e->aux) {
          report_inline_failed_reason(e);
          heap->delete_node((edge_heap_node_t *)e->aux);
          e->aux = NULL;
        }
      }
      /* In case we redirected to unreachable node we only need to remove the
         fibheap entry.  */
      else if (e->aux) {
        heap->delete_node((edge_heap_node_t *)e->aux);
        e->aux = NULL;
      }
      if (e->next_callee)
        e = e->next_callee;
      else {
        do {
          if (e->caller == node) return;
          if (e->caller == update_since) check_inlinability = false;
          e = e->caller->callers;
        } while (!e->next_callee);
        e = e->next_callee;
      }
    }
}

/* Enqueue all recursive calls from NODE into priority queue depending on
   how likely we want to recursively inline the call.  */

static void lookup_recursive_calls(struct cgraph_node *node,
                                   struct cgraph_node *where,
                                   edge_heap_t *heap) {
  struct cgraph_edge *e;
  enum availability avail;

  for (e = where->callees; e; e = e->next_callee)
    if (e->callee == node ||
        (e->callee->ultimate_alias_target(&avail, e->caller) == node &&
         avail > AVAIL_INTERPOSABLE))
      heap->insert(-e->sreal_frequency(), e);
  for (e = where->callees; e; e = e->next_callee)
    if (!e->inline_failed) lookup_recursive_calls(node, e->callee, heap);
}

/* Decide on recursive inlining: in the case function has recursive calls,
   inline until body size reaches given argument.  If any new indirect edges
   are discovered in the process, add them to *NEW_EDGES, unless NEW_EDGES
   is NULL.  */

static bool recursive_inlining(struct cgraph_edge *edge,
                               vec<cgraph_edge *> *new_edges) {
  cgraph_node *to =
      (edge->caller->inlined_to ? edge->caller->inlined_to : edge->caller);
  int limit = opt_for_fn(to->decl, param_max_inline_insns_recursive_auto);
  edge_heap_t heap(sreal::min());
  struct cgraph_node *node;
  struct cgraph_edge *e;
  struct cgraph_node *master_clone = NULL, *next;
  int depth = 0;
  int n = 0;

  node = edge->caller;
  if (node->inlined_to) node = node->inlined_to;

  if (DECL_DECLARED_INLINE_P(node->decl))
    limit = opt_for_fn(to->decl, param_max_inline_insns_recursive);

  /* Make sure that function is small enough to be considered for inlining.  */
  if (estimate_size_after_inlining(node, edge) >= limit) return false;
  lookup_recursive_calls(node, node, &heap);
  if (heap.empty()) return false;

  if (dump_file)
    fprintf(dump_file, "  Performing recursive inlining on %s\n",
            node->dump_name());

  /* Do the inlining and update list of recursive call during process.  */
  while (!heap.empty()) {
    struct cgraph_edge *curr = heap.extract_min();
    struct cgraph_node *cnode, *dest = curr->callee;

    if (!can_inline_edge_p(curr, true) ||
        !can_inline_edge_by_limits_p(curr, true))
      continue;

    /* MASTER_CLONE is produced in the case we already started modified
   the function. Be sure to redirect edge to the original body before
   estimating growths otherwise we will be seeing growths after inlining
   the already modified body.  */
    if (master_clone) {
      curr->redirect_callee(master_clone);
      if (edge_growth_cache != NULL) edge_growth_cache->remove(curr);
    }

    if (estimate_size_after_inlining(node, curr) > limit) {
      curr->redirect_callee(dest);
      if (edge_growth_cache != NULL) edge_growth_cache->remove(curr);
      break;
    }

    depth = 1;
    for (cnode = curr->caller; cnode->inlined_to;
         cnode = cnode->callers->caller)
      if (node->decl == curr->callee->ultimate_alias_target()->decl) depth++;

    if (!want_inline_self_recursive_call_p(curr, node, false, depth)) {
      curr->redirect_callee(dest);
      if (edge_growth_cache != NULL) edge_growth_cache->remove(curr);
      continue;
    }

    if (dump_file) {
      fprintf(dump_file, "   Inlining call of depth %i", depth);
      if (node->count.nonzero_p() && curr->count.initialized_p()) {
        fprintf(
            dump_file, " called approx. %.2f times per call",
            (double)curr->count.to_gcov_type() / node->count.to_gcov_type());
      }
      fprintf(dump_file, "\n");
    }
    if (!master_clone) {
      /* We need original clone to copy around.  */
      master_clone = node->create_clone(node->decl, node->count, false, vNULL,
                                        true, NULL, NULL);
      for (e = master_clone->callees; e; e = e->next_callee)
        if (!e->inline_failed) clone_inlined_nodes(e, true, false, NULL);
      curr->redirect_callee(master_clone);
      if (edge_growth_cache != NULL) edge_growth_cache->remove(curr);
    }

    inline_call(curr, false, new_edges, &overall_size, true);
    reset_node_cache(node);
    lookup_recursive_calls(node, curr->callee, &heap);
    n++;
  }

  if (!heap.empty() && dump_file)
    fprintf(dump_file, "    Recursive inlining growth limit met.\n");

  if (!master_clone) return false;

  if (dump_enabled_p())
    dump_printf_loc(MSG_NOTE, edge->call_stmt,
                    "\n   Inlined %i times, "
                    "body grown from size %i to %i, time %f to %f\n",
                    n, ipa_size_summaries->get(master_clone)->size,
                    ipa_size_summaries->get(node)->size,
                    ipa_fn_summaries->get(master_clone)->time.to_double(),
                    ipa_fn_summaries->get(node)->time.to_double());

  /* Remove master clone we used for inlining.  We rely that clones inlined
     into master clone gets queued just before master clone so we don't
     need recursion.  */
  for (node = symtab->first_function(); node != master_clone; node = next) {
    next = symtab->next_function(node);
    if (node->inlined_to == master_clone) node->remove();
  }
  master_clone->remove();
  return true;
}

/* Given whole compilation unit estimate of INSNS, compute how large we can
   allow the unit to grow.  */

static int64_t compute_max_insns(cgraph_node *node, int insns) {
  int max_insns = insns;
  if (max_insns < opt_for_fn(node->decl, param_large_unit_insns))
    max_insns = opt_for_fn(node->decl, param_large_unit_insns);

  return ((int64_t)max_insns *
          (100 + opt_for_fn(node->decl, param_inline_unit_growth)) / 100);
}

/* Compute badness of all edges in NEW_EDGES and add them to the HEAP.  */

static void add_new_edges_to_heap(edge_heap_t *heap,
                                  vec<cgraph_edge *> &new_edges) {
  while (new_edges.length() > 0) {
    struct cgraph_edge *edge = new_edges.pop();

    gcc_assert(!edge->aux);
    gcc_assert(edge->callee);
    if (edge->inline_failed && can_inline_edge_p(edge, true) &&
        want_inline_small_function_p(edge, true) &&
        can_inline_edge_by_limits_p(edge, true))
      edge->aux = heap->insert(edge_badness(edge, false), edge);
  }
}

/* Remove EDGE from the fibheap.  */

static void heap_edge_removal_hook(struct cgraph_edge *e, void *data) {
  if (e->aux) {
    ((edge_heap_t *)data)->delete_node((edge_heap_node_t *)e->aux);
    e->aux = NULL;
  }
}

/* Return true if speculation of edge E seems useful.
   If ANTICIPATE_INLINING is true, be conservative and hope that E
   may get inlined.  */

bool speculation_useful_p(struct cgraph_edge *e, bool anticipate_inlining) {
  /* If we have already decided to inline the edge, it seems useful.  */
  if (!e->inline_failed) return true;

  enum availability avail;
  struct cgraph_node *target =
      e->callee->ultimate_alias_target(&avail, e->caller);

  gcc_assert(e->speculative && !e->indirect_unknown_callee);

  if (!e->maybe_hot_p()) return false;

  /* See if IP optimizations found something potentially useful about the
     function.  For now we look only for CONST/PURE flags.  Almost everything
     else we propagate is useless.  */
  if (avail >= AVAIL_AVAILABLE) {
    int ecf_flags = flags_from_decl_or_type(target->decl);
    if (ecf_flags & ECF_CONST) {
      if (!(e->speculative_call_indirect_edge()->indirect_info->ecf_flags &
            ECF_CONST))
        return true;
    } else if (ecf_flags & ECF_PURE) {
      if (!(e->speculative_call_indirect_edge()->indirect_info->ecf_flags &
            ECF_PURE))
        return true;
    }
  }
  /* If we did not managed to inline the function nor redirect
     to an ipa-cp clone (that are seen by having local flag set),
     it is probably pointless to inline it unless hardware is missing
     indirect call predictor.  */
  if (!anticipate_inlining && !target->local) return false;
  /* For overwritable targets there is not much to do.  */
  if (!can_inline_edge_p(e, false) ||
      !can_inline_edge_by_limits_p(e, false, true))
    return false;
  /* OK, speculation seems interesting.  */
  return true;
}

/* We know that EDGE is not going to be inlined.
   See if we can remove speculation.  */

static void resolve_noninline_speculation(edge_heap_t *edge_heap,
                                          struct cgraph_edge *edge) {
  if (edge->speculative && !speculation_useful_p(edge, false)) {
    struct cgraph_node *node = edge->caller;
    struct cgraph_node *where = node->inlined_to ? node->inlined_to : node;
    auto_bitmap updated_nodes;

    if (edge->count.ipa().initialized_p()) spec_rem += edge->count.ipa();
    cgraph_edge::resolve_speculation(edge);
    reset_edge_caches(where);
    ipa_update_overall_fn_summary(where);
    update_caller_keys(edge_heap, where, updated_nodes, NULL);
    update_callee_keys(edge_heap, where, NULL, updated_nodes);
  }
}

/* Return true if NODE should be accounted for overall size estimate.
   Skip all nodes optimized for size so we can measure the growth of hot
   part of program no matter of the padding.  */

bool inline_account_function_p(struct cgraph_node *node) {
  return (!DECL_EXTERNAL(node->decl) &&
          !opt_for_fn(node->decl, optimize_size) &&
          node->frequency != NODE_FREQUENCY_UNLIKELY_EXECUTED);
}

/* Count number of callers of NODE and store it into DATA (that
   points to int.  Worker for cgraph_for_node_and_aliases.  */

static bool sum_callers(struct cgraph_node *node, void *data) {
  struct cgraph_edge *e;
  int *num_calls = (int *)data;

  for (e = node->callers; e; e = e->next_caller) (*num_calls)++;
  return false;
}

/* We only propagate across edges with non-interposable callee.  */

inline bool ignore_edge_p(struct cgraph_edge *e) {
  enum availability avail;
  e->callee->function_or_virtual_thunk_symbol(&avail, e->caller);
  return (avail <= AVAIL_INTERPOSABLE);
}

/* We use greedy algorithm for inlining of small functions:
   All inline candidates are put into prioritized heap ordered in
   increasing badness.

   The inlining of small functions is bounded by unit growth parameters.  */

static void inline_small_functions(void) {
  struct cgraph_node *node;
  struct cgraph_edge *edge;
  edge_heap_t edge_heap(sreal::min());
  auto_bitmap updated_nodes;
  int min_size;
  auto_vec<cgraph_edge *> new_indirect_edges;
  int initial_size = 0;
  struct cgraph_node **order = XCNEWVEC(cgraph_node *, symtab->cgraph_count);
  struct cgraph_edge_hook_list *edge_removal_hook_holder;
  new_indirect_edges.create(8);

  edge_removal_hook_holder =
      symtab->add_edge_removal_hook(&heap_edge_removal_hook, &edge_heap);

  /* Compute overall unit size and other global parameters used by badness
     metrics.  */
  free_engine();
  max_count = profile_count::uninitialized();
  ipa_reduced_postorder(order, true, ignore_edge_p);
  free(order);

  FOR_EACH_DEFINED_FUNCTION(node)
  if (!node->inlined_to) {
    if (!node->alias && node->analyzed &&
        (node->has_gimple_body_p() || node->thunk) &&
        opt_for_fn(node->decl, optimize)) {
      class ipa_fn_summary *info = ipa_fn_summaries->get(node);
      struct ipa_dfs_info *dfs = (struct ipa_dfs_info *)node->aux;

      /* Do not account external functions, they will be optimized out
         if not inlined.  Also only count the non-cold portion of program.  */
      if (inline_account_function_p(node))
        initial_size += ipa_size_summaries->get(node)->size;
      info->growth = estimate_growth(node);

      int num_calls = 0;
      node->call_for_symbol_and_aliases(sum_callers, &num_calls, true);
      if (num_calls == 1) info->single_caller = true;
      if (dfs && dfs->next_cycle) {
        struct cgraph_node *n2;
        int id = dfs->scc_no + 1;
        for (n2 = node; n2; n2 = ((struct ipa_dfs_info *)n2->aux)->next_cycle)
          if (opt_for_fn(n2->decl, optimize)) {
            ipa_fn_summary *info2 =
                ipa_fn_summaries->get(n2->inlined_to ? n2->inlined_to : n2);
            if (info2->scc_no) break;
            info2->scc_no = id;
          }
      }
    }

    for (edge = node->callers; edge; edge = edge->next_caller)
      max_count = max_count.max(edge->count.ipa());
  }
  ipa_free_postorder_info();
  initialize_growth_caches();

  if (dump_file)
    fprintf(
        dump_file,
        "\nDeciding on inlining of small functions.  Starting with size %i.\n",
        initial_size);

  overall_size = initial_size;
  min_size = overall_size;

  /* Populate the heap with all edges we might inline.  */

  std::vector<double> filter_features;

  FOR_EACH_DEFINED_FUNCTION(node) {
    bool update = false;
    struct cgraph_edge *next = NULL;
    bool has_speculative = false;

    if (!opt_for_fn(node->decl, optimize)
        /* With -Og we do not want to perform IPA inlining of small
           functions since there are no scalar cleanups after it
           that would realize the anticipated win.  All abstraction
           is removed during early inlining.  */
        || opt_for_fn(node->decl, optimize_debug))
      continue;

    if (dump_file)
      fprintf(dump_file, "Enqueueing calls in %s.\n", node->dump_name());

    for (edge = node->callees; edge; edge = edge->next_callee) {
      if (edge_ignore_limits_map.find(edge) == edge_ignore_limits_map.end()) {
        edge_ignore_limits_map[edge] = false;
      }
      /* AI4Compiler Inference Process */
      /* Invoke the plug-in framework to perform model inference and modify
       * heuristic rules based on the prediction result.  */
      get_features(node, edge, filter_features);
      initialize(g_inline_model_path);
      add_int64_input(simultaneous_prefetches, 1);
      add_int64_input(l1_cache_size, 1);
      add_int64_input(l1_cache_line_size, 1);
      add_int64_input(l2_cache_size, 1);
      add_int64_input(prefetch_latency, 1);
      add_int64_input(ipa_prefetch_distance_factor, 1);
      add_string_input(var_str, 1);
      add_double_input(filter_features.data(), filter_features.size());

      int err = inference();
      if (err == 0) {
        int64_t *result = get_int64_output(0);
        if (result && result[0] == 1) {
          edge_ignore_limits_map[edge] = true;
        }
      }

      if (edge->inline_failed && !edge->aux && can_inline_edge_p(edge, true) &&
          want_inline_small_function_p(edge, true) &&
          can_inline_edge_by_limits_p(edge, true) && edge->inline_failed) {
        gcc_assert(!edge->aux);
        update_edge_key(&edge_heap, edge);
      }
      if (edge->speculative) has_speculative = true;
    }
    if (has_speculative)
      for (edge = node->callees; edge; edge = next) {
        next = edge->next_callee;
        if (edge->speculative &&
            !speculation_useful_p(edge, edge->aux != NULL)) {
          cgraph_edge::resolve_speculation(edge);
          update = true;
        }
      }
    if (update) {
      struct cgraph_node *where = node->inlined_to ? node->inlined_to : node;
      ipa_update_overall_fn_summary(where);
      reset_edge_caches(where);
      update_caller_keys(&edge_heap, where, updated_nodes, NULL);
      update_callee_keys(&edge_heap, where, NULL, updated_nodes);
      bitmap_clear(updated_nodes);
    }
  }

  gcc_assert(in_lto_p || !(max_count > 0) ||
             (profile_info && flag_branch_probabilities));

  std::vector<double> heap_features;

  while (!edge_heap.empty()) {
    int old_size = overall_size;
    struct cgraph_node *where, *callee;
    sreal badness = edge_heap.min_key();
    sreal current_badness;
    int growth;

    edge = edge_heap.extract_min();
    gcc_assert(edge->aux);
    edge->aux = NULL;
    if (!edge->inline_failed || !edge->callee->analyzed) continue;

    if (edge_ignore_limits_map.find(edge) == edge_ignore_limits_map.end()) {
      /* AI4Compiler Inference Process */
      /* Invoke the plug-in framework to perform model inference and modify
       * heuristic rules based on the prediction result.  */
      get_features(edge->caller, edge, heap_features);
      initialize(g_inline_model_path);
      add_int64_input(simultaneous_prefetches, 1);
      add_int64_input(l1_cache_size, 1);
      add_int64_input(l1_cache_line_size, 1);
      add_int64_input(l2_cache_size, 1);
      add_int64_input(prefetch_latency, 1);
      add_int64_input(ipa_prefetch_distance_factor, 1);
      add_string_input(var_str, 1);
      add_double_input(heap_features.data(), heap_features.size());

      int err = inference();
      if (err == 0) {
        int64_t *result = get_int64_output(0);

        if (result && result[0] == 1) {
          edge_ignore_limits_map[edge] = true;
        }
      }
    }

    /* Be sure that caches are maintained consistent.
   This check is affected by scaling roundoff errors when compiling for
   IPA this we skip it in that case.  */
    if (flag_checking && !edge->callee->count.ipa_p() &&
        (!max_count.initialized_p() || !max_count.nonzero_p())) {
      sreal cached_badness = edge_badness(edge, false);

      int old_size_est = estimate_edge_size(edge);
      sreal old_time_est = estimate_edge_time(edge);
      int old_hints_est = estimate_edge_hints(edge);

      if (edge_growth_cache != NULL) edge_growth_cache->remove(edge);
      reset_node_cache(edge->caller->inlined_to ? edge->caller->inlined_to
                                                : edge->caller);
      gcc_assert(old_size_est == estimate_edge_size(edge));
      gcc_assert(old_time_est == estimate_edge_time(edge));
      /* FIXME:

         gcc_assert (old_hints_est == estimate_edge_hints (edge));

         fails with profile feedback because some hints depends on
         maybe_hot_edge_p predicate and because callee gets inlined to other
         calls, the edge may become cold.
         This ought to be fixed by computing relative probabilities
         for given invocation but that will be better done once whole
         code is converted to sreals.  Disable for now and revert to "wrong"
         value so enable/disable checking paths agree.  */
      edge_growth_cache->get(edge)->hints = old_hints_est + 1;

      /* When updating the edge costs, we only decrease badness in the keys.
         Increases of badness are handled lazily; when we see key with out
         of date value on it, we re-insert it now.  */
      current_badness = edge_badness(edge, false);
      gcc_assert(cached_badness == current_badness);
      gcc_assert(current_badness >= badness);
    } else
      current_badness = edge_badness(edge, false);
    if (current_badness != badness) {
      if (edge_heap.min() && current_badness > edge_heap.min_key()) {
        edge->aux = edge_heap.insert(current_badness, edge);
        continue;
      } else
        badness = current_badness;
    }

    if (!can_inline_edge_p(edge, true) ||
        !can_inline_edge_by_limits_p(edge, true)) {
      resolve_noninline_speculation(&edge_heap, edge);
      continue;
    }

    callee = edge->callee->ultimate_alias_target();
    growth = estimate_edge_growth(edge);
    if (dump_file) {
      fprintf(dump_file, "\nConsidering %s with %i size\n", callee->dump_name(),
              ipa_size_summaries->get(callee)->size);
      fprintf(
          dump_file,
          " to be inlined into %s in %s:%i\n"
          " Estimated badness is %f, frequency %.2f.\n",
          edge->caller->dump_name(),
          edge->call_stmt &&
                  (LOCATION_LOCUS(gimple_location(
                       (const gimple *)edge->call_stmt)) > BUILTINS_LOCATION)
              ? gimple_filename((const gimple *)edge->call_stmt)
              : "unknown",
          edge->call_stmt ? gimple_lineno((const gimple *)edge->call_stmt) : -1,
          badness.to_double(), edge->sreal_frequency().to_double());
      if (edge->count.ipa().initialized_p()) {
        fprintf(dump_file, " Called ");
        edge->count.ipa().dump(dump_file);
        fprintf(dump_file, " times\n");
      }
      if (dump_flags & TDF_DETAILS) edge_badness(edge, true);
    }

    where = edge->caller;

    if (overall_size + growth > compute_max_insns(where, min_size) &&
        (edge_ignore_limits_map.find(edge) == edge_ignore_limits_map.end() ||
         !edge_ignore_limits_map[edge]) &&
        !DECL_DISREGARD_INLINE_LIMITS(callee->decl)) {
      edge->inline_failed = CIF_INLINE_UNIT_GROWTH_LIMIT;
      report_inline_failed_reason(edge);
      resolve_noninline_speculation(&edge_heap, edge);
      continue;
    }

    if (!want_inline_small_function_p(edge, true)) {
      resolve_noninline_speculation(&edge_heap, edge);
      continue;
    }

    profile_count old_count = callee->count;

    /* Heuristics for inlining small functions work poorly for
   recursive calls where we do effects similar to loop unrolling.
   When inlining such edge seems profitable, leave decision on
   specific inliner.  */
    if (edge->recursive_p()) {
      if (where->inlined_to) where = where->inlined_to;
      if (!recursive_inlining(
              edge, opt_for_fn(edge->caller->decl, flag_indirect_inlining)
                        ? &new_indirect_edges
                        : NULL)) {
        edge->inline_failed = CIF_RECURSIVE_INLINING;
        resolve_noninline_speculation(&edge_heap, edge);
        continue;
      }
      reset_edge_caches(where);
      /* Recursive inliner inlines all recursive calls of the function
         at once. Consequently we need to update all callee keys.  */
      if (opt_for_fn(edge->caller->decl, flag_indirect_inlining))
        add_new_edges_to_heap(&edge_heap, new_indirect_edges);
      update_callee_keys(&edge_heap, where, where, updated_nodes);
      bitmap_clear(updated_nodes);
    } else {
      struct cgraph_node *outer_node = NULL;
      int depth = 0;

      /* Consider the case where self recursive function A is inlined
         into B.  This is desired optimization in some cases, since it
         leads to effect similar of loop peeling and we might completely
         optimize out the recursive call.  However we must be extra
         selective.  */

      where = edge->caller;
      while (where->inlined_to) {
        if (where->decl == callee->decl) outer_node = where, depth++;
        where = where->callers->caller;
      }
      if (outer_node &&
          !want_inline_self_recursive_call_p(edge, outer_node, true, depth)) {
        edge->inline_failed = (DECL_DISREGARD_INLINE_LIMITS(edge->callee->decl)
                                   ? CIF_RECURSIVE_INLINING
                                   : CIF_UNSPECIFIED);
        resolve_noninline_speculation(&edge_heap, edge);
        continue;
      } else if (depth && dump_file)
        fprintf(dump_file, " Peeling recursion with depth %i\n", depth);

      gcc_checking_assert(!callee->inlined_to);

      int old_size = ipa_size_summaries->get(where)->size;
      sreal old_time = ipa_fn_summaries->get(where)->time;

      inline_call(edge, true, &new_indirect_edges, &overall_size, true);
      reset_edge_caches(edge->callee);
      add_new_edges_to_heap(&edge_heap, new_indirect_edges);

      /* If caller's size and time increased we do not need to update
         all edges because badness is not going to decrease.  */
      if (old_size <= ipa_size_summaries->get(where)->size &&
          old_time <= ipa_fn_summaries->get(where)->time
          /* Wrapper penalty may be non-monotonous in this respect.
             Fortunately it only affects small functions.  */
          && !wrapper_heuristics_may_apply(where, old_size))
        update_callee_keys(&edge_heap, edge->callee, edge->callee,
                           updated_nodes);
      else
        update_callee_keys(&edge_heap, where, edge->callee, updated_nodes);
    }
    where = edge->caller;
    if (where->inlined_to) where = where->inlined_to;

    /* Our profitability metric can depend on local properties
   such as number of inlinable calls and size of the function body.
   After inlining these properties might change for the function we
   inlined into (since it's body size changed) and for the functions
   called by function we inlined (since number of it inlinable callers
   might change).  */
    update_caller_keys(&edge_heap, where, updated_nodes, NULL);
    /* Offline copy count has possibly changed, recompute if profile is
   available.  */
    struct cgraph_node *n =
        cgraph_node::get(edge->callee->decl)->ultimate_alias_target();
    if (n != edge->callee && n->analyzed && !(n->count == old_count) &&
        n->count.ipa_p())
      update_callee_keys(&edge_heap, n, NULL, updated_nodes);
    bitmap_clear(updated_nodes);

    if (dump_enabled_p()) {
      ipa_fn_summary *s = ipa_fn_summaries->get(where);

      /* dump_printf can't handle %+i.  */
      char buf_net_change[100];
      snprintf(buf_net_change, sizeof buf_net_change, "%+i",
               overall_size - old_size);

      dump_printf_loc(MSG_OPTIMIZED_LOCATIONS, edge->call_stmt,
                      " Inlined %C into %C which now has time %f and "
                      "size %i, net change of %s%s.\n",
                      edge->callee, edge->caller, s->time.to_double(),
                      ipa_size_summaries->get(edge->caller)->size,
                      buf_net_change,
                      cross_module_call_p(edge) ? " (cross module)" : "");
    }
    if (min_size > overall_size) {
      min_size = overall_size;

      if (dump_file)
        fprintf(dump_file, "New minimal size reached: %i\n", min_size);
    }
  }

  free_growth_caches();

  free_engine();

  if (dump_enabled_p())
    dump_printf(MSG_NOTE,
                "Unit growth for small function inlining: %i->%i (%i%%)\n",
                initial_size, overall_size,
                initial_size ? overall_size * 100 / (initial_size)-100 : 0);
  symtab->remove_edge_removal_hook(edge_removal_hook_holder);
}

/* Flatten NODE.  Performed both during early inlining and
   at IPA inlining time.  */

static void flatten_function(struct cgraph_node *node, bool early,
                             bool update) {
  struct cgraph_edge *e;

  /* We shouldn't be called recursively when we are being processed.  */
  gcc_assert(node->aux == NULL);

  node->aux = (void *)node;

  for (e = node->callees; e; e = e->next_callee) {
    struct cgraph_node *orig_callee;
    struct cgraph_node *callee = e->callee->ultimate_alias_target();

    /* We've hit cycle?  It is time to give up.  */
    if (callee->aux) {
      if (dump_enabled_p())
        dump_printf_loc(MSG_MISSED_OPTIMIZATION, e->call_stmt,
                        "Not inlining %C into %C to avoid cycle.\n", callee,
                        e->caller);
      if (cgraph_inline_failed_type(e->inline_failed) != CIF_FINAL_ERROR)
        e->inline_failed = CIF_RECURSIVE_INLINING;
      continue;
    }

    /* When the edge is already inlined, we just need to recurse into
   it in order to fully flatten the leaves.  */
    if (!e->inline_failed) {
      flatten_function(callee, early, false);
      continue;
    }

    /* Flatten attribute needs to be processed during late inlining. For
   extra code quality we however do flattening during early optimization,
   too.  */
    if (!early ? !can_inline_edge_p(e, true) &&
                     !can_inline_edge_by_limits_p(e, true)
               : !can_early_inline_edge_p(e))
      continue;

    if (e->recursive_p()) {
      if (dump_enabled_p())
        dump_printf_loc(MSG_MISSED_OPTIMIZATION, e->call_stmt,
                        "Not inlining: recursive call.\n");
      continue;
    }

    if (gimple_in_ssa_p(DECL_STRUCT_FUNCTION(node->decl)) !=
        gimple_in_ssa_p(DECL_STRUCT_FUNCTION(callee->decl))) {
      if (dump_enabled_p())
        dump_printf_loc(MSG_MISSED_OPTIMIZATION, e->call_stmt,
                        "Not inlining: SSA form does not match.\n");
      continue;
    }

    /* Inline the edge and flatten the inline clone.  Avoid
       recursing through the original node if the node was cloned.  */
    if (dump_enabled_p())
      dump_printf_loc(MSG_OPTIMIZED_LOCATIONS, e->call_stmt,
                      " Inlining %C into %C.\n", callee, e->caller);
    orig_callee = callee;
    inline_call(e, true, NULL, NULL, false);
    if (e->callee != orig_callee) orig_callee->aux = (void *)node;
    flatten_function(e->callee, early, false);
    if (e->callee != orig_callee) orig_callee->aux = NULL;
  }

  node->aux = NULL;
  cgraph_node *where = node->inlined_to ? node->inlined_to : node;
  if (update && opt_for_fn(where->decl, optimize))
    ipa_update_overall_fn_summary(where);
}

/* Inline NODE to all callers.  Worker for cgraph_for_node_and_aliases.
   DATA points to number of calls originally found so we avoid infinite
   recursion.  */

static bool inline_to_all_callers_1(struct cgraph_node *node, void *data,
                                    hash_set<cgraph_node *> *callers) {
  int *num_calls = (int *)data;
  bool callee_removed = false;

  while (node->callers && !node->inlined_to) {
    struct cgraph_node *caller = node->callers->caller;

    if (!can_inline_edge_p(node->callers, true) ||
        !can_inline_edge_by_limits_p(node->callers, true) ||
        node->callers->recursive_p()) {
      if (dump_file) fprintf(dump_file, "Uninlinable call found; giving up.\n");
      *num_calls = 0;
      return false;
    }

    if (dump_file) {
      cgraph_node *ultimate = node->ultimate_alias_target();
      fprintf(dump_file, "\nInlining %s size %i.\n", ultimate->dump_name(),
              ipa_size_summaries->get(ultimate)->size);
      fprintf(dump_file, " Called once from %s %i insns.\n",
              node->callers->caller->dump_name(),
              ipa_size_summaries->get(node->callers->caller)->size);
    }

    /* Remember which callers we inlined to, delaying updating the
   overall summary.  */
    callers->add(node->callers->caller);
    inline_call(node->callers, true, NULL, NULL, false, &callee_removed);
    if (dump_file)
      fprintf(dump_file, " Inlined into %s which now has %i size\n",
              caller->dump_name(), ipa_size_summaries->get(caller)->size);
    if (!(*num_calls)--) {
      if (dump_file) fprintf(dump_file, "New calls found; giving up.\n");
      return callee_removed;
    }
    if (callee_removed) return true;
  }
  return false;
}

/* Wrapper around inline_to_all_callers_1 doing delayed overall summary
   update.  */

static bool inline_to_all_callers(struct cgraph_node *node, void *data) {
  hash_set<cgraph_node *> callers;
  bool res = inline_to_all_callers_1(node, data, &callers);
  /* Perform the delayed update of the overall summary of all callers
     processed.  This avoids quadratic behavior in the cases where
     we have a lot of calls to the same function.  */
  for (hash_set<cgraph_node *>::iterator i = callers.begin();
       i != callers.end(); ++i)
    ipa_update_overall_fn_summary((*i)->inlined_to ? (*i)->inlined_to : *i);
  return res;
}

/* Output overall time estimate.  */
static void dump_overall_stats(void) {
  sreal sum_weighted = 0, sum = 0;
  struct cgraph_node *node;

  FOR_EACH_DEFINED_FUNCTION(node)
  if (!node->inlined_to && !node->alias) {
    ipa_fn_summary *s = ipa_fn_summaries->get(node);
    if (s != NULL) {
      sum += s->time;
      if (node->count.ipa().initialized_p())
        sum_weighted += s->time * node->count.ipa().to_gcov_type();
    }
  }
  fprintf(dump_file,
          "Overall time estimate: "
          "%f weighted by profile: "
          "%f\n",
          sum.to_double(), sum_weighted.to_double());
}

/* Output some useful stats about inlining.  */

static void dump_inline_stats(void) {
  int64_t inlined_cnt = 0, inlined_indir_cnt = 0;
  int64_t inlined_virt_cnt = 0, inlined_virt_indir_cnt = 0;
  int64_t noninlined_cnt = 0, noninlined_indir_cnt = 0;
  int64_t noninlined_virt_cnt = 0, noninlined_virt_indir_cnt = 0;
  int64_t inlined_speculative = 0, inlined_speculative_ply = 0;
  int64_t indirect_poly_cnt = 0, indirect_cnt = 0;
  int64_t reason[CIF_N_REASONS][2];
  sreal reason_freq[CIF_N_REASONS];
  int i;
  struct cgraph_node *node;

  memset(reason, 0, sizeof(reason));
  for (i = 0; i < CIF_N_REASONS; i++) reason_freq[i] = 0;
  FOR_EACH_DEFINED_FUNCTION(node) {
    struct cgraph_edge *e;
    for (e = node->callees; e; e = e->next_callee) {
      if (e->inline_failed) {
        if (e->count.ipa().initialized_p())
          reason[(int)e->inline_failed][0] += e->count.ipa().to_gcov_type();
        reason_freq[(int)e->inline_failed] += e->sreal_frequency();
        reason[(int)e->inline_failed][1]++;
        if (DECL_VIRTUAL_P(e->callee->decl) && e->count.ipa().initialized_p()) {
          if (e->indirect_inlining_edge)
            noninlined_virt_indir_cnt += e->count.ipa().to_gcov_type();
          else
            noninlined_virt_cnt += e->count.ipa().to_gcov_type();
        } else if (e->count.ipa().initialized_p()) {
          if (e->indirect_inlining_edge)
            noninlined_indir_cnt += e->count.ipa().to_gcov_type();
          else
            noninlined_cnt += e->count.ipa().to_gcov_type();
        }
      } else if (e->count.ipa().initialized_p()) {
        if (e->speculative) {
          if (DECL_VIRTUAL_P(e->callee->decl))
            inlined_speculative_ply += e->count.ipa().to_gcov_type();
          else
            inlined_speculative += e->count.ipa().to_gcov_type();
        } else if (DECL_VIRTUAL_P(e->callee->decl)) {
          if (e->indirect_inlining_edge)
            inlined_virt_indir_cnt += e->count.ipa().to_gcov_type();
          else
            inlined_virt_cnt += e->count.ipa().to_gcov_type();
        } else {
          if (e->indirect_inlining_edge)
            inlined_indir_cnt += e->count.ipa().to_gcov_type();
          else
            inlined_cnt += e->count.ipa().to_gcov_type();
        }
      }
    }
    for (e = node->indirect_calls; e; e = e->next_callee)
      if (e->indirect_info->polymorphic & e->count.ipa().initialized_p())
        indirect_poly_cnt += e->count.ipa().to_gcov_type();
      else if (e->count.ipa().initialized_p())
        indirect_cnt += e->count.ipa().to_gcov_type();
  }
  if (max_count.initialized_p()) {
    fprintf(dump_file,
            "Inlined %" PRId64
            " + speculative "
            "%" PRId64
            " + speculative polymorphic "
            "%" PRId64
            " + previously indirect "
            "%" PRId64
            " + virtual "
            "%" PRId64
            " + virtual and previously indirect "
            "%" PRId64
            "\n"
            "Not inlined "
            "%" PRId64
            " + previously indirect "
            "%" PRId64
            " + virtual "
            "%" PRId64
            " + virtual and previously indirect "
            "%" PRId64
            " + still indirect "
            "%" PRId64
            " + still indirect polymorphic "
            "%" PRId64 "\n",
            inlined_cnt, inlined_speculative, inlined_speculative_ply,
            inlined_indir_cnt, inlined_virt_cnt, inlined_virt_indir_cnt,
            noninlined_cnt, noninlined_indir_cnt, noninlined_virt_cnt,
            noninlined_virt_indir_cnt, indirect_cnt, indirect_poly_cnt);
    fprintf(dump_file, "Removed speculations ");
    spec_rem.dump(dump_file);
    fprintf(dump_file, "\n");
  }
  dump_overall_stats();
  fprintf(dump_file, "\nWhy inlining failed?\n");
  for (i = 0; i < CIF_N_REASONS; i++)
    if (reason[i][1])
      fprintf(dump_file, "%-50s: %8i calls, %8f freq, %" PRId64 " count\n",
              cgraph_inline_failed_string((cgraph_inline_failed_t)i),
              (int)reason[i][1], reason_freq[i].to_double(), reason[i][0]);
}

/* Called when node is removed.  */

static void flatten_remove_node_hook(struct cgraph_node *node, void *data) {
  if (lookup_attribute("flatten", DECL_ATTRIBUTES(node->decl)) == NULL) return;

  hash_set<struct cgraph_node *> *removed =
      (hash_set<struct cgraph_node *> *)data;
  removed->add(node);
}

/* ===---- AI4Compiler IPA-Inline Feature Extraction Functions ----=== */
/* Obtains the caller and callee information of the current node.  */

void get_caller_callee_info(struct cgraph_node *node, int &caller_count,
                            int &callee_count) {
  caller_count = 0;
  callee_count = 0;
  if (node_caller_callee_count.find(node) == node_caller_callee_count.end()) {
    if (node->callers) {
      for (cgraph_edge *e = node->callers; e; e = e->next_caller) {
        caller_count++;
      }
    }
    if (node->callees) {
      for (cgraph_edge *e = node->callees; e; e = e->next_callee) {
        callee_count++;
      }
    }
    node_caller_callee_count[node] = {caller_count, callee_count};
  } else {
    auto it = node_caller_callee_count[node];
    caller_count = it.first;
    callee_count = it.second;
  }
  return;
}

/* Obtains the basic block information of the current node.  */

void get_basic_block_info(struct cgraph_node *node, int &bb_count,
                          int &cond_bb_count) {
  tree function_decl = node->decl;
  struct function *my_function = DECL_STRUCT_FUNCTION(function_decl);
  gimple_stmt_iterator gsi;
  basic_block bb;
  bb_count = 0;
  cond_bb_count = 0;

  if (!gimple_has_body_p(node->decl)) {
    return;
  }

  FOR_EACH_BB_FN(bb, my_function) {
    bb_count++;
    bool is_conditionally_executed = false;
    for (gsi = gsi_start_bb(bb); !gsi_end_p(gsi); gsi_next(&gsi)) {
      enum gimple_code code = gimple_code(gsi_stmt(gsi));
      if (code == GIMPLE_COND) {
        is_conditionally_executed = true;
        break;
      }
    }

    if (is_conditionally_executed) {
      cond_bb_count++;
    }
  }
  return;
}

/* Obtains the number of input parameters of the current node.  */

int get_function_parameters_info(struct cgraph_node *node) {
  tree function_decl = node->decl;
  tree args = DECL_ARGUMENTS(function_decl);

  int num_params = 0;

  for (tree param = args; param; param = TREE_CHAIN(param)) {
    num_params++;
  }

  return num_params;
}

/* Obtains the number of gimple instructions on the current node.  */

static StatementInfo count_stmt_types_fn(struct cgraph_node *node) {
  tree function_decl = node->decl;
  struct function *my_function = DECL_STRUCT_FUNCTION(function_decl);
  gimple_stmt_iterator gsi;
  basic_block bb;

  StatementInfo info;
  info.total_stmt_count = 0;

  for (int i = 0; i < LAST_AND_UNUSED_GIMPLE_CODE; ++i) {
    info.stmt_counts[static_cast<enum gimple_code>(i)] = 0;
  }

  if (!gimple_has_body_p(node->decl)) {
    return info;
  }

  // gcc_assert(my_function && my_function->cfg);
  FOR_EACH_BB_FN(bb, my_function) {
    for (gsi = gsi_start_bb(bb); !gsi_end_p(gsi); gsi_next(&gsi)) {
      enum gimple_code code = gimple_code(gsi_stmt(gsi));
      info.stmt_counts[code]++;
      info.total_stmt_count++;
    }
  }

  return info;
}

/* Obtains the invoking times and invoking frequency of the current edge.  */

void get_edge_info(struct cgraph_edge *edge, int &edge_count,
                   double &edge_freq) {
  if (edge->count.initialized_p()) {
    edge_count = edge->count.ipa().to_gcov_type();
  } else {
    edge_count = 0;
  }

  edge_freq = edge->sreal_frequency().to_double();

  return;
}

/* Obtains the current node information, which is used to extract the caller and
 * callee features.  */

void extract_node_features(struct cgraph_node *node,
                           std::vector<double> &features) {
  int Caller_Count;
  int Callee_Count;
  get_caller_callee_info(node, Caller_Count, Callee_Count);
  features.push_back(static_cast<double>(Caller_Count));
  features.push_back(static_cast<double>(Callee_Count));

  int Basic_Block_Count;
  int Conditionally_Executed_Blocks;
  get_basic_block_info(node, Basic_Block_Count, Conditionally_Executed_Blocks);
  features.push_back(static_cast<double>(Basic_Block_Count));
  features.push_back(static_cast<double>(Conditionally_Executed_Blocks));

  int Total_Parameters_Count = get_function_parameters_info(node);
  features.push_back(static_cast<double>(Total_Parameters_Count));

  StatementInfo Stmy_Info = count_stmt_types_fn(node);
  features.push_back(static_cast<double>(Stmy_Info.total_stmt_count));

  const enum gimple_code gimple_codes[] = {
      GIMPLE_COND,   GIMPLE_DEBUG,       GIMPLE_LABEL,  GIMPLE_SWITCH,
      GIMPLE_ASSIGN, GIMPLE_ASM,         GIMPLE_CALL,   GIMPLE_RETURN,
      GIMPLE_RESX,   GIMPLE_EH_DISPATCH, GIMPLE_PREDICT};

  for (const auto &code : gimple_codes) {
    features.push_back(static_cast<double>(Stmy_Info.stmt_counts[code]));
  }
}

/* Obtains the features of nodes and edge for onnx model input.  */

void get_features(struct cgraph_node *node, struct cgraph_edge *edge,
                  std::vector<double> &features) {
  features.clear();

  extract_node_features(node, features);
  extract_node_features(edge->callee, features);

  int Edge_Count;
  double Edge_Freq;
  get_edge_info(edge, Edge_Count, Edge_Freq);
  features.push_back(static_cast<double>(Edge_Count));
  features.push_back(Edge_Freq);
}
/* ===---- AI4Compiler IPA-Inline Feature Extraction Functions ----=== */

/* Decide on the inlining.  We do so in the topological order to avoid
   expenses on updating data structures.  */

static unsigned int ipa_inline(void) {
  struct cgraph_node *node;
  int nnodes;
  struct cgraph_node **order;
  int i, j;
  int cold;
  bool remove_functions = false;

  order = XCNEWVEC(struct cgraph_node *, symtab->cgraph_count);

  if (dump_file) ipa_dump_fn_summaries(dump_file);

  nnodes = ipa_reverse_postorder(order);
  spec_rem = profile_count::zero();

  FOR_EACH_FUNCTION(node) {
    node->aux = 0;

    /* Recompute the default reasons for inlining because they may have
   changed during merging.  */
    if (in_lto_p) {
      for (cgraph_edge *e = node->callees; e; e = e->next_callee) {
        gcc_assert(e->inline_failed);
        initialize_inline_failed(e);
      }
      for (cgraph_edge *e = node->indirect_calls; e; e = e->next_callee)
        initialize_inline_failed(e);
    }
  }

  if (dump_file) fprintf(dump_file, "\nFlattening functions:\n");

  /* First shrink order array, so that it only contains nodes with
     flatten attribute.  */
  for (i = nnodes - 1, j = i; i >= 0; i--) {
    node = order[i];
    if (node->definition
        /* Do not try to flatten aliases.  These may happen for example when
           creating local aliases.  */
        && !node->alias &&
        lookup_attribute("flatten", DECL_ATTRIBUTES(node->decl)) != NULL)
      order[j--] = order[i];
  }

  /* After the above loop, order[j + 1] ... order[nnodes - 1] contain
     nodes with flatten attribute.  If there is more than one such
     node, we need to register a node removal hook, as flatten_function
     could remove other nodes with flatten attribute.  See PR82801.  */
  struct cgraph_node_hook_list *node_removal_hook_holder = NULL;
  hash_set<struct cgraph_node *> *flatten_removed_nodes = NULL;
  if (j < nnodes - 2) {
    flatten_removed_nodes = new hash_set<struct cgraph_node *>;
    node_removal_hook_holder = symtab->add_cgraph_removal_hook(
        &flatten_remove_node_hook, flatten_removed_nodes);
  }

  /* In the first pass handle functions to be flattened.  Do this with
     a priority so none of our later choices will make this impossible.  */
  for (i = nnodes - 1; i > j; i--) {
    node = order[i];
    if (flatten_removed_nodes && flatten_removed_nodes->contains(node))
      continue;

    /* Handle nodes to be flattened.
   Ideally when processing callees we stop inlining at the
   entry of cycles, possibly cloning that entry point and
   try to flatten itself turning it into a self-recursive
   function.  */
    if (dump_file) fprintf(dump_file, "Flattening %s\n", node->dump_name());
    flatten_function(node, false, true);
  }

  if (j < nnodes - 2) {
    symtab->remove_cgraph_removal_hook(node_removal_hook_holder);
    delete flatten_removed_nodes;
  }
  free(order);

  if (dump_file) dump_overall_stats();

  inline_small_functions();

  gcc_assert(symtab->state == IPA_SSA);
  symtab->state = IPA_SSA_AFTER_INLINING;
  /* Do first after-inlining removal.  We want to remove all "stale" extern
     inline functions and virtual functions so we really know what is called
     once.  */
  symtab->remove_unreachable_nodes(dump_file);

  /* Inline functions with a property that after inlining into all callers the
     code size will shrink because the out-of-line copy is eliminated.
     We do this regardless on the callee size as long as function growth limits
     are met.  */
  if (dump_file)
    fprintf(dump_file,
            "\nDeciding on functions to be inlined into all callers and "
            "removing useless speculations:\n");

  /* Inlining one function called once has good chance of preventing
     inlining other function into the same callee.  Ideally we should
     work in priority order, but probably inlining hot functions first
     is good cut without the extra pain of maintaining the queue.

     ??? this is not really fitting the bill perfectly: inlining function
     into callee often leads to better optimization of callee due to
     increased context for optimization.
     For example if main() function calls a function that outputs help
     and then function that does the main optimization, we should inline
     the second with priority even if both calls are cold by themselves.

     We probably want to implement new predicate replacing our use of
     maybe_hot_edge interpreted as maybe_hot_edge || callee is known
     to be hot.  */
  for (cold = 0; cold <= 1; cold++) {
    FOR_EACH_DEFINED_FUNCTION(node) {
      struct cgraph_edge *edge, *next;
      bool update = false;

      if (!opt_for_fn(node->decl, optimize) ||
          !opt_for_fn(node->decl, flag_inline_functions_called_once))
        continue;

      for (edge = node->callees; edge; edge = next) {
        next = edge->next_callee;
        if (edge->speculative && !speculation_useful_p(edge, false)) {
          if (edge->count.ipa().initialized_p()) spec_rem += edge->count.ipa();
          cgraph_edge::resolve_speculation(edge);
          update = true;
          remove_functions = true;
        }
      }
      if (update) {
        struct cgraph_node *where = node->inlined_to ? node->inlined_to : node;
        reset_edge_caches(where);
        ipa_update_overall_fn_summary(where);
      }
      if (want_inline_function_to_all_callers_p(node, cold)) {
        int num_calls = 0;
        node->call_for_symbol_and_aliases(sum_callers, &num_calls, true);
        while (node->call_for_symbol_and_aliases(inline_to_all_callers,
                                                 &num_calls, true))
          ;
        remove_functions = true;
      }
    }
  }

  if (dump_enabled_p())
    dump_printf(MSG_NOTE, "\nInlined %i calls, eliminated %i functions\n\n",
                ncalls_inlined, nfunctions_inlined);
  if (dump_file) dump_inline_stats();

  if (dump_file) ipa_dump_fn_summaries(dump_file);
  return remove_functions ? TODO_remove_functions : 0;
}
/* ===------------------------- IPA-Inline-Pass -------------------------=== */

/* ===------------------------- Loop-Unroll-Pass -------------------------=== */
struct iv_to_split {
  rtx_insn *insn; /* The insn in that the induction variable occurs.  */
  rtx orig_var;   /* The variable (register) for the IV before split.  */
  rtx base_var;   /* The variable on that the values in the further
             iterations are based.  */
  rtx step;       /* Step of the induction variable.  */
  struct iv_to_split *next; /* Next entry in walking order.  */
};

/* Information about accumulators to expand.  */

struct var_to_expand {
  rtx_insn *insn; /* The insn in that the variable expansion occurs.  */
  rtx reg;        /* The accumulator which is expanded.  */
  vec<rtx>
      var_expansions; /* The copies of the accumulator which is expanded.  */
  struct var_to_expand *next; /* Next entry in walking order.  */
  enum rtx_code op;    /* The type of the accumulation - addition, subtraction
                          or multiplication.  */
  int expansion_count; /* Count the number of expansions generated so far.  */
  int reuse_expansion; /* The expansion we intend to reuse to expand
                          the accumulator.  If REUSE_EXPANSION is 0 reuse
                          the original accumulator.  Else use
                          var_expansions[REUSE_EXPANSION - 1].  */
};

/* Hashtable helper for iv_to_split.  */

struct iv_split_hasher : free_ptr_hash<iv_to_split> {
  static inline hashval_t hash(const iv_to_split *);
  static inline bool equal(const iv_to_split *, const iv_to_split *);
};

/* A hash function for information about insns to split.  */

inline hashval_t iv_split_hasher::hash(const iv_to_split *ivts) {
  return (hashval_t)INSN_UID(ivts->insn);
}

/* An equality functions for information about insns to split.  */

inline bool iv_split_hasher::equal(const iv_to_split *i1,
                                   const iv_to_split *i2) {
  return i1->insn == i2->insn;
}

/* Hashtable helper for iv_to_split.  */

struct var_expand_hasher : free_ptr_hash<var_to_expand> {
  static inline hashval_t hash(const var_to_expand *);
  static inline bool equal(const var_to_expand *, const var_to_expand *);
};

/* Return a hash for VES.  */

inline hashval_t var_expand_hasher::hash(const var_to_expand *ves) {
  return (hashval_t)INSN_UID(ves->insn);
}

/* Return true if I1 and I2 refer to the same instruction.  */

inline bool var_expand_hasher::equal(const var_to_expand *i1,
                                     const var_to_expand *i2) {
  return i1->insn == i2->insn;
}

/* Information about optimization applied in
   the unrolled loop.  */

struct opt_info {
  hash_table<iv_split_hasher> *insns_to_split; /* A hashtable of insns to
                          split.  */
  struct iv_to_split *iv_to_split_head;        /* The first iv to split.  */
  struct iv_to_split **iv_to_split_tail; /* Pointer to the tail of the list.  */
  hash_table<var_expand_hasher> *insns_with_var_to_expand; /* A hashtable of
                    insns with accumulators to expand.  */
  struct var_to_expand *var_to_expand_head; /* The first var to expand.  */
  struct var_to_expand *
      *var_to_expand_tail;    /* Pointer to the tail of the list.  */
  unsigned first_new_block;   /* The first basic block that was
                                 duplicated.  */
  basic_block loop_exit;      /* The loop exit basic block.  */
  basic_block loop_preheader; /* The loop preheader basic block.  */
};

static void decide_unroll_stupid(class loop *, int);
static void decide_unroll_constant_iterations(class loop *, int);
static void decide_unroll_runtime_iterations(class loop *, int);
static void unroll_loop_stupid(class loop *);
static void decide_unrolling_(int);
static void unroll_loop_constant_iterations(class loop *);
static void unroll_loop_runtime_iterations(class loop *);
static struct opt_info *analyze_insns_in_loop(class loop *);
static void opt_info_start_duplication(struct opt_info *);
static void apply_opt_in_copies(struct opt_info *, unsigned, bool, bool);
static void free_opt_info(struct opt_info *);
static struct var_to_expand *analyze_insn_to_expand_var(class loop *,
                                                        rtx_insn *);
static bool referenced_in_one_insn_in_loop_p(class loop *, rtx, int *);
static struct iv_to_split *analyze_iv_to_split_insn(rtx_insn *);
static void expand_var_during_unrolling(struct var_to_expand *, rtx_insn *);
static void insert_var_expansion_initialization(struct var_to_expand *,
                                                basic_block);
static void combine_var_copies_in_loop_exit(struct var_to_expand *,
                                            basic_block);
static rtx get_expansion(struct var_to_expand *);

/* Emit a message summarizing the unroll that will be
   performed for LOOP, along with the loop's location LOCUS, if
   appropriate given the dump or -fopt-info settings.  */

static void report_unroll(class loop *loop, dump_location_t locus) {
  dump_flags_t report_flags = MSG_OPTIMIZED_LOCATIONS | TDF_DETAILS;

  if (loop->lpt_decision.decision == LPT_NONE) return;

  if (!dump_enabled_p()) return;

  dump_metadata_t metadata(report_flags, locus.get_impl_location());
  dump_printf_loc(metadata, locus.get_user_location(), "loop unrolled %d times",
                  loop->lpt_decision.times);
  if (profile_info && loop->header->count.initialized_p())
    dump_printf(metadata, " (header execution count %d)",
                (int)loop->header->count.to_gcov_type());

  dump_printf(metadata, "\n");
}

// Estimate unroll factor
int64_t get_unroll_factor(class loop *loop_) {
  auto util = ai4c::LoopUtil(loop_);
  std::vector<float> features = util.analyze_insns();
  std::vector<double> double_features(features.size());
  std::transform(features.begin(), features.end(), double_features.begin(),
                 [](float val) { return static_cast<double>(val); });
  int vec_size = double_features.size();

  initialize(g_unroll_model_path);
  add_int64_input(simultaneous_prefetches, 1);
  add_int64_input(l1_cache_size, 1);
  add_int64_input(l1_cache_line_size, 1);
  add_int64_input(l2_cache_size, 1);
  add_int64_input(prefetch_latency, 1);
  add_int64_input(ipa_prefetch_distance_factor, 1);
  add_string_input(var_str, 1);
  add_double_input(double_features.data(), vec_size);
  int err = inference();
  if (err) return err;
  int64_t *result = get_int64_output(0);

  int64_t label = result[0];
  return label;
}

static bool loop_exit_at_end_p(class loop *loop) {
  class niter_desc *desc = get_simple_loop_desc(loop);
  rtx_insn *insn;

  /* We should never have conditional in latch block.  */
  gcc_assert(desc->in_edge->dest != loop->header);

  if (desc->in_edge->dest != loop->latch) return false;

  /* Check that the latch is empty.  */
  FOR_BB_INSNS(loop->latch, insn) {
    if (INSN_P(insn) && active_insn_p(insn)) return false;
  }

  return true;
}

/* Decide whether unroll loops and how much.  */
static void decide_unrolling_(int flags) {
  /* Scan the loops, inner ones first. */
  int64_t label;

  for (auto loop : loops_list(cfun, LI_FROM_INNERMOST)) {
    loop->lpt_decision.decision = LPT_NONE;
    dump_user_location_t locus = get_loop_location(loop);
    std::string functionName = current_function_name();
    if (dump_enabled_p())
      dump_printf_loc(MSG_NOTE, locus,
                      "considering unrolling loop %d at BB %d\n", loop->num,
                      loop->header->index);

    if (loop->unroll == 1) {
      if (dump_file)
        fprintf(dump_file,
                ";; Not unrolling loop, user didn't want it unrolled\n");
      continue;
    }

    /* Do not peel cold areas.  */
    if (optimize_loop_for_size_p(loop)) {
      if (dump_file) fprintf(dump_file, ";; Not considering loop, cold area\n");
      continue;
    }

    /* Can the loop be manipulated?  */
    if (!can_duplicate_loop_p(loop)) {
      if (dump_file)
        fprintf(dump_file, ";; Not considering loop, cannot duplicate\n");
      continue;
    }

    /* Skip non-innermost loops.  */
    if (loop->inner) {
      if (dump_file)
        fprintf(dump_file, ";; Not considering loop, is not innermost\n");
      continue;
    }

    loop->ninsns = num_loop_insns(loop);
    loop->av_ninsns = average_num_loop_insns(loop);

    /* Try transformations one by one in decreasing order of priority.  */
    decide_unroll_constant_iterations(loop, flags);
    if (loop->lpt_decision.decision == LPT_NONE)
      decide_unroll_runtime_iterations(loop, flags);
    if (loop->lpt_decision.decision == LPT_NONE)
      decide_unroll_stupid(loop, flags);

    if (loop->lpt_decision.decision == LPT_NONE) {
      continue;
    }

    // get unroll factor
    int64_t index = loop->header->index;
    std::pair<std::string, int64_t> key = std::make_pair(functionName, index);
    if (function_map.find(key) != function_map.end()) {
      label = function_map[key];
    } else {
      label = get_unroll_factor(loop);
      function_map[key] = label;
    }

    loop->unroll = label;

    if (loop->unroll == 1 || loop->unroll == 0) {
      if (dump_file)
        fprintf(dump_file,
                ";; Not unrolling loop, user didn't want it unrolled\n");
      continue;
    }

    if (loop->lpt_decision.decision == LPT_UNROLL_CONSTANT) {
      loop->lpt_decision.times = loop->unroll - 1;
    } else {
      unsigned i, nunroll;
      if (loop->unroll > 0 && loop->unroll < USHRT_MAX) {
        nunroll = loop->unroll;

        for (i = 1; 2 * i <= nunroll; i *= 2) continue;
        loop->lpt_decision.times = i - 1;
      }
    }

    report_unroll(loop, locus);
  }
}

/* Unroll LOOPS.  */
void unroll_loops_(int flags) {
  bool changed = false;
  /* Now decide rest of unrolling.  */
  decide_unrolling_(flags);
  /* Scan the loops, inner ones first.  */
  for (auto loop : loops_list(cfun, LI_FROM_INNERMOST)) {
    /* And perform the appropriate transformations.  */
    switch (loop->lpt_decision.decision) {
      case LPT_UNROLL_CONSTANT:
        unroll_loop_constant_iterations(loop);
        changed = true;
        break;
      case LPT_UNROLL_RUNTIME:
        unroll_loop_runtime_iterations(loop);
        changed = true;
        break;
      case LPT_UNROLL_STUPID:
        unroll_loop_stupid(loop);
        changed = true;
        break;
      case LPT_NONE:
        break;
      default:
        gcc_unreachable();
    }
  }

  if (changed) {
    calculate_dominance_info(CDI_DOMINATORS);
    fix_loop_structure(NULL);
  }

  iv_analysis_done();
}

/* Check whether exit of the LOOP is at the end of loop body.  */

/* Decide whether to unroll LOOP iterating constant number of times
   and how much.  */

static void decide_unroll_constant_iterations(class loop *loop, int flags) {
  unsigned nunroll, nunroll_by_av, best_copies, best_unroll = 0, n_copies, i;
  class niter_desc *desc;
  widest_int iterations;

  /* If we were not asked to unroll this loop, just return back silently.  */
  if (!(flags & UAP_UNROLL) && !loop->unroll) return;

  if (dump_enabled_p())
    dump_printf(MSG_NOTE,
                "considering unrolling loop with constant "
                "number of iterations\n");

  /* nunroll = total number of copies of the original loop body in
     unrolled loop (i.e. if it is 2, we have to duplicate loop body once).  */
  nunroll = param_max_unrolled_insns / loop->ninsns;
  nunroll_by_av = param_max_average_unrolled_insns / loop->av_ninsns;
  if (nunroll > nunroll_by_av) nunroll = nunroll_by_av;
  if (nunroll > (unsigned)param_max_unroll_times)
    nunroll = param_max_unroll_times;

  if (targetm.loop_unroll_adjust)
    nunroll = targetm.loop_unroll_adjust(nunroll, loop);

  /* Skip big loops.  */
  if (nunroll <= 1) {
    if (dump_file) fprintf(dump_file, ";; Not considering loop, is too big\n");
    return;
  }

  /* Check for simple loops.  */
  desc = get_simple_loop_desc(loop);

  /* Check number of iterations.  */
  if (!desc->simple_p || !desc->const_iter || desc->assumptions) {
    if (dump_file)
      fprintf(dump_file,
              ";; Unable to prove that the loop iterates constant times\n");
    return;
  }

  /* Check for an explicit unrolling factor.  */
  if (loop->unroll > 0 && loop->unroll < USHRT_MAX) {
    /* However we cannot unroll completely at the RTL level a loop with
   constant number of iterations; it should have been peeled instead.  */
    if (desc->niter == 0 || (unsigned)loop->unroll > desc->niter - 1) {
      if (dump_file) fprintf(dump_file, ";; Loop should have been peeled\n");
    } else {
      loop->lpt_decision.decision = LPT_UNROLL_CONSTANT;
      loop->lpt_decision.times = loop->unroll - 1;
    }
    return;
  }

  /* Check whether the loop rolls enough to consider.
     Consult also loop bounds and profile; in the case the loop has more
     than one exit it may well loop less than determined maximal number
     of iterations.  */
  if (desc->niter < 2 * nunroll ||
      ((get_estimated_loop_iterations(loop, &iterations) ||
        get_likely_max_loop_iterations(loop, &iterations)) &&
       wi::ltu_p(iterations, 2 * nunroll))) {
    if (dump_file) fprintf(dump_file, ";; Not unrolling loop, doesn't roll\n");
    return;
  }

  /* Success; now compute number of iterations to unroll.  We alter
     nunroll so that as few as possible copies of loop body are
     necessary, while still not decreasing the number of unrollings
     too much (at most by 1).  */
  best_copies = 2 * nunroll + 10;

  i = 2 * nunroll + 2;
  if (i > desc->niter - 2) i = desc->niter - 2;

  for (; i >= nunroll - 1; i--) {
    unsigned exit_mod = desc->niter % (i + 1);

    if (!loop_exit_at_end_p(loop))
      n_copies = exit_mod + i + 1;
    else if (exit_mod != (unsigned)i || desc->noloop_assumptions != NULL_RTX)
      n_copies = exit_mod + i + 2;
    else
      n_copies = i + 1;

    if (n_copies < best_copies) {
      best_copies = n_copies;
      best_unroll = i;
    }
  }

  loop->lpt_decision.decision = LPT_UNROLL_CONSTANT;
  loop->lpt_decision.times = best_unroll;
}

/* Unroll LOOP with constant number of iterations LOOP->LPT_DECISION.TIMES
   times. The transformation does this:

   for (i = 0; i < 102; i++)
     body;

   ==>  (LOOP->LPT_DECISION.TIMES == 3)

   i = 0;
   body; i++;
   body; i++;
   while (i < 102)
     {
       body; i++;
       body; i++;
       body; i++;
       body; i++;
     }
  */
static void unroll_loop_constant_iterations(class loop *loop) {
  unsigned HOST_WIDE_INT niter;
  unsigned exit_mod;
  unsigned i;
  edge e;
  unsigned max_unroll = loop->lpt_decision.times;
  class niter_desc *desc = get_simple_loop_desc(loop);
  bool exit_at_end = loop_exit_at_end_p(loop);
  struct opt_info *opt_info = NULL;
  bool ok;

  niter = desc->niter;

  /* Should not get here (such loop should be peeled instead).  */
  gcc_assert(niter > max_unroll + 1);

  exit_mod = niter % (max_unroll + 1);

  auto_sbitmap wont_exit(max_unroll + 2);
  bitmap_ones(wont_exit);

  auto_vec<edge> remove_edges;
  if (flag_split_ivs_in_unroller || flag_variable_expansion_in_unroller)
    opt_info = analyze_insns_in_loop(loop);

  if (!exit_at_end) {
    /* The exit is not at the end of the loop; leave exit test
   in the first copy, so that the loops that start with test
   of exit condition have continuous body after unrolling.  */

    if (dump_file) fprintf(dump_file, ";; Condition at beginning of loop.\n");

    /* Peel exit_mod iterations.  */
    bitmap_clear_bit(wont_exit, 0);
    if (desc->noloop_assumptions) bitmap_clear_bit(wont_exit, 1);

    if (exit_mod) {
      opt_info_start_duplication(opt_info);
      ok = duplicate_loop_body_to_header_edge(
          loop, loop_preheader_edge(loop), exit_mod, wont_exit, desc->out_edge,
          &remove_edges,
          DLTHE_FLAG_UPDATE_FREQ |
              (opt_info && exit_mod > 1 ? DLTHE_RECORD_COPY_NUMBER : 0));
      gcc_assert(ok);

      if (opt_info && exit_mod > 1)
        apply_opt_in_copies(opt_info, exit_mod, false, false);

      desc->noloop_assumptions = NULL_RTX;
      desc->niter -= exit_mod;
      loop->nb_iterations_upper_bound -= exit_mod;
      if (loop->any_estimate &&
          wi::leu_p(exit_mod, loop->nb_iterations_estimate))
        loop->nb_iterations_estimate -= exit_mod;
      else
        loop->any_estimate = false;
      if (loop->any_likely_upper_bound &&
          wi::leu_p(exit_mod, loop->nb_iterations_likely_upper_bound))
        loop->nb_iterations_likely_upper_bound -= exit_mod;
      else
        loop->any_likely_upper_bound = false;
    }

    bitmap_set_bit(wont_exit, 1);
  } else {
    /* Leave exit test in last copy, for the same reason as above if
   the loop tests the condition at the end of loop body.  */

    if (dump_file) fprintf(dump_file, ";; Condition at end of loop.\n");

    /* We know that niter >= max_unroll + 2; so we do not need to care of
   case when we would exit before reaching the loop.  So just peel
   exit_mod + 1 iterations.  */
    if (exit_mod != max_unroll || desc->noloop_assumptions) {
      bitmap_clear_bit(wont_exit, 0);
      if (desc->noloop_assumptions) bitmap_clear_bit(wont_exit, 1);

      opt_info_start_duplication(opt_info);
      ok = duplicate_loop_body_to_header_edge(
          loop, loop_preheader_edge(loop), exit_mod + 1, wont_exit,
          desc->out_edge, &remove_edges,
          DLTHE_FLAG_UPDATE_FREQ |
              (opt_info && exit_mod > 0 ? DLTHE_RECORD_COPY_NUMBER : 0));
      gcc_assert(ok);

      if (opt_info && exit_mod > 0)
        apply_opt_in_copies(opt_info, exit_mod + 1, false, false);

      desc->niter -= exit_mod + 1;
      loop->nb_iterations_upper_bound -= exit_mod + 1;
      if (loop->any_estimate &&
          wi::leu_p(exit_mod + 1, loop->nb_iterations_estimate))
        loop->nb_iterations_estimate -= exit_mod + 1;
      else
        loop->any_estimate = false;
      if (loop->any_likely_upper_bound &&
          wi::leu_p(exit_mod + 1, loop->nb_iterations_likely_upper_bound))
        loop->nb_iterations_likely_upper_bound -= exit_mod + 1;
      else
        loop->any_likely_upper_bound = false;
      desc->noloop_assumptions = NULL_RTX;

      bitmap_set_bit(wont_exit, 0);
      bitmap_set_bit(wont_exit, 1);
    }

    bitmap_clear_bit(wont_exit, max_unroll);
  }

  /* Now unroll the loop.  */

  opt_info_start_duplication(opt_info);
  ok = duplicate_loop_body_to_header_edge(
      loop, loop_latch_edge(loop), max_unroll, wont_exit, desc->out_edge,
      &remove_edges,
      DLTHE_FLAG_UPDATE_FREQ | (opt_info ? DLTHE_RECORD_COPY_NUMBER : 0));
  gcc_assert(ok);

  if (opt_info) {
    apply_opt_in_copies(opt_info, max_unroll, true, true);
    free_opt_info(opt_info);
  }

  if (exit_at_end) {
    basic_block exit_block = get_bb_copy(desc->in_edge->src);
    /* Find a new in and out edge; they are in the last copy we have made.  */

    if (EDGE_SUCC(exit_block, 0)->dest == desc->out_edge->dest) {
      desc->out_edge = EDGE_SUCC(exit_block, 0);
      desc->in_edge = EDGE_SUCC(exit_block, 1);
    } else {
      desc->out_edge = EDGE_SUCC(exit_block, 1);
      desc->in_edge = EDGE_SUCC(exit_block, 0);
    }
  }

  desc->niter /= max_unroll + 1;
  loop->nb_iterations_upper_bound =
      wi::udiv_trunc(loop->nb_iterations_upper_bound, max_unroll + 1);
  if (loop->any_estimate)
    loop->nb_iterations_estimate =
        wi::udiv_trunc(loop->nb_iterations_estimate, max_unroll + 1);
  if (loop->any_likely_upper_bound)
    loop->nb_iterations_likely_upper_bound =
        wi::udiv_trunc(loop->nb_iterations_likely_upper_bound, max_unroll + 1);
  desc->niter_expr = gen_int_mode(desc->niter, desc->mode);

  /* Remove the edges.  */
  FOR_EACH_VEC_ELT(remove_edges, i, e)
  remove_path(e);

  if (dump_file)
    fprintf(dump_file,
            ";; Unrolled loop %d times, constant # of iterations %i insns\n",
            max_unroll, num_loop_insns(loop));
}

/* Decide whether to unroll LOOP iterating runtime computable number of times
   and how much.  */
static void decide_unroll_runtime_iterations(class loop *loop, int flags) {
  unsigned nunroll, nunroll_by_av, i;
  class niter_desc *desc;
  widest_int iterations;

  /* If we were not asked to unroll this loop, just return back silently.  */
  if (!(flags & UAP_UNROLL) && !loop->unroll) return;

  if (dump_enabled_p())
    dump_printf(MSG_NOTE,
                "considering unrolling loop with runtime-"
                "computable number of iterations\n");

  /* nunroll = total number of copies of the original loop body in
     unrolled loop (i.e. if it is 2, we have to duplicate loop body once.  */
  nunroll = param_max_unrolled_insns / loop->ninsns;
  nunroll_by_av = param_max_average_unrolled_insns / loop->av_ninsns;
  if (nunroll > nunroll_by_av) nunroll = nunroll_by_av;
  if (nunroll > (unsigned)param_max_unroll_times)
    nunroll = param_max_unroll_times;

  if (targetm.loop_unroll_adjust)
    nunroll = targetm.loop_unroll_adjust(nunroll, loop);

  if (loop->unroll > 0 && loop->unroll < USHRT_MAX) nunroll = loop->unroll;

  /* Skip big loops.  */
  if (nunroll <= 1) {
    if (dump_file) fprintf(dump_file, ";; Not considering loop, is too big\n");
    return;
  }

  /* Check for simple loops.  */
  desc = get_simple_loop_desc(loop);

  /* Check simpleness.  */
  if (!desc->simple_p || desc->assumptions) {
    if (dump_file)
      fprintf(dump_file,
              ";; Unable to prove that the number of iterations "
              "can be counted in runtime\n");
    return;
  }

  if (desc->const_iter) {
    if (dump_file) fprintf(dump_file, ";; Loop iterates constant times\n");
    return;
  }

  /* Check whether the loop rolls.  */
  if ((get_estimated_loop_iterations(loop, &iterations) ||
       get_likely_max_loop_iterations(loop, &iterations)) &&
      wi::ltu_p(iterations, 2 * nunroll)) {
    if (dump_file) fprintf(dump_file, ";; Not unrolling loop, doesn't roll\n");
    return;
  }

  /* Success; now force nunroll to be power of 2, as code-gen
     requires it, we are unable to cope with overflows in
     computation of number of iterations.  */
  for (i = 1; 2 * i <= nunroll; i *= 2) continue;

  loop->lpt_decision.decision = LPT_UNROLL_RUNTIME;
  loop->lpt_decision.times = i - 1;
}

/* Splits edge E and inserts the sequence of instructions INSNS on it, and
   returns the newly created block.  If INSNS is NULL_RTX, nothing is changed
   and NULL is returned instead.  */

basic_block split_edge_and_insert(edge e, rtx_insn *insns) {
  basic_block bb;

  if (!insns) return NULL;
  bb = split_edge(e);
  emit_insn_after(insns, BB_END(bb));

  /* ??? We used to assume that INSNS can contain control flow insns, and
     that we had to try to find sub basic blocks in BB to maintain a valid
     CFG.  For this purpose we used to set the BB_SUPERBLOCK flag on BB
     and call break_superblocks when going out of cfglayout mode.  But it
     turns out that this never happens; and that if it does ever happen,
     the verify_flow_info at the end of the RTL loop passes would fail.

     There are two reasons why we expected we could have control flow insns
     in INSNS.  The first is when a comparison has to be done in parts, and
     the second is when the number of iterations is computed for loops with
     the number of iterations known at runtime.  In both cases, test cases
     to get control flow in INSNS appear to be impossible to construct:

      * If do_compare_rtx_and_jump needs several branches to do comparison
    in a mode that needs comparison by parts, we cannot analyze the
    number of iterations of the loop, and we never get to unrolling it.

      * The code in expand_divmod that was suspected to cause creation of
    branching code seems to be only accessed for signed division.  The
    divisions used by # of iterations analysis are always unsigned.
    Problems might arise on architectures that emits branching code
    for some operations that may appear in the unroller (especially
    for division), but we have no such architectures.

     Considering all this, it was decided that we should for now assume
     that INSNS can in theory contain control flow insns, but in practice
     it never does.  So we don't handle the theoretical case, and should
     a real failure ever show up, we have a pretty good clue for how to
     fix it.  */

  return bb;
}

/* Prepare a sequence comparing OP0 with OP1 using COMP and jumping to LABEL if
   true, with probability PROB.  If CINSN is not NULL, it is the insn to copy
   in order to create a jump.  */

static rtx_insn *compare_and_jump_seq(rtx op0, rtx op1, enum rtx_code comp,
                                      rtx_code_label *label,
                                      profile_probability prob,
                                      rtx_insn *cinsn) {
  rtx_insn *seq;
  rtx_jump_insn *jump;
  rtx cond;
  machine_mode mode;

  mode = GET_MODE(op0);
  if (mode == VOIDmode) mode = GET_MODE(op1);

  start_sequence();
  if (GET_MODE_CLASS(mode) == MODE_CC) {
    /* A hack -- there seems to be no easy generic way how to make a
   conditional jump from a ccmode comparison.  */
    gcc_assert(cinsn);
    cond = XEXP(SET_SRC(pc_set(cinsn)), 0);
    gcc_assert(GET_CODE(cond) == comp);
    gcc_assert(rtx_equal_p(op0, XEXP(cond, 0)));
    gcc_assert(rtx_equal_p(op1, XEXP(cond, 1)));
    emit_jump_insn(copy_insn(PATTERN(cinsn)));
    jump = as_a<rtx_jump_insn *>(get_last_insn());
    JUMP_LABEL(jump) = JUMP_LABEL(cinsn);
    LABEL_NUSES(JUMP_LABEL(jump))++;
    redirect_jump(jump, label, 0);
  } else {
    gcc_assert(!cinsn);

    op0 = force_operand(op0, NULL_RTX);
    op1 = force_operand(op1, NULL_RTX);
    do_compare_rtx_and_jump(op0, op1, comp, 0, mode, NULL_RTX, NULL, label,
                            profile_probability::uninitialized());
    jump = as_a<rtx_jump_insn *>(get_last_insn());
    jump->set_jump_target(label);
    LABEL_NUSES(label)++;
  }
  if (prob.initialized_p()) add_reg_br_prob_note(jump, prob);

  seq = get_insns();
  end_sequence();

  return seq;
}

/* Unroll LOOP for which we are able to count number of iterations in
   runtime LOOP->LPT_DECISION.TIMES times.  The times value must be a
   power of two.  The transformation does this (with some extra care
   for case n < 0):

   for (i = 0; i < n; i++)
     body;

   ==>  (LOOP->LPT_DECISION.TIMES == 3)

   i = 0;
   mod = n % 4;

   switch (mod)
     {
       case 3:
         body; i++;
       case 2:
         body; i++;
       case 1:
         body; i++;
       case 0: ;
     }

   while (i < n)
     {
       body; i++;
       body; i++;
       body; i++;
       body; i++;
     }
   */
static void unroll_loop_runtime_iterations(class loop *loop) {
  rtx old_niter, niter, tmp;
  rtx_insn *init_code, *branch_code;
  unsigned i;
  profile_probability p;
  basic_block preheader, *body, swtch, ezc_swtch = NULL;
  int may_exit_copy;
  profile_count iter_count, new_count;
  unsigned n_peel;
  edge e;
  bool extra_zero_check, last_may_exit;
  unsigned max_unroll = loop->lpt_decision.times;
  class niter_desc *desc = get_simple_loop_desc(loop);
  bool exit_at_end = loop_exit_at_end_p(loop);
  struct opt_info *opt_info = NULL;
  bool ok;

  if (flag_split_ivs_in_unroller || flag_variable_expansion_in_unroller)
    opt_info = analyze_insns_in_loop(loop);

  /* Remember blocks whose dominators will have to be updated.  */
  auto_vec<basic_block> dom_bbs;

  body = get_loop_body(loop);
  for (i = 0; i < loop->num_nodes; i++) {
    for (basic_block bb : get_dominated_by(CDI_DOMINATORS, body[i]))
      if (!flow_bb_inside_loop_p(loop, bb)) dom_bbs.safe_push(bb);
  }
  free(body);

  if (!exit_at_end) {
    /* Leave exit in first copy (for explanation why see comment in
   unroll_loop_constant_iterations).  */
    may_exit_copy = 0;
    n_peel = max_unroll - 1;
    extra_zero_check = true;
    last_may_exit = false;
  } else {
    /* Leave exit in last copy (for explanation why see comment in
   unroll_loop_constant_iterations).  */
    may_exit_copy = max_unroll;
    n_peel = max_unroll;
    extra_zero_check = false;
    last_may_exit = true;
  }

  /* Get expression for number of iterations.  */
  start_sequence();
  old_niter = niter = gen_reg_rtx(desc->mode);
  tmp = force_operand(copy_rtx(desc->niter_expr), niter);
  if (tmp != niter) emit_move_insn(niter, tmp);

  /* For loops that exit at end and whose number of iterations is reliable,
     add one to niter to account for first pass through loop body before
     reaching exit test. */
  if (exit_at_end && !desc->noloop_assumptions) {
    niter = expand_simple_binop(desc->mode, PLUS, niter, const1_rtx, NULL_RTX,
                                0, OPTAB_LIB_WIDEN);
    old_niter = niter;
  }

  /* Count modulo by ANDing it with max_unroll; we use the fact that
     the number of unrollings is a power of two, and thus this is correct
     even if there is overflow in the computation.  */
  niter = expand_simple_binop(desc->mode, AND, niter,
                              gen_int_mode(max_unroll, desc->mode), NULL_RTX, 0,
                              OPTAB_LIB_WIDEN);

  init_code = get_insns();
  end_sequence();
  unshare_all_rtl_in_chain(init_code);

  /* Precondition the loop.  */
  split_edge_and_insert(loop_preheader_edge(loop), init_code);

  auto_vec<edge> remove_edges;

  auto_sbitmap wont_exit(max_unroll + 2);

  if (extra_zero_check || desc->noloop_assumptions) {
    /* Peel the first copy of loop body.  Leave the exit test if the number
   of iterations is not reliable.  Also record the place of the extra zero
   check.  */
    bitmap_clear(wont_exit);
    if (!desc->noloop_assumptions) bitmap_set_bit(wont_exit, 1);
    ezc_swtch = loop_preheader_edge(loop)->src;
    ok = duplicate_loop_body_to_header_edge(
        loop, loop_preheader_edge(loop), 1, wont_exit, desc->out_edge,
        &remove_edges, DLTHE_FLAG_UPDATE_FREQ);
    gcc_assert(ok);
  }

  /* Record the place where switch will be built for preconditioning.  */
  swtch = split_edge(loop_preheader_edge(loop));

  /* Compute count increments for each switch block and initialize
     innermost switch block.  Switch blocks and peeled loop copies are built
     from innermost outward.  */
  iter_count = new_count = swtch->count.apply_scale(1, max_unroll + 1);
  swtch->count = new_count;

  for (i = 0; i < n_peel; i++) {
    /* Peel the copy.  */
    bitmap_clear(wont_exit);
    if (i != n_peel - 1 || !last_may_exit) bitmap_set_bit(wont_exit, 1);
    ok = duplicate_loop_body_to_header_edge(
        loop, loop_preheader_edge(loop), 1, wont_exit, desc->out_edge,
        &remove_edges, DLTHE_FLAG_UPDATE_FREQ);
    gcc_assert(ok);

    /* Create item for switch.  */
    unsigned j = n_peel - i - (extra_zero_check ? 0 : 1);
    p = profile_probability::always().apply_scale(1, i + 2);

    preheader = split_edge(loop_preheader_edge(loop));
    /* Add in count of edge from switch block.  */
    preheader->count += iter_count;
    branch_code =
        compare_and_jump_seq(copy_rtx(niter), gen_int_mode(j, desc->mode), EQ,
                             block_label(preheader), p, NULL);

    /* We rely on the fact that the compare and jump cannot be optimized out,
   and hence the cfg we create is correct.  */
    gcc_assert(branch_code != NULL_RTX);

    swtch = split_edge_and_insert(single_pred_edge(swtch), branch_code);
    set_immediate_dominator(CDI_DOMINATORS, preheader, swtch);
    single_succ_edge(swtch)->probability = p.invert();
    new_count += iter_count;
    swtch->count = new_count;
    e = make_edge(swtch, preheader,
                  single_succ_edge(swtch)->flags & EDGE_IRREDUCIBLE_LOOP);
    e->probability = p;
  }

  if (extra_zero_check) {
    /* Add branch for zero iterations.  */
    p = profile_probability::always().apply_scale(1, max_unroll + 1);
    swtch = ezc_swtch;
    preheader = split_edge(loop_preheader_edge(loop));
    /* Recompute count adjustments since initial peel copy may
   have exited and reduced those values that were computed above.  */
    iter_count = swtch->count.apply_scale(1, max_unroll + 1);
    /* Add in count of edge from switch block.  */
    preheader->count += iter_count;
    branch_code = compare_and_jump_seq(copy_rtx(niter), const0_rtx, EQ,
                                       block_label(preheader), p, NULL);
    gcc_assert(branch_code != NULL_RTX);

    swtch = split_edge_and_insert(single_succ_edge(swtch), branch_code);
    set_immediate_dominator(CDI_DOMINATORS, preheader, swtch);
    single_succ_edge(swtch)->probability = p.invert();
    e = make_edge(swtch, preheader,
                  single_succ_edge(swtch)->flags & EDGE_IRREDUCIBLE_LOOP);
    e->probability = p;
  }

  /* Recount dominators for outer blocks.  */
  iterate_fix_dominators(CDI_DOMINATORS, dom_bbs, false);

  /* And unroll loop.  */

  bitmap_ones(wont_exit);
  bitmap_clear_bit(wont_exit, may_exit_copy);
  opt_info_start_duplication(opt_info);

  ok = duplicate_loop_body_to_header_edge(
      loop, loop_latch_edge(loop), max_unroll, wont_exit, desc->out_edge,
      &remove_edges,
      DLTHE_FLAG_UPDATE_FREQ | (opt_info ? DLTHE_RECORD_COPY_NUMBER : 0));
  gcc_assert(ok);

  if (opt_info) {
    apply_opt_in_copies(opt_info, max_unroll, true, true);
    free_opt_info(opt_info);
  }

  if (exit_at_end) {
    basic_block exit_block = get_bb_copy(desc->in_edge->src);
    /* Find a new in and out edge; they are in the last copy we have
   made.  */

    if (EDGE_SUCC(exit_block, 0)->dest == desc->out_edge->dest) {
      desc->out_edge = EDGE_SUCC(exit_block, 0);
      desc->in_edge = EDGE_SUCC(exit_block, 1);
    } else {
      desc->out_edge = EDGE_SUCC(exit_block, 1);
      desc->in_edge = EDGE_SUCC(exit_block, 0);
    }
  }

  /* Remove the edges.  */
  FOR_EACH_VEC_ELT(remove_edges, i, e)
  remove_path(e);

  /* We must be careful when updating the number of iterations due to
     preconditioning and the fact that the value must be valid at entry
     of the loop.  After passing through the above code, we see that
     the correct new number of iterations is this:  */
  gcc_assert(!desc->const_iter);
  desc->niter_expr = simplify_gen_binary(
      UDIV, desc->mode, old_niter, gen_int_mode(max_unroll + 1, desc->mode));
  loop->nb_iterations_upper_bound =
      wi::udiv_trunc(loop->nb_iterations_upper_bound, max_unroll + 1);
  if (loop->any_estimate)
    loop->nb_iterations_estimate =
        wi::udiv_trunc(loop->nb_iterations_estimate, max_unroll + 1);
  if (loop->any_likely_upper_bound)
    loop->nb_iterations_likely_upper_bound =
        wi::udiv_trunc(loop->nb_iterations_likely_upper_bound, max_unroll + 1);
  if (exit_at_end) {
    desc->niter_expr =
        simplify_gen_binary(MINUS, desc->mode, desc->niter_expr, const1_rtx);
    desc->noloop_assumptions = NULL_RTX;
    --loop->nb_iterations_upper_bound;
    if (loop->any_estimate && loop->nb_iterations_estimate != 0)
      --loop->nb_iterations_estimate;
    else
      loop->any_estimate = false;
    if (loop->any_likely_upper_bound &&
        loop->nb_iterations_likely_upper_bound != 0)
      --loop->nb_iterations_likely_upper_bound;
    else
      loop->any_likely_upper_bound = false;
  }

  if (dump_file)
    fprintf(dump_file,
            ";; Unrolled loop %d times, counting # of iterations "
            "in runtime, %i insns\n",
            max_unroll, num_loop_insns(loop));
}

/* Decide whether to unroll LOOP stupidly and how much.  */
static void decide_unroll_stupid(class loop *loop, int flags) {
  unsigned nunroll, nunroll_by_av, i;
  class niter_desc *desc;
  widest_int iterations;

  /* If we were not asked to unroll this loop, just return back silently.  */
  if (!(flags & UAP_UNROLL_ALL) && !loop->unroll) return;

  if (dump_enabled_p())
    dump_printf(MSG_NOTE, "considering unrolling loop stupidly\n");

  /* nunroll = total number of copies of the original loop body in
     unrolled loop (i.e. if it is 2, we have to duplicate loop body once.  */
  nunroll = param_max_unrolled_insns / loop->ninsns;
  nunroll_by_av = param_max_average_unrolled_insns / loop->av_ninsns;
  if (nunroll > nunroll_by_av) nunroll = nunroll_by_av;
  if (nunroll > (unsigned)param_max_unroll_times)
    nunroll = param_max_unroll_times;

  if (targetm.loop_unroll_adjust)
    nunroll = targetm.loop_unroll_adjust(nunroll, loop);

  if (loop->unroll > 0 && loop->unroll < USHRT_MAX) nunroll = loop->unroll;

  /* Skip big loops.  */
  if (nunroll <= 1) {
    if (dump_file) fprintf(dump_file, ";; Not considering loop, is too big\n");
    return;
  }

  /* Check for simple loops.  */
  desc = get_simple_loop_desc(loop);

  /* Check simpleness.  */
  if (desc->simple_p && !desc->assumptions) {
    if (dump_file) fprintf(dump_file, ";; Loop is simple\n");
    return;
  }

  /* Do not unroll loops with branches inside -- it increases number
     of mispredicts.
     TODO: this heuristic needs tunning; call inside the loop body
     is also relatively good reason to not unroll.  */
  if (num_loop_branches(loop) > 1) {
    if (dump_file) fprintf(dump_file, ";; Not unrolling, contains branches\n");
    return;
  }

  /* Check whether the loop rolls.  */
  if ((get_estimated_loop_iterations(loop, &iterations) ||
       get_likely_max_loop_iterations(loop, &iterations)) &&
      wi::ltu_p(iterations, 2 * nunroll)) {
    if (dump_file) fprintf(dump_file, ";; Not unrolling loop, doesn't roll\n");
    return;
  }

  /* Success.  Now force nunroll to be power of 2, as it seems that this
     improves results (partially because of better alignments, partially
     because of some dark magic).  */
  for (i = 1; 2 * i <= nunroll; i *= 2) continue;

  loop->lpt_decision.decision = LPT_UNROLL_STUPID;
  loop->lpt_decision.times = i - 1;
}

/* Unroll a LOOP LOOP->LPT_DECISION.TIMES times.  The transformation does this:

   while (cond)
     body;

   ==>  (LOOP->LPT_DECISION.TIMES == 3)

   while (cond)
     {
       body;
       if (!cond) break;
       body;
       if (!cond) break;
       body;
       if (!cond) break;
       body;
     }
   */
static void unroll_loop_stupid(class loop *loop) {
  unsigned nunroll = loop->lpt_decision.times;
  class niter_desc *desc = get_simple_loop_desc(loop);
  struct opt_info *opt_info = NULL;
  bool ok;

  if (flag_split_ivs_in_unroller || flag_variable_expansion_in_unroller)
    opt_info = analyze_insns_in_loop(loop);

  auto_sbitmap wont_exit(nunroll + 1);
  bitmap_clear(wont_exit);
  opt_info_start_duplication(opt_info);

  ok = duplicate_loop_body_to_header_edge(
      loop, loop_latch_edge(loop), nunroll, wont_exit, NULL, NULL,
      DLTHE_FLAG_UPDATE_FREQ | (opt_info ? DLTHE_RECORD_COPY_NUMBER : 0));
  gcc_assert(ok);

  if (opt_info) {
    apply_opt_in_copies(opt_info, nunroll, true, true);
    free_opt_info(opt_info);
  }

  if (desc->simple_p) {
    /* We indeed may get here provided that there are nontrivial assumptions
   for a loop to be really simple.  We could update the counts, but the
   problem is that we are unable to decide which exit will be taken
   (not really true in case the number of iterations is constant,
   but no one will do anything with this information, so we do not
   worry about it).  */
    desc->simple_p = false;
  }

  if (dump_file)
    fprintf(dump_file, ";; Unrolled loop %d times, %i insns\n", nunroll,
            num_loop_insns(loop));
}

/* Returns true if REG is referenced in one nondebug insn in LOOP.
   Set *DEBUG_USES to the number of debug insns that reference the
   variable.  */

static bool referenced_in_one_insn_in_loop_p(class loop *loop, rtx reg,
                                             int *debug_uses) {
  basic_block *body, bb;
  unsigned i;
  int count_ref = 0;
  rtx_insn *insn;

  body = get_loop_body(loop);
  for (i = 0; i < loop->num_nodes; i++) {
    bb = body[i];

    FOR_BB_INSNS(bb, insn)
    if (!rtx_referenced_p(reg, insn))
      continue;
    else if (DEBUG_INSN_P(insn))
      ++*debug_uses;
    else if (++count_ref > 1)
      break;
  }
  free(body);
  return (count_ref == 1);
}

/* Reset the DEBUG_USES debug insns in LOOP that reference REG.  */

static void reset_debug_uses_in_loop(class loop *loop, rtx reg,
                                     int debug_uses) {
  basic_block *body, bb;
  unsigned i;
  rtx_insn *insn;

  body = get_loop_body(loop);
  for (i = 0; debug_uses && i < loop->num_nodes; i++) {
    bb = body[i];

    FOR_BB_INSNS(bb, insn)
    if (!DEBUG_INSN_P(insn) || !rtx_referenced_p(reg, insn))
      continue;
    else {
      validate_change(insn, &INSN_VAR_LOCATION_LOC(insn),
                      gen_rtx_UNKNOWN_VAR_LOC(), 0);
      if (!--debug_uses) break;
    }
  }
  free(body);
}

/* Determine whether INSN contains an accumulator
   which can be expanded into separate copies,
   one for each copy of the LOOP body.

   for (i = 0 ; i < n; i++)
     sum += a[i];

   ==>

   sum += a[i]
   ....
   i = i+1;
   sum1 += a[i]
   ....
   i = i+1
   sum2 += a[i];
   ....

   Return NULL if INSN contains no opportunity for expansion of accumulator.
   Otherwise, allocate a VAR_TO_EXPAND structure, fill it with the relevant
   information and return a pointer to it.
*/

static struct var_to_expand *analyze_insn_to_expand_var(class loop *loop,
                                                        rtx_insn *insn) {
  rtx set, dest, src;
  struct var_to_expand *ves;
  unsigned accum_pos;
  enum rtx_code code;
  int debug_uses = 0;

  set = single_set(insn);
  if (!set) return NULL;

  dest = SET_DEST(set);
  src = SET_SRC(set);
  code = GET_CODE(src);

  if (code != PLUS && code != MINUS && code != MULT && code != FMA) return NULL;

  if (FLOAT_MODE_P(GET_MODE(dest))) {
    if (!flag_associative_math) return NULL;
    /* In the case of FMA, we're also changing the rounding.  */
    if (code == FMA && !flag_unsafe_math_optimizations) return NULL;
  }

  /* Hmm, this is a bit paradoxical.  We know that INSN is a valid insn
     in MD.  But if there is no optab to generate the insn, we cannot
     perform the variable expansion.  This can happen if an MD provides
     an insn but not a named pattern to generate it, for example to avoid
     producing code that needs additional mode switches like for x87/mmx.

     So we check have_insn_for which looks for an optab for the operation
     in SRC.  If it doesn't exist, we can't perform the expansion even
     though INSN is valid.  */
  if (!have_insn_for(code, GET_MODE(src))) return NULL;

  if (!REG_P(dest) && !(GET_CODE(dest) == SUBREG && REG_P(SUBREG_REG(dest))))
    return NULL;

  /* Find the accumulator use within the operation.  */
  if (code == FMA) {
    /* We only support accumulation via FMA in the ADD position.  */
    if (!rtx_equal_p(dest, XEXP(src, 2))) return NULL;
    accum_pos = 2;
  } else if (rtx_equal_p(dest, XEXP(src, 0)))
    accum_pos = 0;
  else if (rtx_equal_p(dest, XEXP(src, 1))) {
    /* The method of expansion that we are using; which includes the
   initialization of the expansions with zero and the summation of
       the expansions at the end of the computation will yield wrong
   results for (x = something - x) thus avoid using it in that case.  */
    if (code == MINUS) return NULL;
    accum_pos = 1;
  } else
    return NULL;

  /* It must not otherwise be used.  */
  if (code == FMA) {
    if (rtx_referenced_p(dest, XEXP(src, 0)) ||
        rtx_referenced_p(dest, XEXP(src, 1)))
      return NULL;
  } else if (rtx_referenced_p(dest, XEXP(src, 1 - accum_pos)))
    return NULL;

  /* It must be used in exactly one insn.  */
  if (!referenced_in_one_insn_in_loop_p(loop, dest, &debug_uses)) return NULL;

  if (dump_file) {
    fprintf(dump_file, "\n;; Expanding Accumulator ");
    print_rtl(dump_file, dest);
    fprintf(dump_file, "\n");
  }

  if (debug_uses)
    /* Instead of resetting the debug insns, we could replace each
       debug use in the loop with the sum or product of all expanded
       accumulators.  Since we'll only know of all expansions at the
       end, we'd have to keep track of which vars_to_expand a debug
       insn in the loop references, take note of each copy of the
       debug insn during unrolling, and when it's all done, compute
       the sum or product of each variable and adjust the original
       debug insn and each copy thereof.  What a pain!  */
    reset_debug_uses_in_loop(loop, dest, debug_uses);

  /* Record the accumulator to expand.  */
  ves = XNEW(struct var_to_expand);
  ves->insn = insn;
  ves->reg = copy_rtx(dest);
  ves->var_expansions.create(1);
  ves->next = NULL;
  ves->op = GET_CODE(src);
  ves->expansion_count = 0;
  ves->reuse_expansion = 0;
  return ves;
}

/* Determine whether there is an induction variable in INSN that
   we would like to split during unrolling.

   I.e. replace

   i = i + 1;
   ...
   i = i + 1;
   ...
   i = i + 1;
   ...

   type chains by

   i0 = i + 1
   ...
   i = i0 + 1
   ...
   i = i0 + 2
   ...

   Return NULL if INSN contains no interesting IVs.  Otherwise, allocate
   an IV_TO_SPLIT structure, fill it with the relevant information and return a
   pointer to it.  */

static struct iv_to_split *analyze_iv_to_split_insn(rtx_insn *insn) {
  rtx set, dest;
  class rtx_iv iv;
  struct iv_to_split *ivts;
  scalar_int_mode mode;
  bool ok;

  /* For now we just split the basic induction variables.  Later this may be
     extended for example by selecting also addresses of memory references.  */
  set = single_set(insn);
  if (!set) return NULL;

  dest = SET_DEST(set);
  if (!REG_P(dest) || !is_a<scalar_int_mode>(GET_MODE(dest), &mode))
    return NULL;

  if (!biv_p(insn, mode, dest)) return NULL;

  ok = iv_analyze_result(insn, dest, &iv);

  /* This used to be an assert under the assumption that if biv_p returns
     true that iv_analyze_result must also return true.  However, that
     assumption is not strictly correct as evidenced by pr25569.

     Returning NULL when iv_analyze_result returns false is safe and
     avoids the problems in pr25569 until the iv_analyze_* routines
     can be fixed, which is apparently hard and time consuming
     according to their author.  */
  if (!ok) return NULL;

  if (iv.step == const0_rtx || iv.mode != iv.extend_mode) return NULL;

  /* Record the insn to split.  */
  ivts = XNEW(struct iv_to_split);
  ivts->insn = insn;
  ivts->orig_var = dest;
  ivts->base_var = NULL_RTX;
  ivts->step = iv.step;
  ivts->next = NULL;

  return ivts;
}

/* Determines which of insns in LOOP can be optimized.
   Return a OPT_INFO struct with the relevant hash tables filled
   with all insns to be optimized.  The FIRST_NEW_BLOCK field
   is undefined for the return value.  */

static struct opt_info *analyze_insns_in_loop(class loop *loop) {
  basic_block *body, bb;
  unsigned i;
  struct opt_info *opt_info = XCNEW(struct opt_info);
  rtx_insn *insn;
  struct iv_to_split *ivts = NULL;
  struct var_to_expand *ves = NULL;
  iv_to_split **slot1;
  var_to_expand **slot2;
  auto_vec<edge> edges = get_loop_exit_edges(loop);
  edge exit;
  bool can_apply = false;

  iv_analysis_loop_init(loop);

  body = get_loop_body(loop);

  if (flag_split_ivs_in_unroller) {
    opt_info->insns_to_split =
        new hash_table<iv_split_hasher>(5 * loop->num_nodes);
    opt_info->iv_to_split_head = NULL;
    opt_info->iv_to_split_tail = &opt_info->iv_to_split_head;
  }

  /* Record the loop exit bb and loop preheader before the unrolling.  */
  opt_info->loop_preheader = loop_preheader_edge(loop)->src;

  if (edges.length() == 1) {
    exit = edges[0];
    if (!(exit->flags & EDGE_COMPLEX)) {
      opt_info->loop_exit = split_edge(exit);
      can_apply = true;
    }
  }

  if (flag_variable_expansion_in_unroller && can_apply) {
    opt_info->insns_with_var_to_expand =
        new hash_table<var_expand_hasher>(5 * loop->num_nodes);
    opt_info->var_to_expand_head = NULL;
    opt_info->var_to_expand_tail = &opt_info->var_to_expand_head;
  }

  for (i = 0; i < loop->num_nodes; i++) {
    bb = body[i];
    if (!dominated_by_p(CDI_DOMINATORS, loop->latch, bb)) continue;

    FOR_BB_INSNS(bb, insn) {
      if (!INSN_P(insn)) continue;

      if (opt_info->insns_to_split) ivts = analyze_iv_to_split_insn(insn);

      if (ivts) {
        slot1 = opt_info->insns_to_split->find_slot(ivts, INSERT);
        gcc_assert(*slot1 == NULL);
        *slot1 = ivts;
        *opt_info->iv_to_split_tail = ivts;
        opt_info->iv_to_split_tail = &ivts->next;
        continue;
      }

      if (opt_info->insns_with_var_to_expand)
        ves = analyze_insn_to_expand_var(loop, insn);

      if (ves) {
        slot2 = opt_info->insns_with_var_to_expand->find_slot(ves, INSERT);
        gcc_assert(*slot2 == NULL);
        *slot2 = ves;
        *opt_info->var_to_expand_tail = ves;
        opt_info->var_to_expand_tail = &ves->next;
      }
    }
  }

  free(body);
  return opt_info;
}

/* Called just before loop duplication.  Records start of duplicated area
   to OPT_INFO.  */

static void opt_info_start_duplication(struct opt_info *opt_info) {
  if (opt_info) opt_info->first_new_block = last_basic_block_for_fn(cfun);
}

/* Determine the number of iterations between initialization of the base
   variable and the current copy (N_COPY).  N_COPIES is the total number
   of newly created copies.  UNROLLING is true if we are unrolling
   (not peeling) the loop.  */

static unsigned determine_split_iv_delta(unsigned n_copy, unsigned n_copies,
                                         bool unrolling) {
  if (unrolling) {
    /* If we are unrolling, initialization is done in the original loop
   body (number 0).  */
    return n_copy;
  } else {
    /* If we are peeling, the copy in that the initialization occurs has
   number 1.  The original loop (number 0) is the last.  */
    if (n_copy)
      return n_copy - 1;
    else
      return n_copies;
  }
}

/* Allocate basic variable for the induction variable chain.  */

static void allocate_basic_variable(struct iv_to_split *ivts) {
  rtx expr = SET_SRC(single_set(ivts->insn));

  ivts->base_var = gen_reg_rtx(GET_MODE(expr));
}

/* Insert initialization of basic variable of IVTS before INSN, taking
   the initial value from INSN.  */

static void insert_base_initialization(struct iv_to_split *ivts,
                                       rtx_insn *insn) {
  rtx expr = copy_rtx(SET_SRC(single_set(insn)));
  rtx_insn *seq;

  start_sequence();
  expr = force_operand(expr, ivts->base_var);
  if (expr != ivts->base_var) emit_move_insn(ivts->base_var, expr);
  seq = get_insns();
  end_sequence();

  emit_insn_before(seq, insn);
}

/* Replace the use of induction variable described in IVTS in INSN
   by base variable + DELTA * step.  */

static void split_iv(struct iv_to_split *ivts, rtx_insn *insn, unsigned delta) {
  rtx expr, *loc, incr, var;
  rtx_insn *seq;
  machine_mode mode = GET_MODE(ivts->base_var);
  rtx src, dest, set;

  /* Construct base + DELTA * step.  */
  if (!delta)
    expr = ivts->base_var;
  else {
    incr = simplify_gen_binary(MULT, mode, copy_rtx(ivts->step),
                               gen_int_mode(delta, mode));
    expr = simplify_gen_binary(PLUS, GET_MODE(ivts->base_var), ivts->base_var,
                               incr);
  }

  /* Figure out where to do the replacement.  */
  loc = &SET_SRC(single_set(insn));

  /* If we can make the replacement right away, we're done.  */
  if (validate_change(insn, loc, expr, 0)) return;

  /* Otherwise, force EXPR into a register and try again.  */
  start_sequence();
  var = gen_reg_rtx(mode);
  expr = force_operand(expr, var);
  if (expr != var) emit_move_insn(var, expr);
  seq = get_insns();
  end_sequence();
  emit_insn_before(seq, insn);

  if (validate_change(insn, loc, var, 0)) return;

  /* The last chance.  Try recreating the assignment in insn
     completely from scratch.  */
  set = single_set(insn);
  gcc_assert(set);

  start_sequence();
  *loc = var;
  src = copy_rtx(SET_SRC(set));
  dest = copy_rtx(SET_DEST(set));
  src = force_operand(src, dest);
  if (src != dest) emit_move_insn(dest, src);
  seq = get_insns();
  end_sequence();

  emit_insn_before(seq, insn);
  delete_insn(insn);
}

/* Return one expansion of the accumulator recorded in struct VE.  */

static rtx get_expansion(struct var_to_expand *ve) {
  rtx reg;

  if (ve->reuse_expansion == 0)
    reg = ve->reg;
  else
    reg = ve->var_expansions[ve->reuse_expansion - 1];

  if (ve->var_expansions.length() == (unsigned)ve->reuse_expansion)
    ve->reuse_expansion = 0;
  else
    ve->reuse_expansion++;

  return reg;
}

/* Given INSN replace the uses of the accumulator recorded in VE
   with a new register.  */

static void expand_var_during_unrolling(struct var_to_expand *ve,
                                        rtx_insn *insn) {
  rtx new_reg, set;
  bool really_new_expansion = false;

  set = single_set(insn);
  gcc_assert(set);

  /* Generate a new register only if the expansion limit has not been
     reached.  Else reuse an already existing expansion.  */
  if (param_max_variable_expansions > ve->expansion_count) {
    really_new_expansion = true;
    new_reg = gen_reg_rtx(GET_MODE(ve->reg));
  } else
    new_reg = get_expansion(ve);

  validate_replace_rtx_group(SET_DEST(set), new_reg, insn);
  if (apply_change_group())
    if (really_new_expansion) {
      ve->var_expansions.safe_push(new_reg);
      ve->expansion_count++;
    }
}

/* Initialize the variable expansions in loop preheader.  PLACE is the
   loop-preheader basic block where the initialization of the
   expansions should take place.  The expansions are initialized with
   (-0) when the operation is plus or minus to honor sign zero.  This
   way we can prevent cases where the sign of the final result is
   effected by the sign of the expansion.  Here is an example to
   demonstrate this:

   for (i = 0 ; i < n; i++)
     sum += something;

   ==>

   sum += something
   ....
   i = i+1;
   sum1 += something
   ....
   i = i+1
   sum2 += something;
   ....

   When SUM is initialized with -zero and SOMETHING is also -zero; the
   final result of sum should be -zero thus the expansions sum1 and sum2
   should be initialized with -zero as well (otherwise we will get +zero
   as the final result).  */

static void insert_var_expansion_initialization(struct var_to_expand *ve,
                                                basic_block place) {
  rtx_insn *seq;
  rtx var, zero_init;
  unsigned i;
  machine_mode mode = GET_MODE(ve->reg);
  bool honor_signed_zero_p = HONOR_SIGNED_ZEROS(mode);

  if (ve->var_expansions.length() == 0) return;

  start_sequence();
  switch (ve->op) {
    case FMA:
      /* Note that we only accumulate FMA via the ADD operand.  */
    case PLUS:
    case MINUS:
      FOR_EACH_VEC_ELT(ve->var_expansions, i, var) {
        if (honor_signed_zero_p)
          zero_init =
              simplify_gen_unary(rtx_code::NEG, mode, CONST0_RTX(mode), mode);
        else
          zero_init = CONST0_RTX(mode);
        emit_move_insn(var, zero_init);
      }
      break;

    case MULT:
      FOR_EACH_VEC_ELT(ve->var_expansions, i, var) {
        zero_init = CONST1_RTX(GET_MODE(var));
        emit_move_insn(var, zero_init);
      }
      break;

    default:
      gcc_unreachable();
  }

  seq = get_insns();
  end_sequence();

  emit_insn_after(seq, BB_END(place));
}

/* Combine the variable expansions at the loop exit.  PLACE is the
   loop exit basic block where the summation of the expansions should
   take place.  */

static void combine_var_copies_in_loop_exit(struct var_to_expand *ve,
                                            basic_block place) {
  rtx sum = ve->reg;
  rtx expr, var;
  rtx_insn *seq, *insn;
  unsigned i;

  if (ve->var_expansions.length() == 0) return;

  /* ve->reg might be SUBREG or some other non-shareable RTL, and we use
     it both here and as the destination of the assignment.  */
  sum = copy_rtx(sum);
  start_sequence();
  switch (ve->op) {
    case FMA:
      /* Note that we only accumulate FMA via the ADD operand.  */
    case PLUS:
    case MINUS:
      FOR_EACH_VEC_ELT(ve->var_expansions, i, var)
      sum = simplify_gen_binary(PLUS, GET_MODE(ve->reg), var, sum);
      break;

    case MULT:
      FOR_EACH_VEC_ELT(ve->var_expansions, i, var)
      sum = simplify_gen_binary(MULT, GET_MODE(ve->reg), var, sum);
      break;

    default:
      gcc_unreachable();
  }

  expr = force_operand(sum, ve->reg);
  if (expr != ve->reg) emit_move_insn(ve->reg, expr);
  seq = get_insns();
  end_sequence();

  insn = BB_HEAD(place);
  while (!NOTE_INSN_BASIC_BLOCK_P(insn)) insn = NEXT_INSN(insn);

  emit_insn_after(seq, insn);
}

/* Strip away REG_EQUAL notes for IVs we're splitting.

   Updating REG_EQUAL notes for IVs we split is tricky: We
   cannot tell until after unrolling, DF-rescanning, and liveness
   updating, whether an EQ_USE is reached by the split IV while
   the IV reg is still live.  See PR55006.

   ??? We cannot use remove_reg_equal_equiv_notes_for_regno,
   because RTL loop-iv requires us to defer rescanning insns and
   any notes attached to them.  So resort to old techniques...  */

static void maybe_strip_eq_note_for_split_iv(struct opt_info *opt_info,
                                             rtx_insn *insn) {
  struct iv_to_split *ivts;
  rtx note = find_reg_equal_equiv_note(insn);
  if (!note) return;
  for (ivts = opt_info->iv_to_split_head; ivts; ivts = ivts->next)
    if (reg_mentioned_p(ivts->orig_var, note)) {
      remove_note(insn, note);
      return;
    }
}

/* Apply loop optimizations in loop copies using the
   data which gathered during the unrolling.  Structure
   OPT_INFO record that data.

   UNROLLING is true if we unrolled (not peeled) the loop.
   REWRITE_ORIGINAL_BODY is true if we should also rewrite the original body of
   the loop (as it should happen in complete unrolling, but not in ordinary
   peeling of the loop).  */

static void apply_opt_in_copies(struct opt_info *opt_info, unsigned n_copies,
                                bool unrolling, bool rewrite_original_loop) {
  unsigned i, delta;
  basic_block bb, orig_bb;
  rtx_insn *insn, *orig_insn, *next;
  struct iv_to_split ivts_templ, *ivts;
  struct var_to_expand ve_templ, *ves;

  /* Sanity check -- we need to put initialization in the original loop
     body.  */
  gcc_assert(!unrolling || rewrite_original_loop);

  /* Allocate the basic variables (i0).  */
  if (opt_info->insns_to_split)
    for (ivts = opt_info->iv_to_split_head; ivts; ivts = ivts->next)
      allocate_basic_variable(ivts);

  for (i = opt_info->first_new_block;
       i < (unsigned)last_basic_block_for_fn(cfun); i++) {
    bb = BASIC_BLOCK_FOR_FN(cfun, i);
    orig_bb = get_bb_original(bb);

    /* bb->aux holds position in copy sequence initialized by
   duplicate_loop_body_to_header_edge.  */
    delta = determine_split_iv_delta((size_t)bb->aux, n_copies, unrolling);
    bb->aux = 0;
    orig_insn = BB_HEAD(orig_bb);
    FOR_BB_INSNS_SAFE(bb, insn, next) {
      if (!INSN_P(insn) ||
          (DEBUG_BIND_INSN_P(insn) && INSN_VAR_LOCATION_DECL(insn) &&
           TREE_CODE(INSN_VAR_LOCATION_DECL(insn)) == LABEL_DECL))
        continue;

      while (!INSN_P(orig_insn) ||
             (DEBUG_BIND_INSN_P(orig_insn) &&
              INSN_VAR_LOCATION_DECL(orig_insn) &&
              (TREE_CODE(INSN_VAR_LOCATION_DECL(orig_insn)) == LABEL_DECL)))
        orig_insn = NEXT_INSN(orig_insn);

      ivts_templ.insn = orig_insn;
      ve_templ.insn = orig_insn;

      /* Apply splitting iv optimization.  */
      if (opt_info->insns_to_split) {
        maybe_strip_eq_note_for_split_iv(opt_info, insn);

        ivts = opt_info->insns_to_split->find(&ivts_templ);

        if (ivts) {
          gcc_assert(GET_CODE(PATTERN(insn)) == GET_CODE(PATTERN(orig_insn)));

          if (!delta) insert_base_initialization(ivts, insn);
          split_iv(ivts, insn, delta);
        }
      }
      /* Apply variable expansion optimization.  */
      if (unrolling && opt_info->insns_with_var_to_expand) {
        ves = (struct var_to_expand *)opt_info->insns_with_var_to_expand->find(
            &ve_templ);
        if (ves) {
          gcc_assert(GET_CODE(PATTERN(insn)) == GET_CODE(PATTERN(orig_insn)));
          expand_var_during_unrolling(ves, insn);
        }
      }
      orig_insn = NEXT_INSN(orig_insn);
    }
  }

  if (!rewrite_original_loop) return;

  /* Initialize the variable expansions in the loop preheader
     and take care of combining them at the loop exit.  */
  if (opt_info->insns_with_var_to_expand) {
    for (ves = opt_info->var_to_expand_head; ves; ves = ves->next)
      insert_var_expansion_initialization(ves, opt_info->loop_preheader);
    for (ves = opt_info->var_to_expand_head; ves; ves = ves->next)
      combine_var_copies_in_loop_exit(ves, opt_info->loop_exit);
  }

  /* Rewrite also the original loop body.  Find them as originals of the blocks
     in the last copied iteration, i.e. those that have
     get_bb_copy (get_bb_original (bb)) == bb.  */
  for (i = opt_info->first_new_block;
       i < (unsigned)last_basic_block_for_fn(cfun); i++) {
    bb = BASIC_BLOCK_FOR_FN(cfun, i);
    orig_bb = get_bb_original(bb);
    if (get_bb_copy(orig_bb) != bb) continue;

    delta = determine_split_iv_delta(0, n_copies, unrolling);
    for (orig_insn = BB_HEAD(orig_bb); orig_insn != NEXT_INSN(BB_END(bb));
         orig_insn = next) {
      next = NEXT_INSN(orig_insn);

      if (!INSN_P(orig_insn)) continue;

      ivts_templ.insn = orig_insn;
      if (opt_info->insns_to_split) {
        maybe_strip_eq_note_for_split_iv(opt_info, orig_insn);

        ivts =
            (struct iv_to_split *)opt_info->insns_to_split->find(&ivts_templ);
        if (ivts) {
          if (!delta) insert_base_initialization(ivts, orig_insn);
          split_iv(ivts, orig_insn, delta);
          continue;
        }
      }
    }
  }
}

/* Release OPT_INFO.  */

static void free_opt_info(struct opt_info *opt_info) {
  delete opt_info->insns_to_split;
  opt_info->insns_to_split = NULL;
  if (opt_info->insns_with_var_to_expand) {
    struct var_to_expand *ves;

    for (ves = opt_info->var_to_expand_head; ves; ves = ves->next)
      ves->var_expansions.release();
    delete opt_info->insns_with_var_to_expand;
    opt_info->insns_with_var_to_expand = NULL;
  }
  free(opt_info);
}
/* ===------------------------- Loop-Unroll-Pass -------------------------=== */

/* ===------------------------- IPA-Inline -------------------------=== */
const pass_data ipa_inline_opt_pass_data = {
    .type = IPA_PASS,  // GIMPLE_PASS, SIMPLE_IPA_PASS, RTL_PASS
    .name = "ipa_inline_opt_pass",
    .optinfo_flags = OPTGROUP_INLINE,
    .tv_id = TV_IPA_INLINING,
    .properties_required = 0,
    .properties_provided = 0,
    .properties_destroyed = 0,
    .todo_flags_start = 0,
    .todo_flags_finish = (TODO_dump_symtab)};

struct ipa_inline_opt_pass : ipa_opt_pass_d {
 public:
  ipa_inline_opt_pass()
      : ipa_opt_pass_d(ipa_inline_opt_pass_data, g, NULL, /* generate_summary */
                       NULL,                              /* write_summary */
                       NULL,                              /* read_summary */
                       NULL, /* write_optimization_summary */
                       NULL, /* read_optimization_summary */
                       NULL, /* stmt_fixup */
                       0,    /* function_transform_todo_flags_start */
                       inline_transform, /* function_transform */
                       NULL)             /* variable_transform */
  {}

  virtual unsigned int execute(function *) override { return ipa_inline(); }

  virtual ipa_inline_opt_pass *clone() override { return this; }
};

struct register_pass_info ipa_inline_opt_passinfo {
  // create `ipa_inline_opt_pass` replace `pass_ipa_inline`
  .pass = new ipa_inline_opt_pass(), .reference_pass_name = "inline",
  .ref_pass_instance_number = 0,
  .pos_op = PASS_POS_REPLACE  // PASS_POS_INSERT_AFTER, PASS_POS_REPLACE
};
/* ===------------------------- IPA-Inline -------------------------=== */

/* ===------------------------- Loop-Unroll -------------------------=== */
const pass_data rtl_unroll_opt_pass_data = {.type = RTL_PASS,
                                            .name = "rtl_unroll_opt_pass",
                                            .optinfo_flags = OPTGROUP_LOOP,
                                            .tv_id = TV_LOOP_UNROLL,
                                            .properties_required = 0,
                                            .properties_provided = 0,
                                            .properties_destroyed = 0,
                                            .todo_flags_start = 0,
                                            .todo_flags_finish = 0};

struct rtl_unroll_opt_pass : rtl_opt_pass {
 public:
  rtl_unroll_opt_pass() : rtl_opt_pass(rtl_unroll_opt_pass_data, g) {}

  virtual bool gate(function *) {
    return (flag_unroll_loops || flag_unroll_all_loops || cfun->has_unroll);
  }

  virtual unsigned int execute(function *fun) override;

  virtual rtl_unroll_opt_pass *clone() override { return this; }
};

unsigned int rtl_unroll_opt_pass::execute(function *fun) {
  if (number_of_loops(fun) > 1) {
    int flags = 0;
    if (dump_file) df_dump(dump_file);

    if (flag_unroll_loops) flags |= UAP_UNROLL;
    if (flag_unroll_all_loops) flags |= UAP_UNROLL_ALL;

    unroll_loops_(flags);
  }
  return 0;
}

struct register_pass_info rtl_unroll_opt_passinfo {
  .pass = new rtl_unroll_opt_pass(), .reference_pass_name = "loop2_unroll",
  .ref_pass_instance_number = 0, .pos_op = PASS_POS_REPLACE
};
/* ===------------------------- Loop-Unroll -------------------------=== */

int plugin_init(struct plugin_name_args *plugin_info,
                struct plugin_gcc_version *version) {
  bool inline_switch = false;
  bool unroll_switch = false;

  const char *native_tune_clear = get_optimize_decision();
  execute_sha256(native_tune_clear, hash, sizeof(hash));
  var_str[0] = hash;

  if (!plugin_default_version_check(version, &gcc_version)) {
    printf("incompatible gcc/plugin versions\n");
    return 1;
  }

  for (int i = 0; i < plugin_info->argc; i++) {
    std::string key(plugin_info->argv[i].key);
    std::string value(plugin_info->argv[i].value);
    if (std::strcmp(plugin_info->argv[i].key, "inline_model") == 0) {
      g_inline_model_path = plugin_info->argv[i].value;
      inline_switch = true;
    } else if (std::strcmp(plugin_info->argv[i].key, "unroll_model") == 0) {
      g_unroll_model_path = plugin_info->argv[i].value;
      unroll_switch = true;
    } else if (std::strcmp(plugin_info->argv[i].key, "engine") == 0) {
      g_infer_path = plugin_info->argv[i].value;
    }
  }

  if (access(g_inline_model_path, F_OK) && inline_switch) {
    fprintf(stderr, "Model '%s' not found\n", g_inline_model_path);
    return -1;
  }

  if (access(g_unroll_model_path, F_OK) && unroll_switch) {
    fprintf(stderr, "Model '%s' not found\n", g_unroll_model_path);
    return -1;
  }

  g_infer_handle = dlopen(g_infer_path, RTLD_LAZY);
  if (!g_infer_handle) {
    fprintf(stderr, "%s\n", dlerror());
    return -1;
  }

  initialize = (init_engine_t)dlsym(g_infer_handle, "initialize");
  add_double_input =
      (add_double_input_t)dlsym(g_infer_handle, "add_double_input");
  add_int64_input = (add_int64_input_t)dlsym(g_infer_handle, "add_int64_input");
  add_float_input = (add_float_input_t)dlsym(g_infer_handle, "add_float_input");
  add_string_input =
      (add_string_input_t)dlsym(g_infer_handle, "add_string_input");
  inference = (inference_t)dlsym(g_infer_handle, "inference");
  get_int64_output =
      (get_int64_output_t)dlsym(g_infer_handle, "get_int64_output");
  free_engine = (free_engine_t)dlsym(g_infer_handle, "free_engine");

  dlclose(g_infer_handle);

  const char *const plugin_name = plugin_info->base_name;

  if (inline_switch) {
    register_callback(plugin_name, PLUGIN_PASS_MANAGER_SETUP, NULL,
                      &ipa_inline_opt_passinfo);
  }

  if (unroll_switch) {
    register_callback(plugin_name, PLUGIN_PASS_MANAGER_SETUP, NULL,
                      &rtl_unroll_opt_passinfo);
  }

  return 0;
}
