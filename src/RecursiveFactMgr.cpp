// ****************************** ExtendedCsmith ****************************** >>
//
//  RecursiveFactMgr.cpp
//  extended-csmith
//
//  Created by eitan mashiah on 3.8.2016.
//  Copyright © 2016 eitan mashiah. All rights reserved.
//

#include "RecursiveFactMgr.h"
#include "RecursiveBlock.h"
#include "Block.h"
#include "FactMgr.h"
#include "Function.h"

RecursiveFactMgr::RecursiveFactMgr(vector<const Block*> call_chain, FactMgr* fact_mgr)
    : curr_fact_mgr(fact_mgr),
      func(fact_mgr->func),
      merged_fact_mgr(NULL),
      merged_first_fact_mgr(NULL),
      max_fact_mgrs(CGOptions::max_fact_sets_in_inclusive_fact_set())
{
    map_fact_mgrs[call_chain] = fact_mgr;
}

RecursiveFactMgr::~RecursiveFactMgr(void)
{
    if (merged_fact_mgr)
        merged_fact_mgr = NULL;
    
    if (merged_first_fact_mgr)
        merged_first_fact_mgr = NULL;
}

/**
 * Adds a new fact manager and sets it to be the current fact manager.
 */
void
RecursiveFactMgr::add_fact_mgr(vector<const Block*> call_chain, FactMgr* fact_mgr)
{
    map_fact_mgrs[call_chain] = fact_mgr;
    curr_fact_mgr = fact_mgr;
}

/**
 * Removes from the map all of the elements except the first one.
 * In addition, resets input/output env of this statement and all statements included to empty,
 * in the left fact manager.
 */
void
RecursiveFactMgr::rec_call_reset_map_fact_mgrs(const Statement* stm)
{
    map<vector<const Block*>, FactMgr*>::iterator iter = map_fact_mgrs.begin();
    curr_fact_mgr = iter->second;
    iter++;
    map_fact_mgrs.erase(iter, map_fact_mgrs.end());
    curr_fact_mgr->reset_stm_fact_maps(stm);
}

/**
 * Inserts all of the deleted fact managers back to the map_fact_mgrs.
 * In addition, for each fact manager, resets input/output env and all statements included to empty,
 * and sets its cfg_edges to that of merged_fact_mgr (it was updated after removing the last statement).
 */
void
RecursiveFactMgr::rec_func_reset_map_fact_mgrs(const Statement* stm)
{
    map_fact_mgrs.insert(map_deleted_fact_mgrs.begin(), map_deleted_fact_mgrs.end());
    map<vector<const Block*>, FactMgr*>::iterator iter = map_fact_mgrs.begin();
    for (iter = map_fact_mgrs.begin(); iter != map_fact_mgrs.end(); iter++) {
        vector<const Block*> call_chain = iter->first;
        FactMgr* fm = iter->second;
        fm->reset_stm_fact_maps(stm);
        fm->global_facts = map_pre_facts[call_chain];
        fm->cfg_edges = curr_fact_mgr->cfg_edges;
    }
}

/**
 * Updates the map after preforming DFA analysis, following a recursive call creation.
 */
