#ifndef AI4C_OPTION_UTILS_H
#define AI4C_OPTION_UTILS_H

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "coretypes.h"

namespace ai4c {

#define STRINGIFY(s) #s

#define UPDATE_OPTION_VALUE(OPTS, OPTS_SET, OPTION, VALUE) \
  do {                                                     \
    (OPTS)->x##OPTION = VALUE;                             \
  } while (false)

// Remove options that starts with "optimize", "debug_", and "str_".
std::vector<std::string> int_opt_indices{
    "param_align_loop_iterations",
    "param_align_threshold",
    "param_asan_protect_allocas",
    "param_asan_instrument_reads",
    "param_asan_instrument_writes",
    "param_asan_instrumentation_with_call_threshold",
    "param_asan_memintrin",
    "param_asan_stack",
    "param_asan_use_after_return",
    "param_avg_loop_niter",
    "param_branch_prob_threshold",
    "param_builtin_expect_probability",
    "param_builtin_string_cmp_inline_length",
    "param_comdat_sharing_probability",
    "param_pointer_compression_size",
    "param_construct_interfere_size",
    "param_destruct_interfere_size",
    "param_dse_max_alias_queries_per_store",
    "param_dse_max_object_size",
    "param_early_inlining_insns",
    "param_evrp_sparse_threshold",
    "param_evrp_switch_limit",
    "param_fsm_scale_path_blocks",
    "param_fsm_scale_path_stmts",
    "param_gcse_after_reload_critical_fraction",
    "param_gcse_after_reload_partial_fraction",
    "param_gcse_cost_distance_ratio",
    "param_gcse_unrestricted_cost",
    "param_graphite_max_arrays_per_scop",
    "param_graphite_max_nb_scop_params",
    "param_hwasan_instrument_allocas",
    "param_hwasan_instrument_mem_intrinsics",
    "param_hwasan_instrument_reads",
    "param_hwasan_instrument_stack",
    "param_hwasan_instrument_writes",
    "param_hwasan_random_frame_tag",
    "param_inline_heuristics_hint_percent",
    "param_inline_min_speedup",
    "param_inline_unit_growth",
    "param_ipa_cp_eval_threshold",
    "param_ipa_cp_large_unit_insns",
    "param_ipa_cp_loop_hint_bonus",
    "param_ipa_cp_max_recursive_depth",
    "param_ipa_cp_min_recursive_probability",
    "param_ipa_cp_profile_count_base",
    "param_ipa_cp_recursion_penalty",
    "param_ipa_cp_recursive_freq_factor",
    "param_ipa_cp_single_call_penalty",
    "param_ipa_cp_unit_growth",
    "param_ipa_cp_value_list_size",
    "param_ipa_jump_function_lookups",
    "param_ipa_max_aa_steps",
    "param_ipa_max_agg_items",
    "param_ipa_max_loop_predicates",
    "param_ipa_max_param_expr_ops",
    "param_ipa_max_switch_predicate_bounds",
    "param_ipa_prefetch_distance_factor",
    "param_ipa_prefetch_locality",
    "param_ipa_prefetch_pagesize",
    "param_ipa_sra_max_replacements",
    "param_ipa_sra_ptr_growth_factor",
    "param_ira_consider_dup_in_all_alts",
    "param_ira_loop_reserved_regs",
    "param_ira_max_conflict_table_size",
    "param_ira_max_loops_num",
    "param_issue_topn",
    "param_iv_always_prune_cand_set_bound",
    "param_iv_consider_all_candidates_bound",
    "param_iv_max_considered_uses",
    "param_jump_table_max_growth_ratio_for_size",
    "param_jump_table_max_growth_ratio_for_speed",
    "param_l1_cache_line_size",
    "param_l1_cache_size",
    "param_l2_cache_size",
    "param_large_function_growth",
    "param_large_function_insns",
    "param_stack_frame_growth",
    "param_large_stack_frame",
    "param_large_unit_insns",
    "param_lim_expensive",
    "param_llc_allocate_func_counts_threshold",
    "param_loop_block_tile_size",
    "param_loop_interchange_max_num_stmts",
    "param_loop_interchange_stride_ratio",
    "param_loop_invariant_max_bbs_in_loop",
    "param_loop_max_datarefs_for_datadeps",
    "param_loop_versioning_max_inner_insns",
    "param_loop_versioning_max_outer_insns",
    "param_lra_inheritance_ebb_probability_cutoff",
    "param_lra_max_considered_reload_pseudos",
    "param_max_average_unrolled_insns",
    "param_max_combine_insns",
    "param_max_unroll_iterations",
    "param_max_completely_peel_times",
    "param_max_completely_peeled_insns",
    "param_max_crossjump_edges",
    "param_max_cse_insns",
    "param_max_cse_path_length",
    "param_max_cselib_memory_locations",
    "param_max_debug_marker_count",
    "param_max_delay_slot_insn_search",
    "param_max_delay_slot_live_search",
    "param_max_dse_active_local_stores",
    "param_early_inliner_max_iterations",
    "param_max_find_base_term_values",
    "param_max_fsm_thread_length",
    "param_max_fsm_thread_path_insns",
    "param_max_gcse_insertion_ratio",
    "param_max_gcse_memory",
    "param_max_goto_duplication_insns",
    "param_max_grow_copy_bb_insns",
    "param_max_hoist_depth",
    "param_inline_functions_called_once_insns",
    "param_inline_functions_called_once_loop_depth",
    "param_max_inline_insns_auto",
    "param_max_inline_insns_recursive_auto",
    "param_max_inline_insns_recursive",
    "param_max_inline_insns_single",
    "param_max_inline_recursive_depth_auto",
    "param_max_inline_recursive_depth",
    "param_max_isl_operations",
    "param_max_iterations_computation_cost",
    "param_max_iterations_to_track",
    "param_max_jump_thread_duplication_stmts",
    "param_max_last_value_rtl",
    "param_max_loop_header_insns",
    "param_max_modulo_backtrack_attempts",
    "param_max_partial_antic_length",
    "param_max_peel_branches",
    "param_max_peel_times",
    "param_max_peeled_insns",
    "param_max_pending_list_length",
    "param_max_pipeline_region_blocks",
    "param_max_pipeline_region_insns",
    "param_max_pow_sqrt_depth",
    "param_max_predicted_iterations",
    "param_max_reload_search_insns",
    "param_max_rtl_if_conversion_insns",
    "param_max_rtl_if_conversion_predictable_cost",
    "param_max_rtl_if_conversion_unpredictable_cost",
    "param_max_sched_insn_conflict_delay",
    "param_max_sched_ready_insns",
    "param_max_sched_region_blocks",
    "param_max_sched_region_insns",
    "param_max_slsr_candidate_scan",
    "param_max_speculative_devirt_maydefs",
    "param_max_stores_to_merge",
    "param_max_stores_to_sink",
    "param_max_tail_merge_comparisons",
    "param_max_tail_merge_iterations",
    "param_max_tracked_strlens",
    "param_max_tree_if_conversion_phi_args",
    "param_max_unroll_times",
    "param_max_unrolled_insns",
    "param_max_unswitch_insns",
    "param_max_unswitch_level",
    "param_max_variable_expansions",
    "param_max_vartrack_expr_depth",
    "param_max_vartrack_reverse_op_size",
    "param_max_vartrack_size",
    "param_max_vrp_switch_assertions",
    "param_mem_access_num",
    "param_mem_access_ratio",
    "param_min_crossjump_insns",
    "param_min_inline_recursive_probability",
    "param_min_insn_to_prefetch_ratio",
    "param_min_loop_cond_split_prob",
    "param_min_pagesize",
    "param_min_size_for_stack_sharing",
    "param_min_spec_prob",
    "param_modref_max_accesses",
    "param_modref_max_adjustments",
    "param_modref_max_bases",
    "param_modref_max_depth",
    "param_modref_max_escape_points",
    "param_modref_max_refs",
    "param_modref_max_tests",
    "param_ldp_dependency_search_range",
    "param_parloops_min_per_thread",
    "param_partial_inlining_entry_probability",
    "param_predictable_branch_outcome",
    "param_prefetch_dynamic_strides",
    "param_prefetch_latency",
    "param_prefetch_min_insn_to_mem_ratio",
    "param_prefetch_minimum_stride",
    "param_prefetch_offset",
    "param_ranger_logical_depth",
    "param_relation_block_limit",
    "param_rpo_vn_max_loop_depth",
    "param_sccvn_max_alias_queries_per_access",
    "param_scev_max_expr_complexity",
    "param_scev_max_expr_size",
    "param_sched_mem_true_dep_cost",
    "param_sched_pressure_algorithm",
    "param_sched_spec_prob_cutoff",
    "param_sched_state_edge_prob_cutoff",
    "param_selsched_insns_to_rename",
    "param_selsched_max_lookahead",
    "param_selsched_max_sched_times",
    "semi_relayout_level",
    "param_simultaneous_prefetches",
    "param_sink_frequency_threshold",
    "param_sms_max_ii_factor",
    "param_sms_min_sc",
    "param_sra_max_propagations",
    "param_ssa_name_def_chain_limit",
    "param_ssp_buffer_size",
    "param_stack_clash_protection_guard_size",
    "param_stack_clash_protection_probe_interval",
    "param_store_merging_allow_unaligned",
    "param_store_merging_max_size",
    "param_struct_reorg_cold_struct_ratio",
    "param_switch_conversion_branch_ratio",
    "param_tm_max_aggregate_size",
    "param_tracer_dynamic_coverage_feedback",
    "param_tracer_dynamic_coverage",
    "param_tracer_max_code_growth",
    "param_tracer_min_branch_probability_feedback",
    "param_tracer_min_branch_probability",
    "param_tracer_min_branch_ratio",
    "param_uninit_control_dep_attempts",
    "param_uninlined_function_insns",
    "param_uninlined_function_thunk_insns",
    "param_uninlined_function_thunk_time",
    "param_unlikely_bb_count_fraction",
    "param_unroll_jam_max_unroll",
    "param_unroll_jam_min_percent",
    "param_use_after_scope_direct_emission_threshold",
    "param_vect_epilogues_nomask",
    "param_vect_induction_float",
    "param_vect_inner_loop_cost_factor",
    "param_vect_max_peeling_for_alignment",
    "param_vect_max_version_for_alias_checks",
    "param_vect_max_version_for_alignment_checks",
    "param_vect_partial_vector_usage",
    "flag_sched_stalled_insns_dep",
    "flag_tree_parallelize_loops",
    "flag_aggressive_loop_optimizations",
    "flag_align_functions",
    "flag_align_jumps",
    "flag_align_labels",
    "flag_align_loops",
    "flag_allocation_dce",
    "flag_asynchronous_unwind_tables",
    "flag_auto_inc_dec",
    "flag_bit_tests",
    "flag_branch_on_count_reg",
    "flag_caller_saves",
    "flag_code_hoisting",
    "flag_combine_stack_adjustments",
    "flag_compare_elim_after_reload",
    "flag_cprop_registers",
    "flag_crossjumping",
    "flag_cse_follow_jumps",
    "flag_dce",
    "flag_defer_pop",
    "flag_delete_dead_exceptions",
    "flag_delete_null_pointer_checks",
    "flag_devirtualize",
    "flag_devirtualize_speculatively",
    "flag_dse",
    "flag_early_inlining",
    "flag_expensive_optimizations",
    "flag_forward_propagate",
    "flag_fp_int_builtin_inexact",
    "flag_gcse",
    "flag_gcse_lm",
    "flag_guess_branch_prob",
    "flag_hoist_adjacent_loads",
    "flag_if_conversion",
    "flag_if_conversion2",
    "flag_indirect_inlining",
    "flag_inline_atomics",
    "flag_inline_functions",
    "flag_inline_functions_called_once",
    "flag_inline_small_functions",
    "flag_ipa_bit_cp",
    "flag_ipa_cp",
    "flag_ipa_icf",
    "flag_ipa_icf_functions",
    "flag_ipa_icf_variables",
    "flag_ipa_modref",
    "flag_ipa_profile",
    "flag_ipa_pure_const",
    "flag_ipa_ra",
    "flag_ipa_reference",
    "flag_ipa_reference_addressable",
    "flag_ipa_sra",
    "flag_ipa_stack_alignment",
    "flag_ipa_strict_aliasing",
    "flag_ipa_vrp",
    "flag_ira_hoist_pressure",
    "flag_ira_share_save_slots",
    "flag_ira_share_spill_slots",
    "flag_isolate_erroneous_paths_dereference",
    "flag_ivopts",
    "flag_jump_tables",
    "flag_lifetime_dse",
    "flag_lra_remat",
    "flag_errno_math",
    "flag_move_loop_invariants",
    "flag_move_loop_stores",
    "flag_omit_frame_pointer",
    "flag_optimize_sibling_calls",
    "flag_optimize_strlen",
    "flag_partial_inlining",
    "flag_peephole2",
    "flag_plt",
    "flag_prefetch_loop_arrays",
    "flag_printf_return_value",
    "flag_ree",
    "flag_rename_registers",
    "flag_reorder_blocks",
    "flag_reorder_functions",
    "flag_rerun_cse_after_loop",
    "flag_rtti",
    "flag_sched_critical_path_heuristic",
    "flag_sched_dep_count_heuristic",
    "flag_sched_group_heuristic",
    "flag_schedule_interblock",
    "flag_sched_last_insn_heuristic",
    "flag_sched_pressure",
    "flag_sched_rank_heuristic",
    "flag_schedule_speculative",
    "flag_sched_spec_insn_heuristic",
    "flag_schedule_fusion",
    "flag_schedule_insns",
    "flag_schedule_insns_after_reload",
    "flag_section_anchors",
    "flag_semantic_interposition",
    "flag_shrink_wrap",
    "flag_shrink_wrap_separate",
    "flag_signed_zeros",
    "flag_split_ivs_in_unroller",
    "flag_split_wide_types",
    "flag_ssa_backprop",
    "flag_ssa_phiopt",
    "flag_stdarg_opt",
    "flag_store_merging",
    "flag_strict_aliasing",
    "flag_strict_volatile_bitfields",
    "flag_thread_jumps",
    "flag_threadsafe_statics",
    "flag_toplevel_reorder",
    "flag_trapping_math",
    "flag_tree_bit_ccp",
    "flag_tree_builtin_call_dce",
    "flag_tree_ccp",
    "flag_tree_ch",
    "flag_tree_coalesce_vars",
    "flag_tree_copy_prop",
    "flag_tree_cselim",
    "flag_tree_dce",
    "flag_tree_dom",
    "flag_tree_dse",
    "flag_tree_forwprop",
    "flag_tree_fre",
    "flag_tree_loop_distribute_patterns",
    "flag_tree_loop_if_convert",
    "flag_tree_loop_im",
    "flag_tree_loop_ivcanon",
    "flag_tree_loop_optimize",
    "flag_tree_loop_vectorize",
    "flag_tree_phiprop",
    "flag_tree_pre",
    "flag_tree_pta",
    "flag_tree_reassoc",
    "flag_tree_scev_cprop",
    "flag_tree_sink",
    "flag_tree_slp_vectorize",
    "flag_tree_slsr",
    "flag_tree_sra",
    "flag_tree_switch_conversion",
    "flag_tree_tail_merge",
    "flag_tree_ter",
    "flag_tree_vrp",
    "flag_cunroll_grow_size",
    "flag_unroll_loops",
    "flag_unwind_tables",
    "flag_var_tracking",
    "flag_var_tracking_assignments",
    "flag_var_tracking_uninit",
    "flag_web",
};

std::vector<std::string> enum_opt_indices{
    "param_evrp_mode",
    "param_evrp_mode",
    "param_ranger_debug",
    "param_threader_debug",
    "param_vrp1_mode",
    "param_vrp2_mode",
    "flag_excess_precision",
    "flag_fp_contract_mode",
    "flag_ira_algorithm",
    "flag_ira_region",
    "flag_live_patching",
    "flag_fp_model",
    "flag_reorder_blocks_algorithm",
    "flag_simd_cost_model",
    "flag_stack_reuse",
    "flag_auto_var_init",
    "flag_vect_cost_model",
};

void check_option(int opt_idx, struct gcc_options *opts,
                  struct gcc_options *opts_set, std::string option, int value) {
#define UPDATE_OPTION(OPTION) UPDATE_OPTION_VALUE(opts, opts_set, OPTION, value)

