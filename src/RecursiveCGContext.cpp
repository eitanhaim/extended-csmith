// ****************************** ExtendedCsmith ****************************** >>
//
//  RecursiveCGContext.cpp
//  extended-csmith
//
//  Created by eitan mashiah on 3.8.2016.
//  Copyright © 2016 eitan mashiah. All rights reserved.
//

#include "RecursiveCGContext.h"
#include "RecursiveFactMgr.h"
#include "RecursiveBlock.h"
#include "Block.h"
#include "CGContext.h"
#include "Function.h"

RecursiveCGContext::RecursiveCGContext(CGContext* cg_context)
    : curr_cg_context(cg_context),
      func(cg_context->get_current_func()),
      max_cg_contexts(CGOptions::max_fact_sets_in_inclusive_fact_set())
{
    map_cg_contexts[cg_context->call_chain] = cg_context;
}

RecursiveCGContext::~RecursiveCGContext(void)
{
    delete merged_cg_context;
}

/**
 * Adds a new context and sets it to be the current context.
 */
void
RecursiveCGContext::add_cg_context(CGContext* cg_context)
{
    map_cg_contexts[cg_context->call_chain] = cg_context;
    curr_cg_context = cg_context;
}

/**
 * Removes from the map all of the elements except the first one.
 * In addition, resets incoming effects in the left context.
 */
void
RecursiveCGContext::rec_call_reset_map_cg_contexts(const Effect& pre_effect)
{
    map<vector<const Block*>, CGContext*>::iterator iter = map_cg_contexts.begin();
    curr_cg_context = iter->second;
    iter++;
    map_cg_contexts.erase(iter, map_cg_contexts.end());
    curr_cg_context->reset_effect_accum(pre_effect);
}

/**
 * Inserts all of the deleted contexts back to the map_cg_contexts.
 * In addition, for each context, resets incoming effects.
 */
void
RecursiveCGContext::rec_func_reset_map_cg_contexts()
{
    map_cg_contexts.insert(map_deleted_cg_contexts.begin(), map_deleted_cg_contexts.end());
    map<vector<const Block*>, CGContext*>::iterator iter;
    for (iter = map_cg_contexts.begin(); iter != map_cg_contexts.end(); iter++) {
        vector<const Block*> call_chain = iter->first;
        CGContext* cg_context = iter->second;
        cg_context->reset_effect_accum(map_pre_effects[call_chain]);
    }
}

/**
 * Updates the map after preforming DFA analysis, following a recursive call creation.
 */
void
RecursiveCGContext::update_map_cg_contexts(const Statement* rec_if, const Statement*rec_block, const Statement* rec_stmt)
{
    vector<const Block*> to_remove;
    map<vector<const Block*>, CGContext*>::iterator iter;
    for (iter = map_cg_contexts.begin(); iter != map_cg_contexts.end(); iter++) {
        bool is_last = (std::next(iter) == map_cg_contexts.end());
        bool is_penultimate = (!is_last && std::next(iter, 2) == map_cg_contexts.end());
        if (is_penultimate) {
            to_remove = iter->first;
            continue;
        }
        
        CGContext *cg_context = iter->second;
        CGContext *next_cg_context = is_last? cg_context : std::next(iter)->second;
        
        // handover from callee to caller: effects
        RecursiveFactMgr *rec_fm = get_rec_fact_mgr_for_func(func);
        FactMgr* fm = rec_fm->get_fact_mgr(cg_context);
        cg_context->reset_effect_stm(fm->map_stm_effect[rec_stmt]);
        cg_context->reset_effect_accum(fm->map_accum_effect[rec_stmt]);
        cg_context->add_effect(*next_cg_context->get_effect_accum());
        
        // set the values of map_stm_effect and map_accum_effect,
        // for the statements containing the recursive call
        fm->map_stm_effect[rec_stmt] = cg_context->get_effect_stm();
        fm->map_accum_effect[rec_stmt] = *cg_context->get_effect_accum();
        if (rec_if && rec_block) {
            fm->map_stm_effect[rec_block].add_effect(cg_context->get_effect_stm());
            fm->map_accum_effect[rec_block].add_effect(*cg_context->get_effect_accum());
            fm->map_stm_effect[rec_if].add_effect(cg_context->get_effect_stm());
            fm->map_accum_effect[rec_if].add_effect(*cg_context->get_effect_accum());
        }
    }
    map_cg_contexts.erase(to_remove);
    
    init_merged_cg_context();
}