void
RecursiveFactMgr::update_map_fact_mgrs(const Statement* rec_if, const Statement*rec_block, const Statement* rec_stmt)
{
    // save the return facts of the first fact manager to global_ret_facts
    FactVec ret_facts;
    func->blocks[0]->add_back_return_facts(map_fact_mgrs.begin()->second, ret_facts);
    global_ret_facts = ret_facts;
    
    vector<const Block*> to_remove;
    map<vector<const Block*>, FactMgr*>::iterator iter;
    for (iter = map_fact_mgrs.begin(); iter != map_fact_mgrs.end(); iter++) {
        bool is_last = (std::next(iter) == map_fact_mgrs.end());
        bool is_penultimate = (!is_last && std::next(iter, 2) == map_fact_mgrs.end());
        if (is_penultimate) {
            to_remove = iter->first;
            continue;
        }
        
        FactMgr *fm = iter->second;
        FactMgr *next_fm = is_last? fm : std::next(iter)->second;
        
        // handover from callee to caller: points-to/union facts
        FactVec ret_facts;
        func->blocks[0]->add_back_return_facts(next_fm, ret_facts);
        dynamic_cast<RecursiveBlock*>(func->blocks[0])->rec_call->save_return_fact(ret_facts);
        fm->global_facts = ret_facts;
        FactVec facts_copy = fm->map_facts_in[rec_stmt];
        renew_facts(facts_copy, fm->global_facts);
        fm->global_facts = facts_copy;
        
        // set map_facts_out values for the statements containing the recursive call
        fm->remove_rv_facts(fm->global_facts);
        fm->map_facts_out[rec_stmt] = fm->global_facts;
        if (rec_if && rec_block) {
            fm->map_facts_out[rec_block] = fm->global_facts;
            fm->map_facts_out[rec_if] = fm->global_facts;
        }
    }
    map_fact_mgrs.erase(to_remove);
    
    init_merged_fact_mgr();
}

/**
 * Initializes the merged_fact_mgr fact mangar, 
 * just before starting to build the sub-block after the recursive call.
 */
void
RecursiveFactMgr::init_merged_fact_mgr()
{
    merged_fact_mgr = new FactMgr(func);

    map<vector<const Block*>, FactMgr*>::iterator iter;
    for (iter = map_fact_mgrs.begin(); iter != map_fact_mgrs.end(); iter++) {
        FactMgr *fm = iter->second;
        merge_facts(merged_fact_mgr->global_facts, fm->global_facts);
        
        map<const Statement*, FactVec>::iterator iter_facts_in;
        for (iter_facts_in = fm->map_facts_in.begin(); iter_facts_in != fm->map_facts_in.end(); iter_facts_in++) {
            const Statement* stm = iter_facts_in->first;
            FactVec facts_in = iter_facts_in->second;
            merge_facts(merged_fact_mgr->map_facts_in[stm], facts_in);
        }

        map<const Statement*, FactVec>::iterator iter_facts_out;
        for (iter_facts_out = fm->map_facts_out.begin(); iter_facts_out != fm->map_facts_out.end(); iter_facts_out++) {
            const Statement* stm = iter_facts_out->first;
            FactVec facts_out = iter_facts_out->second;
            merge_facts(merged_fact_mgr->map_facts_out[stm], facts_out);
        }
        
        if (iter ==  map_fact_mgrs.begin()) {
            merged_fact_mgr->map_visited = fm->map_visited;
            merged_fact_mgr->cfg_edges = fm->cfg_edges;
        }
    }
    
    curr_fact_mgr = merged_fact_mgr;
}

/**
 * Saves the pre-facts before starting to generate the sub-block following the reucrsive call.
 */
void
RecursiveFactMgr::save_pre_facts()
{
    map<vector<const Block*>, FactMgr*>::iterator iter;
    for (iter = map_fact_mgrs.begin(); iter != map_fact_mgrs.end(); iter++) {
        vector<const Block*> call_chain = iter->first;
        FactMgr *fm = iter->second;
        map_pre_facts[call_chain] = fm->global_facts;
    }
}

/**
 * Prepares the map for the current iteration in DFS analysis,
 * performed after generating the recursive call.
 */