  switch (opt_idx) {
    case 0:
      UPDATE_OPTION(_param_align_loop_iterations);
      break;
    case 1:
      UPDATE_OPTION(_param_align_threshold);
      break;
    case 2:
      UPDATE_OPTION(_param_asan_protect_allocas);
      break;
    case 3:
      UPDATE_OPTION(_param_asan_instrument_reads);
      break;
    case 4:
      UPDATE_OPTION(_param_asan_instrument_writes);
      break;
    case 5:
      UPDATE_OPTION(_param_asan_instrumentation_with_call_threshold);
      break;
    case 6:
      UPDATE_OPTION(_param_asan_memintrin);
      break;
    case 7:
      UPDATE_OPTION(_param_asan_stack);
      break;
    case 8:
      UPDATE_OPTION(_param_asan_use_after_return);
      break;
    case 9:
      UPDATE_OPTION(_param_avg_loop_niter);
      break;
    // case 10:
    //     UPDATE_OPTION(_param_branch_prob_threshold);
    //     break;
    case 11:
      UPDATE_OPTION(_param_builtin_expect_probability);
      break;
    case 12:
      UPDATE_OPTION(_param_builtin_string_cmp_inline_length);
      break;
    case 13:
      UPDATE_OPTION(_param_comdat_sharing_probability);
      break;
    case 14:
      UPDATE_OPTION(_param_pointer_compression_size);
      break;
    case 15:
      UPDATE_OPTION(_param_construct_interfere_size);
      break;
    case 16:
      UPDATE_OPTION(_param_destruct_interfere_size);
      break;
    case 17:
      UPDATE_OPTION(_param_dse_max_alias_queries_per_store);
      break;
    case 18:
      UPDATE_OPTION(_param_dse_max_object_size);
      break;
    case 19:
      UPDATE_OPTION(_param_early_inlining_insns);
      break;
    case 20:
      UPDATE_OPTION(_param_evrp_sparse_threshold);
      break;
    case 21:
      UPDATE_OPTION(_param_evrp_switch_limit);
      break;
    case 22:
      UPDATE_OPTION(_param_fsm_scale_path_blocks);
      break;
    case 23:
      UPDATE_OPTION(_param_fsm_scale_path_stmts);
      break;
    case 24:
      UPDATE_OPTION(_param_gcse_after_reload_critical_fraction);
      break;
    case 25:
      UPDATE_OPTION(_param_gcse_after_reload_partial_fraction);
      break;
    case 26:
      UPDATE_OPTION(_param_gcse_cost_distance_ratio);
      break;
    case 27:
      UPDATE_OPTION(_param_gcse_unrestricted_cost);
      break;
    case 28:
      UPDATE_OPTION(_param_graphite_max_arrays_per_scop);
      break;
    case 29:
      UPDATE_OPTION(_param_graphite_max_nb_scop_params);
      break;
    case 30:
      UPDATE_OPTION(_param_hwasan_instrument_allocas);
      break;
    case 31:
      UPDATE_OPTION(_param_hwasan_instrument_mem_intrinsics);
      break;
    case 32:
      UPDATE_OPTION(_param_hwasan_instrument_reads);
      break;
    case 33:
      UPDATE_OPTION(_param_hwasan_instrument_stack);
      break;
    case 34:
      UPDATE_OPTION(_param_hwasan_instrument_writes);
      break;
    case 35:
      UPDATE_OPTION(_param_hwasan_random_frame_tag);
      break;
    case 36:
      UPDATE_OPTION(_param_inline_heuristics_hint_percent);
      break;
    case 37:
      UPDATE_OPTION(_param_inline_min_speedup);
      break;
    case 38:
      UPDATE_OPTION(_param_inline_unit_growth);
      break;
    case 39:
      UPDATE_OPTION(_param_ipa_cp_eval_threshold);
      break;
    case 40:
      UPDATE_OPTION(_param_ipa_cp_large_unit_insns);
      break;
    case 41:
      UPDATE_OPTION(_param_ipa_cp_loop_hint_bonus);
      break;
    case 42:
      UPDATE_OPTION(_param_ipa_cp_max_recursive_depth);
      break;
    case 43:
      UPDATE_OPTION(_param_ipa_cp_min_recursive_probability);
      break;
    case 44:
      UPDATE_OPTION(_param_ipa_cp_profile_count_base);
      break;
    case 45:
      UPDATE_OPTION(_param_ipa_cp_recursion_penalty);
      break;
    case 46:
      UPDATE_OPTION(_param_ipa_cp_recursive_freq_factor);
      break;
    case 47:
      UPDATE_OPTION(_param_ipa_cp_single_call_penalty);
      break;
    case 48:
      UPDATE_OPTION(_param_ipa_cp_unit_growth);
      break;
    case 49:
      UPDATE_OPTION(_param_ipa_cp_value_list_size);
      break;
    case 50:
      UPDATE_OPTION(_param_ipa_jump_function_lookups);
      break;
    case 51:
      UPDATE_OPTION(_param_ipa_max_aa_steps);
      break;
    case 52:
      UPDATE_OPTION(_param_ipa_max_agg_items);
      break;
    case 53:
      UPDATE_OPTION(_param_ipa_max_loop_predicates);
      break;
    case 54:
      UPDATE_OPTION(_param_ipa_max_param_expr_ops);
      break;
    case 55:
      UPDATE_OPTION(_param_ipa_max_switch_predicate_bounds);
      break;
    case 56:
      UPDATE_OPTION(_param_ipa_prefetch_distance_factor);
      break;
    case 57:
      UPDATE_OPTION(_param_ipa_prefetch_locality);
      break;
    // case 58:
    //  UPDATE_OPTION(_param_ipa_prefetch_pagesize);
    //  break;
    case 59:
      UPDATE_OPTION(_param_ipa_sra_max_replacements);
      break;
    case 60:
      UPDATE_OPTION(_param_ipa_sra_ptr_growth_factor);
      break;
    case 61:
      UPDATE_OPTION(_param_ira_consider_dup_in_all_alts);
      break;
    case 62:
      UPDATE_OPTION(_param_ira_loop_reserved_regs);
      break;
    case 63:
      UPDATE_OPTION(_param_ira_max_conflict_table_size);
      break;
    case 64:
      UPDATE_OPTION(_param_ira_max_loops_num);
      break;
    // case 65:
    //     UPDATE_OPTION(_param_issue_topn);
    //     break;
    case 66:
      UPDATE_OPTION(_param_iv_always_prune_cand_set_bound);
      break;
    case 67:
      UPDATE_OPTION(_param_iv_consider_all_candidates_bound);
      break;
    case 68:
      UPDATE_OPTION(_param_iv_max_considered_uses);
      break;
    case 69:
      UPDATE_OPTION(_param_jump_table_max_growth_ratio_for_size);
      break;
    case 70:
      UPDATE_OPTION(_param_jump_table_max_growth_ratio_for_speed);
      break;
    case 71:
      UPDATE_OPTION(_param_l1_cache_line_size);
      break;
    case 72:
      UPDATE_OPTION(_param_l1_cache_size);
      break;
    case 73:
      UPDATE_OPTION(_param_l2_cache_size);
      break;
    case 74:
      UPDATE_OPTION(_param_large_function_growth);
      break;
    case 75:
      UPDATE_OPTION(_param_large_function_insns);
      break;
    case 76:
      UPDATE_OPTION(_param_stack_frame_growth);
      break;
    case 77:
      UPDATE_OPTION(_param_large_stack_frame);
      break;
    case 78:
      UPDATE_OPTION(_param_large_unit_insns);
      break;
    case 79:
      UPDATE_OPTION(_param_lim_expensive);
      break;
    // case 80:
    //     UPDATE_OPTION(_param_llc_allocate_func_counts_threshold);
    //     break;
    case 81:
      UPDATE_OPTION(_param_loop_block_tile_size);
      break;
    case 82:
      UPDATE_OPTION(_param_loop_interchange_max_num_stmts);
      break;
    case 83:
      UPDATE_OPTION(_param_loop_interchange_stride_ratio);
      break;
    case 84:
      UPDATE_OPTION(_param_loop_invariant_max_bbs_in_loop);
      break;
    case 85:
      UPDATE_OPTION(_param_loop_max_datarefs_for_datadeps);
      break;
    case 86:
      UPDATE_OPTION(_param_loop_versioning_max_inner_insns);
      break;
    case 87:
      UPDATE_OPTION(_param_loop_versioning_max_outer_insns);
      break;
    case 88:
      UPDATE_OPTION(_param_lra_inheritance_ebb_probability_cutoff);
      break;
    case 89:
      UPDATE_OPTION(_param_lra_max_considered_reload_pseudos);
      break;
    case 90:
      UPDATE_OPTION(_param_max_average_unrolled_insns);
      break;
    case 91:
      UPDATE_OPTION(_param_max_combine_insns);
      break;
    case 92:
      UPDATE_OPTION(_param_max_unroll_iterations);
      break;
    case 93:
      UPDATE_OPTION(_param_max_completely_peel_times);
      break;
    case 94:
      UPDATE_OPTION(_param_max_completely_peeled_insns);
      break;
    case 95:
      UPDATE_OPTION(_param_max_crossjump_edges);
      break;
    case 96:
      UPDATE_OPTION(_param_max_cse_insns);
      break;
    case 97:
      UPDATE_OPTION(_param_max_cse_path_length);
      break;
    case 98:
      UPDATE_OPTION(_param_max_cselib_memory_locations);
      break;
    case 99:
      UPDATE_OPTION(_param_max_debug_marker_count);
      break;
    case 100:
      UPDATE_OPTION(_param_max_delay_slot_insn_search);
      break;
    case 101:
      UPDATE_OPTION(_param_max_delay_slot_live_search);
      break;
    case 102:
      UPDATE_OPTION(_param_max_dse_active_local_stores);
      break;
    case 103:
      UPDATE_OPTION(_param_early_inliner_max_iterations);
      break;
    case 104:
      UPDATE_OPTION(_param_max_find_base_term_values);
      break;
    case 105:
      UPDATE_OPTION(_param_max_fsm_thread_length);
      break;
    case 106:
      UPDATE_OPTION(_param_max_fsm_thread_path_insns);
      break;
    case 107:
      UPDATE_OPTION(_param_max_gcse_insertion_ratio);
      break;
    case 108:
      UPDATE_OPTION(_param_max_gcse_memory);
      break;
    case 109:
      UPDATE_OPTION(_param_max_goto_duplication_insns);
      break;
    case 110:
      UPDATE_OPTION(_param_max_grow_copy_bb_insns);
      break;
    case 111:
      UPDATE_OPTION(_param_max_hoist_depth);
      break;
    case 112:
      UPDATE_OPTION(_param_inline_functions_called_once_insns);
      break;
    case 113:
      UPDATE_OPTION(_param_inline_functions_called_once_loop_depth);
      break;
    case 114:
      UPDATE_OPTION(_param_max_inline_insns_auto);
      break;
    case 115:
      UPDATE_OPTION(_param_max_inline_insns_recursive_auto);
      break;
    case 116:
      UPDATE_OPTION(_param_max_inline_insns_recursive);
      break;
    case 117:
      UPDATE_OPTION(_param_max_inline_insns_single);
      break;
    case 118:
      UPDATE_OPTION(_param_max_inline_recursive_depth_auto);
      break;
    case 119:
      UPDATE_OPTION(_param_max_inline_recursive_depth);
      break;
    case 120:
      UPDATE_OPTION(_param_max_isl_operations);
      break;
    case 121:
      UPDATE_OPTION(_param_max_iterations_computation_cost);
      break;
    case 122:
      UPDATE_OPTION(_param_max_iterations_to_track);
      break;
    case 123:
      UPDATE_OPTION(_param_max_jump_thread_duplication_stmts);
      break;
    case 124:
      UPDATE_OPTION(_param_max_last_value_rtl);
      break;
    case 125:
      UPDATE_OPTION(_param_max_loop_header_insns);
      break;
    case 126:
      UPDATE_OPTION(_param_max_modulo_backtrack_attempts);
      break;
    case 127:
      UPDATE_OPTION(_param_max_partial_antic_length);
      break;
    case 128:
      UPDATE_OPTION(_param_max_peel_branches);
      break;
    case 129:
      UPDATE_OPTION(_param_max_peel_times);
      break;
    case 130:
      UPDATE_OPTION(_param_max_peeled_insns);
      break;
    case 131:
      UPDATE_OPTION(_param_max_pending_list_length);
      break;
    case 132:
      UPDATE_OPTION(_param_max_pipeline_region_blocks);
      break;
    case 133:
      UPDATE_OPTION(_param_max_pipeline_region_insns);
      break;
    case 134:
      UPDATE_OPTION(_param_max_pow_sqrt_depth);
      break;
    case 135:
      UPDATE_OPTION(_param_max_predicted_iterations);
      break;
    case 136:
      UPDATE_OPTION(_param_max_reload_search_insns);
      break;
    case 137:
      UPDATE_OPTION(_param_max_rtl_if_conversion_insns);
      break;
    case 138:
      UPDATE_OPTION(_param_max_rtl_if_conversion_predictable_cost);
      break;
    case 139:
      UPDATE_OPTION(_param_max_rtl_if_conversion_unpredictable_cost);
      break;
    case 140:
      UPDATE_OPTION(_param_max_sched_insn_conflict_delay);
      break;
    case 141:
      UPDATE_OPTION(_param_max_sched_ready_insns);
      break;
    case 142:
      UPDATE_OPTION(_param_max_sched_region_blocks);
      break;
    case 143:
      UPDATE_OPTION(_param_max_sched_region_insns);
      break;
    case 144:
      UPDATE_OPTION(_param_max_slsr_candidate_scan);
      break;
    case 145:
      UPDATE_OPTION(_param_max_speculative_devirt_maydefs);
      break;
    case 146:
      UPDATE_OPTION(_param_max_stores_to_merge);
      break;
    case 147:
      UPDATE_OPTION(_param_max_stores_to_sink);
      break;
    case 148:
      UPDATE_OPTION(_param_max_tail_merge_comparisons);
      break;
    case 149:
      UPDATE_OPTION(_param_max_tail_merge_iterations);
      break;
    case 150:
      UPDATE_OPTION(_param_max_tracked_strlens);
      break;
    case 151:
      UPDATE_OPTION(_param_max_tree_if_conversion_phi_args);
      break;
    case 152:
      UPDATE_OPTION(_param_max_unroll_times);
      break;
    case 153:
      UPDATE_OPTION(_param_max_unrolled_insns);
      break;
    case 154:
      UPDATE_OPTION(_param_max_unswitch_insns);
      break;
    case 155:
      UPDATE_OPTION(_param_max_unswitch_level);
      break;
    case 156:
      UPDATE_OPTION(_param_max_variable_expansions);
      break;
    case 157:
      UPDATE_OPTION(_param_max_vartrack_expr_depth);
      break;
    case 158:
      UPDATE_OPTION(_param_max_vartrack_reverse_op_size);
      break;
    case 159:
      UPDATE_OPTION(_param_max_vartrack_size);
      break;
    case 160:
      UPDATE_OPTION(_param_max_vrp_switch_assertions);
      break;
    // case 161:
    //     UPDATE_OPTION(_param_mem_access_num);
    //     break;
    // case 162:
    //     UPDATE_OPTION(_param_mem_access_ratio);
    //     break;
    case 163:
      UPDATE_OPTION(_param_min_crossjump_insns);
      break;
    case 164:
      UPDATE_OPTION(_param_min_inline_recursive_probability);
      break;
    case 165:
      UPDATE_OPTION(_param_min_insn_to_prefetch_ratio);
      break;
    case 166:
      UPDATE_OPTION(_param_min_loop_cond_split_prob);
      break;
    case 167:
      UPDATE_OPTION(_param_min_pagesize);
      break;
    case 168:
      UPDATE_OPTION(_param_min_size_for_stack_sharing);
      break;
    case 169:
      UPDATE_OPTION(_param_min_spec_prob);
      break;
    case 170:
      UPDATE_OPTION(_param_modref_max_accesses);
      break;
    case 171:
      UPDATE_OPTION(_param_modref_max_adjustments);
      break;
    case 172:
      UPDATE_OPTION(_param_modref_max_bases);
      break;
    case 173:
      UPDATE_OPTION(_param_modref_max_depth);
      break;
    case 174:
      UPDATE_OPTION(_param_modref_max_escape_points);
      break;
    case 175:
      UPDATE_OPTION(_param_modref_max_refs);
      break;
    case 176:
      UPDATE_OPTION(_param_modref_max_tests);
      break;
    case 177:
      UPDATE_OPTION(_param_ldp_dependency_search_range);
      break;
    case 178:
      UPDATE_OPTION(_param_parloops_min_per_thread);
      break;
    case 179:
      UPDATE_OPTION(_param_partial_inlining_entry_probability);
      break;
    case 180:
      UPDATE_OPTION(_param_predictable_branch_outcome);
      break;
    case 181:
      UPDATE_OPTION(_param_prefetch_dynamic_strides);
      break;
    case 182:
      UPDATE_OPTION(_param_prefetch_latency);
      break;
    case 183:
      UPDATE_OPTION(_param_prefetch_min_insn_to_mem_ratio);
      break;
    case 184:
      UPDATE_OPTION(_param_prefetch_minimum_stride);
      break;
    // case 185:
    //     UPDATE_OPTION(_param_prefetch_offset);
    //     break;
    case 186:
      UPDATE_OPTION(_param_ranger_logical_depth);
      break;
    case 187:
      UPDATE_OPTION(_param_relation_block_limit);
      break;
    case 188:
      UPDATE_OPTION(_param_rpo_vn_max_loop_depth);
      break;
    case 189:
      UPDATE_OPTION(_param_sccvn_max_alias_queries_per_access);
      break;
    case 190:
      UPDATE_OPTION(_param_scev_max_expr_complexity);
      break;
    case 191:
      UPDATE_OPTION(_param_scev_max_expr_size);
      break;
    case 192:
      UPDATE_OPTION(_param_sched_mem_true_dep_cost);
      break;
    case 193:
      UPDATE_OPTION(_param_sched_pressure_algorithm);
      break;
    case 194:
      UPDATE_OPTION(_param_sched_spec_prob_cutoff);
      break;
    case 195:
      UPDATE_OPTION(_param_sched_state_edge_prob_cutoff);
      break;
    case 196:
      UPDATE_OPTION(_param_selsched_insns_to_rename);
      break;
    case 197:
      UPDATE_OPTION(_param_selsched_max_lookahead);
      break;
    case 198:
      UPDATE_OPTION(_param_selsched_max_sched_times);
      break;
    case 199:
      UPDATE_OPTION(_semi_relayout_level);
      break;
    case 200:
      UPDATE_OPTION(_param_simultaneous_prefetches);
      break;
    case 201:
      UPDATE_OPTION(_param_sink_frequency_threshold);
      break;
    case 202:
      UPDATE_OPTION(_param_sms_max_ii_factor);
      break;
    case 203:
      UPDATE_OPTION(_param_sms_min_sc);
      break;
    case 204:
      UPDATE_OPTION(_param_sra_max_propagations);
      break;
    case 205:
      UPDATE_OPTION(_param_ssa_name_def_chain_limit);
      break;
    case 206:
      UPDATE_OPTION(_param_ssp_buffer_size);
      break;
    case 207:
      UPDATE_OPTION(_param_stack_clash_protection_guard_size);
      break;
    case 208:
      UPDATE_OPTION(_param_stack_clash_protection_probe_interval);
      break;
    case 209:
      UPDATE_OPTION(_param_store_merging_allow_unaligned);
      break;
    case 210:
      UPDATE_OPTION(_param_store_merging_max_size);
      break;
    case 211:
      UPDATE_OPTION(_param_struct_reorg_cold_struct_ratio);
      break;
    case 212:
      UPDATE_OPTION(_param_switch_conversion_branch_ratio);
      break;
    case 213:
      UPDATE_OPTION(_param_tm_max_aggregate_size);
      break;
    case 214:
      UPDATE_OPTION(_param_tracer_dynamic_coverage_feedback);
      break;
    case 215:
      UPDATE_OPTION(_param_tracer_dynamic_coverage);
      break;
    case 216:
      UPDATE_OPTION(_param_tracer_max_code_growth);
      break;
    case 217:
      UPDATE_OPTION(_param_tracer_min_branch_probability_feedback);
      break;
    case 218:
      UPDATE_OPTION(_param_tracer_min_branch_probability);
      break;
    case 219:
      UPDATE_OPTION(_param_tracer_min_branch_ratio);
      break;
    case 220:
      UPDATE_OPTION(_param_uninit_control_dep_attempts);
      break;
    case 221:
      UPDATE_OPTION(_param_uninlined_function_insns);
      break;
    case 222:
      UPDATE_OPTION(_param_uninlined_function_thunk_insns);
      break;
    case 223:
      UPDATE_OPTION(_param_uninlined_function_thunk_time);
      break;
    case 224:
      UPDATE_OPTION(_param_unlikely_bb_count_fraction);
      break;
    case 225:
      UPDATE_OPTION(_param_unroll_jam_max_unroll);
      break;
    case 226:
      UPDATE_OPTION(_param_unroll_jam_min_percent);
      break;
    case 227:
      UPDATE_OPTION(_param_use_after_scope_direct_emission_threshold);
      break;
    case 228:
      UPDATE_OPTION(_param_vect_epilogues_nomask);
      break;
    case 229:
      UPDATE_OPTION(_param_vect_induction_float);
      break;
    case 230:
      UPDATE_OPTION(_param_vect_inner_loop_cost_factor);
      break;
    case 231:
      UPDATE_OPTION(_param_vect_max_peeling_for_alignment);
      break;
    case 232:
      UPDATE_OPTION(_param_vect_max_version_for_alias_checks);
      break;
    case 233:
      UPDATE_OPTION(_param_vect_max_version_for_alignment_checks);
      break;
    case 234:
      UPDATE_OPTION(_param_vect_partial_vector_usage);
      break;
    case 235:
      UPDATE_OPTION(_flag_sched_stalled_insns_dep);
      break;
    case 236:
      UPDATE_OPTION(_flag_tree_parallelize_loops);
      break;
    case 237:
      UPDATE_OPTION(_flag_aggressive_loop_optimizations);
      break;
    case 238:
      UPDATE_OPTION(_flag_align_functions);
      break;
    case 239:
      UPDATE_OPTION(_flag_align_jumps);
      break;
    case 240:
      UPDATE_OPTION(_flag_align_labels);
      break;
    case 241:
      UPDATE_OPTION(_flag_align_loops);
      break;
    case 242:
      UPDATE_OPTION(_flag_allocation_dce);
      break;
    case 243:
      UPDATE_OPTION(_flag_asynchronous_unwind_tables);
      break;
    case 244:
      UPDATE_OPTION(_flag_auto_inc_dec);
      break;
    case 245:
      UPDATE_OPTION(_flag_bit_tests);
      break;
    case 246:
      UPDATE_OPTION(_flag_branch_on_count_reg);
      break;
    case 247:
      UPDATE_OPTION(_flag_caller_saves);
      break;
    case 248:
      UPDATE_OPTION(_flag_code_hoisting);
      break;
    case 249:
      UPDATE_OPTION(_flag_combine_stack_adjustments);
      break;
    case 250:
      UPDATE_OPTION(_flag_compare_elim_after_reload);
      break;
    case 251:
      UPDATE_OPTION(_flag_cprop_registers);
      break;
    case 252:
      UPDATE_OPTION(_flag_crossjumping);
      break;
    case 253:
      UPDATE_OPTION(_flag_cse_follow_jumps);
      break;
    case 254:
      UPDATE_OPTION(_flag_dce);
      break;
    case 255:
      UPDATE_OPTION(_flag_defer_pop);
      break;
    case 256:
      UPDATE_OPTION(_flag_delete_dead_exceptions);
      break;
    case 257:
      UPDATE_OPTION(_flag_delete_null_pointer_checks);
      break;
    case 258:
      UPDATE_OPTION(_flag_devirtualize);
      break;
    case 259:
      UPDATE_OPTION(_flag_devirtualize_speculatively);
      break;
    case 260:
      UPDATE_OPTION(_flag_dse);
      break;
    case 261:
      UPDATE_OPTION(_flag_early_inlining);
      break;
    case 262:
      UPDATE_OPTION(_flag_expensive_optimizations);
      break;
    case 263:
      UPDATE_OPTION(_flag_forward_propagate);
      break;
    case 264:
      UPDATE_OPTION(_flag_fp_int_builtin_inexact);
      break;
    case 265:
      UPDATE_OPTION(_flag_gcse);
      break;
    case 266:
      UPDATE_OPTION(_flag_gcse_lm);
      break;
    case 267:
      UPDATE_OPTION(_flag_guess_branch_prob);
      break;
    case 268:
      UPDATE_OPTION(_flag_hoist_adjacent_loads);
      break;
    case 269:
      UPDATE_OPTION(_flag_if_conversion);
      break;
    case 270:
      UPDATE_OPTION(_flag_if_conversion2);
      break;
    case 271:
      UPDATE_OPTION(_flag_indirect_inlining);
      break;
    case 272:
      UPDATE_OPTION(_flag_inline_atomics);
      break;
    case 273:
      UPDATE_OPTION(_flag_inline_functions);
      break;
    case 274:
      UPDATE_OPTION(_flag_inline_functions_called_once);
      break;
    case 275:
      UPDATE_OPTION(_flag_inline_small_functions);
      break;
    case 276:
      UPDATE_OPTION(_flag_ipa_bit_cp);
      break;
    case 277:
      UPDATE_OPTION(_flag_ipa_cp);
      break;
    case 278:
      UPDATE_OPTION(_flag_ipa_icf);
      break;
    case 279:
      UPDATE_OPTION(_flag_ipa_icf_functions);
      break;
    case 280:
      UPDATE_OPTION(_flag_ipa_icf_variables);
      break;
    case 281:
      UPDATE_OPTION(_flag_ipa_modref);
      break;
    case 282:
      UPDATE_OPTION(_flag_ipa_profile);
      break;
    case 283:
      UPDATE_OPTION(_flag_ipa_pure_const);
      break;
    case 284:
      UPDATE_OPTION(_flag_ipa_ra);
      break;
    case 285:
      UPDATE_OPTION(_flag_ipa_reference);
      break;
    case 286:
      UPDATE_OPTION(_flag_ipa_reference_addressable);
      break;
    case 287:
      UPDATE_OPTION(_flag_ipa_sra);
      break;
    case 288:
      UPDATE_OPTION(_flag_ipa_stack_alignment);
      break;
    case 289:
      UPDATE_OPTION(_flag_ipa_strict_aliasing);
      break;
    case 290:
      UPDATE_OPTION(_flag_ipa_vrp);
      break;
    case 291:
      UPDATE_OPTION(_flag_ira_hoist_pressure);
      break;
    case 292:
      UPDATE_OPTION(_flag_ira_share_save_slots);
      break;
    case 293:
      UPDATE_OPTION(_flag_ira_share_spill_slots);
      break;
    case 294:
      UPDATE_OPTION(_flag_isolate_erroneous_paths_dereference);
      break;
    case 295:
      UPDATE_OPTION(_flag_ivopts);
      break;
    case 296:
      UPDATE_OPTION(_flag_jump_tables);
      break;
    case 297:
      UPDATE_OPTION(_flag_lifetime_dse);
      break;
    case 298:
      UPDATE_OPTION(_flag_lra_remat);
      break;
    case 299:
      UPDATE_OPTION(_flag_errno_math);
      break;
    case 300:
      UPDATE_OPTION(_flag_move_loop_invariants);
      break;
    case 301:
      UPDATE_OPTION(_flag_move_loop_stores);
      break;
    case 302:
      UPDATE_OPTION(_flag_omit_frame_pointer);
      break;
    case 303:
      UPDATE_OPTION(_flag_optimize_sibling_calls);
      break;
    case 304:
      UPDATE_OPTION(_flag_optimize_strlen);
      break;
    case 305:
      UPDATE_OPTION(_flag_partial_inlining);
      break;
    case 306:
      UPDATE_OPTION(_flag_peephole2);
      break;
    case 307:
      UPDATE_OPTION(_flag_plt);
      break;
    case 308:
      UPDATE_OPTION(_flag_prefetch_loop_arrays);
      break;
    case 309:
      UPDATE_OPTION(_flag_printf_return_value);
      break;
    case 310:
      UPDATE_OPTION(_flag_ree);
      break;
    case 311:
      UPDATE_OPTION(_flag_rename_registers);
      break;
    case 312:
      UPDATE_OPTION(_flag_reorder_blocks);
      break;
    case 313:
      UPDATE_OPTION(_flag_reorder_functions);
      break;
    case 314:
      UPDATE_OPTION(_flag_rerun_cse_after_loop);
      break;
    case 315:
      UPDATE_OPTION(_flag_rtti);
      break;
    case 316:
      UPDATE_OPTION(_flag_sched_critical_path_heuristic);
      break;
    case 317:
      UPDATE_OPTION(_flag_sched_dep_count_heuristic);
      break;
    case 318:
      UPDATE_OPTION(_flag_sched_group_heuristic);
      break;
    case 319:
      UPDATE_OPTION(_flag_schedule_interblock);
      break;
    case 320:
      UPDATE_OPTION(_flag_sched_last_insn_heuristic);
      break;
    case 321:
      UPDATE_OPTION(_flag_sched_pressure);
      break;
    case 322:
      UPDATE_OPTION(_flag_sched_rank_heuristic);
      break;
    case 323:
      UPDATE_OPTION(_flag_schedule_speculative);
      break;
    case 324:
      UPDATE_OPTION(_flag_sched_spec_insn_heuristic);
      break;
    case 325:
      UPDATE_OPTION(_flag_schedule_fusion);
      break;
    case 326:
      UPDATE_OPTION(_flag_schedule_insns);
      break;
    case 327:
      UPDATE_OPTION(_flag_schedule_insns_after_reload);
      break;
    case 328:
      UPDATE_OPTION(_flag_section_anchors);
      break;
    case 329:
      UPDATE_OPTION(_flag_semantic_interposition);
      break;
    case 330:
      UPDATE_OPTION(_flag_shrink_wrap);
      break;
    case 331:
      UPDATE_OPTION(_flag_shrink_wrap_separate);
      break;
    case 332:
      UPDATE_OPTION(_flag_signed_zeros);
      break;
    case 333:
      UPDATE_OPTION(_flag_split_ivs_in_unroller);
      break;
    case 334:
      UPDATE_OPTION(_flag_split_wide_types);
      break;
    case 335:
      UPDATE_OPTION(_flag_ssa_backprop);
      break;
    case 336:
      UPDATE_OPTION(_flag_ssa_phiopt);
      break;
    case 337:
      UPDATE_OPTION(_flag_stdarg_opt);
      break;
    case 338:
      UPDATE_OPTION(_flag_store_merging);
      break;
    case 339:
      UPDATE_OPTION(_flag_strict_aliasing);
      break;
    case 340:
      UPDATE_OPTION(_flag_strict_volatile_bitfields);
      break;
    case 341:
      UPDATE_OPTION(_flag_thread_jumps);
      break;
    case 342:
      UPDATE_OPTION(_flag_threadsafe_statics);
      break;
    case 343:
      UPDATE_OPTION(_flag_toplevel_reorder);
      break;
    case 344:
      UPDATE_OPTION(_flag_trapping_math);
      break;
    case 345:
      UPDATE_OPTION(_flag_tree_bit_ccp);
      break;
    case 346:
      UPDATE_OPTION(_flag_tree_builtin_call_dce);
      break;
    case 347:
      UPDATE_OPTION(_flag_tree_ccp);
      break;
    case 348:
      UPDATE_OPTION(_flag_tree_ch);
      break;
    case 349:
      UPDATE_OPTION(_flag_tree_coalesce_vars);
      break;
    case 350:
      UPDATE_OPTION(_flag_tree_copy_prop);
      break;
    case 351:
      UPDATE_OPTION(_flag_tree_cselim);
      break;
    case 352:
      UPDATE_OPTION(_flag_tree_dce);
      break;
    case 353:
      UPDATE_OPTION(_flag_tree_dom);
      break;
    case 354:
      UPDATE_OPTION(_flag_tree_dse);
      break;
    case 355:
      UPDATE_OPTION(_flag_tree_forwprop);
      break;
    case 356:
      UPDATE_OPTION(_flag_tree_fre);
      break;
    case 357:
      UPDATE_OPTION(_flag_tree_loop_distribute_patterns);
      break;
    case 358:
      UPDATE_OPTION(_flag_tree_loop_if_convert);
      break;
    case 359:
      UPDATE_OPTION(_flag_tree_loop_im);
      break;
    case 360:
      UPDATE_OPTION(_flag_tree_loop_ivcanon);
      break;
    case 361:
      UPDATE_OPTION(_flag_tree_loop_optimize);
      break;
    case 362:
      UPDATE_OPTION(_flag_tree_loop_vectorize);
      break;
    case 363:
      UPDATE_OPTION(_flag_tree_phiprop);
      break;
    case 364:
      UPDATE_OPTION(_flag_tree_pre);
      break;
    case 365:
      UPDATE_OPTION(_flag_tree_pta);
      break;
    case 366:
      UPDATE_OPTION(_flag_tree_reassoc);
      break;
    case 367:
      UPDATE_OPTION(_flag_tree_scev_cprop);
      break;
    case 368:
      UPDATE_OPTION(_flag_tree_sink);
      break;
    case 369:
      UPDATE_OPTION(_flag_tree_slp_vectorize);
      break;
    case 370:
      UPDATE_OPTION(_flag_tree_slsr);
      break;
    case 371:
      UPDATE_OPTION(_flag_tree_sra);
      break;
    case 372:
      UPDATE_OPTION(_flag_tree_switch_conversion);
      break;
    case 373:
      UPDATE_OPTION(_flag_tree_tail_merge);
      break;
    case 374:
      UPDATE_OPTION(_flag_tree_ter);
      break;
    case 375:
      UPDATE_OPTION(_flag_tree_vrp);
      break;
    case 376:
      UPDATE_OPTION(_flag_cunroll_grow_size);
      break;
    case 377:
      UPDATE_OPTION(_flag_unroll_loops);
      break;
    case 378:
      UPDATE_OPTION(_flag_unwind_tables);
      break;
    case 379:
      UPDATE_OPTION(_flag_var_tracking);
      break;
    case 380:
      UPDATE_OPTION(_flag_var_tracking_assignments);
      break;
    case 381:
      UPDATE_OPTION(_flag_var_tracking_uninit);
      break;
    case 382:
      UPDATE_OPTION(_flag_web);
      break;
    default:
      break;
  }
}

void update(struct gcc_options *opts, struct gcc_options *opts_set,
            std::string option, int value,
            std::map<std::string, int> &opt_idx_map) {
  int opt_idx = opt_idx_map[option];
  if (opt_idx == -1) {
    std::cout << "Option " << option << " is not found in the permitted "
              << "option list in `options_utils.h`." << std::endl;
    return;
  }
  check_option(opt_idx, opts, opts_set, option, value);
}

}  // namespace ai4c

#endif  // AI4C_OPTION_UTILS_H