/**
 * Initializes the merged_cg_context context,
 * just before starting to build the sub-block after the recursive call.
 */
void
RecursiveCGContext::init_merged_cg_context()
{
    map<vector<const Block*>, CGContext*>::iterator iter = map_cg_contexts.begin();
    CGContext *cgc = iter->second;
    Effect *effect_accum = new Effect(cgc->get_accum_effect());
    merged_cg_context = new CGContext(*cgc, cgc->get_effect_context(), effect_accum);
    iter++;
    
    for (; iter != map_cg_contexts.end(); iter++) {
        cgc = iter->second;
        merged_cg_context->add_effect(*cgc->get_effect_accum());
    }
    
    curr_cg_context = merged_cg_context;
}

/**
 * Saves the pre-effects before starting to generate the sub-block following the reucrsive call.
 */
void
RecursiveCGContext::save_pre_effects()
{
    map<vector<const Block*>, CGContext*>::iterator iter;
    for (iter = map_cg_contexts.begin(); iter != map_cg_contexts.end(); iter++) {
        vector<const Block*> call_chain = iter->first;
        CGContext *cgc = iter->second;
        map_pre_effects[call_chain] = cgc->get_accum_effect();
    }

}

/**
 * Resets the effect accums of the contexts from map_cg_contexts.
 */
void
RecursiveCGContext::reset_effect_accums()
{
    Effect eff;
    map<vector<const Block*>, CGContext*>::iterator iter;
    for (iter = map_cg_contexts.begin(); iter != map_cg_contexts.end(); iter++) {
        vector<const Block*> call_chain = iter->first;
        CGContext* cg_context = iter->second;
        cg_context->reset_effect_accum(map_pre_effects[call_chain]);
        eff.add_effect(map_pre_effects[call_chain]);
    }
    merged_cg_context->reset_effect_accum(eff);
}

/**
 * Prepares the map for the current iteration in DFS analysis,
 * performed after generating the recursive call.
 */
void
RecursiveCGContext::prepare_for_curr_iteration()
{
    Statement *rec_stmt = dynamic_cast<RecursiveBlock*>(func->blocks[0])->outermost_rec_stmt;
    RecursiveFactMgr *rec_fm = get_rec_fact_mgr_for_func(func);
    merged_cg_context->reset_effect_stm(rec_fm->merged_fact_mgr->map_stm_effect[rec_stmt]);
    merged_cg_context->reset_effect_accum(rec_fm->merged_fact_mgr->map_accum_effect[rec_stmt]);

    vector<const Block*> to_remove;
    map<vector<const Block*>, CGContext*>::iterator iter;
    for (iter = map_cg_contexts.begin(); iter != map_cg_contexts.end(); iter++) {
        bool is_last = (std::next(iter) == map_cg_contexts.end());
        bool is_penultimate = (!is_last && std::next(iter, 2) == map_cg_contexts.end());
        if (is_penultimate) {
            to_remove = iter->first;
            continue;
        }
        
        CGContext *cg_context = iter->second;
        CGContext *next_cg_context = is_last? cg_context : std::next(iter)->second;
        
        // handover from callee to caller: effects
        FactMgr* fm = rec_fm->get_fact_mgr(cg_context);
        cg_context->reset_effect_stm(fm->map_stm_effect[rec_stmt]);
        cg_context->reset_effect_accum(fm->map_accum_effect[rec_stmt]);
        cg_context->add_effect(next_cg_context->get_accum_effect());
        fm->map_stm_effect[rec_stmt].add_effect(cg_context->get_effect_stm());
        fm->map_accum_effect[rec_stmt].add_effect(cg_context->get_accum_effect());
        
        merged_cg_context->add_effect(cg_context->get_accum_effect());
        rec_fm->merged_fact_mgr->map_stm_effect[rec_stmt].add_effect(cg_context->get_effect_stm());
        rec_fm->merged_fact_mgr->map_accum_effect[rec_stmt].add_effect(cg_context->get_accum_effect());
    }
    
    // remove the penultimate context
    map_deleted_cg_contexts[to_remove] = map_cg_contexts[to_remove];
    map_cg_contexts.erase(to_remove);
}
// **************************************************************************** <<