void
RecursiveFactMgr::prepare_for_curr_iteration()
{
    vector<const Block*> to_remove;
    map<vector<const Block*>, FactMgr*>::iterator iter;
    RecursiveBlock* rec_block = dynamic_cast<RecursiveBlock*>(func->blocks[0]);
    for (iter = map_fact_mgrs.begin(); iter != map_fact_mgrs.end(); iter++) {
        bool is_last = (std::next(iter) == map_fact_mgrs.end());
        bool is_penultimate = (!is_last && std::next(iter, 2) == map_fact_mgrs.end());
        if (is_penultimate) {
            to_remove = iter->first;
            continue;
        }
        
        FactMgr *fm = iter->second;
        FactMgr *next_fm = is_last? fm : std::next(iter)->second;
        
        // handover from callee to caller: points-to/union facts
        FactVec ret_facts;
        rec_block->add_back_post_return_facts(next_fm, ret_facts);
        rec_block->rec_call->save_return_fact(ret_facts);
        FactVec outputs = next_fm->map_facts_out[rec_block];
        merge_facts(outputs, ret_facts);
        Statement *first_stm = rec_block->stms[rec_block->before_block_size + 1];
        FactVec& facts_copy = fm->map_facts_in[first_stm];
        renew_facts(facts_copy, outputs);
        fm->global_facts = facts_copy;
        
        if (iter == map_fact_mgrs.begin()) { // first fact manager
            merge_facts(global_ret_facts, outputs);
        }
    }
    
    // remove the penultimate fact manager
    map_deleted_fact_mgrs[to_remove] = map_fact_mgrs[to_remove];
    map_fact_mgrs.erase(to_remove);
}

/**
 * Updates the maps of the current function and the next function in the recursive call cycle.
 * This function is called after preforming DFA analysis, following the recursive call creations.
 */
void
RecursiveFactMgr::update_map_fact_mgrs_for_adjacent(MutuallyRecursiveFunction *next_func, const Statement* rec_if,
                                                    const Statement*rec_block, const Statement* rec_stmt)
{
    RecursiveFactMgr *next_rec_fm = get_rec_fact_mgr_for_func(next_func);
    RecursiveBlock *next_body = dynamic_cast<RecursiveBlock *>(next_func->blocks[0]);

    // save the return facts of the first fact manager of the first function to global_ret_facts
    if (next_func->is_first()) {
        FactVec ret_facts;
        next_body->add_back_return_facts(next_rec_fm->map_fact_mgrs.begin()->second, ret_facts);
        next_rec_fm->global_ret_facts = ret_facts;
    }
    
    vector<const Block*> to_remove;
    map<vector<const Block*>, FactMgr*>::iterator curr_iter;
    map<vector<const Block*>, FactMgr*>::iterator next_iter;
    for (curr_iter = map_fact_mgrs.begin(), next_iter = std::next(next_rec_fm->map_fact_mgrs.begin());
         curr_iter != map_fact_mgrs.end() && next_iter != next_rec_fm->map_fact_mgrs.end(); curr_iter++, next_iter++) {
        bool is_last = (std::next(next_iter) == map_fact_mgrs.end());
        bool is_penultimate = (!is_last && std::next(next_iter, 2) == map_fact_mgrs.end());
        if (is_penultimate) {
            to_remove = next_iter->first;
            continue;
        }
        
        FactMgr *fm = curr_iter->second;
        FactMgr *next_fm = next_iter->second;
        
        // handover from callee to caller: points-to/union facts
        FactVec ret_facts;
        next_body->add_back_return_facts(next_fm, ret_facts);
        next_body->rec_call->save_return_fact(ret_facts);
        fm->global_facts = ret_facts;
        FactVec facts_copy = fm->map_facts_in[rec_stmt];
        renew_facts(facts_copy, fm->global_facts);
        fm->global_facts = facts_copy;
        
        // set map_facts_out values for the statements containing the recursive call
        fm->remove_rv_facts(fm->global_facts);
        fm->map_facts_out[rec_stmt] = fm->global_facts;
        if (rec_if && rec_block) {
            fm->map_facts_out[rec_block] = fm->global_facts;
            fm->map_facts_out[rec_if] = fm->global_facts;
        }
    }
    next_rec_fm->map_fact_mgrs.erase(to_remove);
    
    //init_merged_fact_mgr();
}
// **************************************************************************** <<
