// ****************************** ExtendedCsmith ****************************** >>
//
//  RecursiveBlock.cpp
//  extended-csmith
//
//  Created by eitan mashiah on 4.8.2016.
//  Copyright © 2016 eitan mashiah. All rights reserved.
//

#include "RecursiveBlock.h"
#include "ImmediateRecursiveFunction.h"
#include "MutuallyRecursiveFunction.h"
#include "RecursiveFactMgr.h"
#include "RecursiveCGContext.h"
#include "FactMgr.h"
#include "CGContext.h"
#include "StatementExpr.h"
#include "StatementIf.h"
#include "Error.h"
#include "DepthSpec.h"
#include "Common.h"


RecursiveBlock::RecursiveBlock(Block* b, int block_size)
    : Block(b, block_size),
      rec_call(NULL),
      outermost_rec_stmt(NULL)
{
    // nothing to do
}

RecursiveBlock::~RecursiveBlock()
{
    // nothing to do
}

static unsigned int
RecursiveBlockProbability(RecursiveBlock &rec_block)
{
    vector<unsigned int> v;
    v.push_back(rec_block.get_actual_block_size() - 1);
    VectorFilter filter(v, NOT_FILTER_OUT);
    filter.disable(fDefault);
    return rnd_upto(rec_block.get_actual_block_size(), &filter);
}

RecursiveBlock *
RecursiveBlock::make_random(RecursiveCGContext& rec_cg_context)
{
    DEPTH_GUARD_BY_TYPE_RETURN(dtBlock, NULL);
    
    CGContext *cg_context = rec_cg_context.get_curr_cg_context();
    Function *curr_func = cg_context->get_current_func();
    assert(curr_func);
    
    bool is_immediate_or_first_mutually = true;
    MutuallyRecursiveFunction *mr_func = NULL;
    if (curr_func->func_type == eMutuallyRecursive) {
        mr_func = dynamic_cast<MutuallyRecursiveFunction *>(curr_func);
        is_immediate_or_first_mutually = mr_func->is_first();
    }
    
    RecursiveBlock *b = new RecursiveBlock(cg_context->get_current_block(), CGOptions::max_block_size());
    b->func = curr_func;
    b->looping = false;
    b->in_array_loop = false;
    
    // push this block onto the variable scope stack
    curr_func->stack.push_back(b);
    curr_func->blocks.push_back(b);
    
    // record global facts at this moment so that subsequent statement
    // inside the block doesn't ruin it
    RecursiveFactMgr *rec_fm = get_rec_fact_mgr_for_func(curr_func);
    FactMgr *fm = rec_fm->get_curr_fact_mgr();
    fm->set_fact_in(b, fm->global_facts);
    Effect pre_effect = cg_context->get_accum_effect();
    Effect* effect_accum = cg_context->get_effect_accum();
    
    // choose the actual size of this block
    b->actual_block_size = BlockProbability(*b) + 1;
    if (Error::get_error() != SUCCESS) {
        curr_func->stack.pop_back();
        delete b;
        return NULL;
    }
    
    // choose the sizes of the sub-blocks before and after the recursive call
    b->before_block_size = RecursiveBlockProbability(*b);
    if (Error::get_error() != SUCCESS) {
        curr_func->stack.pop_back();
        delete b;
        return NULL;
    }
    b->after_block_size =  b->actual_block_size - b->before_block_size - 1;

    if (b->stm_id == 1)
        BREAK_NOP;			// for debugging
    
    // generate the sub-block before the recursive call
    if (!is_immediate_or_first_mutually)
        mr_func->is_building_before = true;
    if (mr_func && mr_func->is_first()) {
        mr_func->first_pre_effect = pre_effect;
        mr_func->first_pre_facts = fm->global_facts;
    }
    for (int i = 0; i < b->before_block_size; i++) {
        map<const Statement*, FactVec> facts_in_copy = fm->map_facts_in;
        map<const Statement*, FactVec> facts_out_copy = fm->map_facts_out;
        map<const Statement*, Effect>  stm_effect_copy = fm->map_stm_effect;
        map<const Statement*, Effect>  accum_effect_copy = fm->map_accum_effect;
        FactVec pre_facts = fm->global_facts;
        Effect pre_effect = cg_context->get_accum_effect();
        
        Statement *s = Statement::make_random(*cg_context);
        if (!s || s->must_return()) {
            fm->map_facts_in = facts_in_copy;
            fm->map_facts_out = facts_out_copy;
            fm->map_stm_effect = stm_effect_copy;
            fm->map_accum_effect = accum_effect_copy;
            cg_context->reset_effect_accum(pre_effect);
            fm->restore_facts(pre_facts);
            delete s;
            i--;
        } else {
            b->stms.push_back(s);
        }
    }
    if (!is_immediate_or_first_mutually)
        mr_func->is_building_before = false;

    if (Error::get_error() != SUCCESS) {
        curr_func->stack.pop_back();
        delete b;
        return NULL;
    }
    
    // generate a statement containing the recursive call
    if (!is_immediate_or_first_mutually)
        b->outermost_rec_stmt = Statement::make_random_recursive(*cg_context, eInvoke);
    else
        b->outermost_rec_stmt = Statement::make_random_recursive(*cg_context, eIfElse);

    b->stms.push_back(b->outermost_rec_stmt);
    
    if (Error::get_error() != SUCCESS) {
        curr_func->stack.pop_back();
        delete b;
        return NULL;
    }
    
    if (!is_immediate_or_first_mutually) {
        return b;
    }
    
    // perform DFA analysis after generating the recursive call
    bool is_ok;
    if (curr_func->func_type == eImmediateRecursive)
        is_ok = b->immediate_rec_call_post_creation_analysis(rec_cg_context, pre_effect);
    else if (mr_func && mr_func->is_first())
        is_ok = mutually_rec_call_post_creation_analysis(mr_func);

    if (!is_ok || Error::get_error() != SUCCESS) {
        curr_func->stack.pop_back();
        delete b;
        return NULL;
    }
    
    // generate the sub-block after the recursive call
    cg_context = rec_cg_context.get_curr_cg_context();
    fm = rec_fm->get_curr_fact_mgr();
    rec_cg_context.save_pre_effects();
    rec_fm->save_pre_facts();
    for (int i = 0; i < b->after_block_size; i++) {
        Statement *s = Statement::make_random(*cg_context);
        if (!s)
            break;
        b->stms.push_back(s);
        s->update_maps();
        if (s->must_return()) {
            break;
        }
    }
    if (Error::get_error() != SUCCESS) {
        curr_func->stack.pop_back();
        delete b;
        return NULL;
    }
    
    // perform DFA analysis after the complete creation
    if (curr_func->func_type == eImmediateRecursive)
        b->immediate_rec_func_post_creation_analysis(rec_cg_context);
    else
        mutually_rec_func_post_creation_analysis(mr_func);

    if (Error::get_error() != SUCCESS) {
        curr_func->stack.pop_back();
        delete b;
        return NULL;
    }
    
    // update the external effect_accum
    cg_context = rec_cg_context.get_curr_cg_context();
    effect_accum->copy_eff(*cg_context->get_effect_accum());
    
    curr_func->stack.pop_back();
    if (Error::get_error() != SUCCESS) {
        delete b;
        return NULL;
    }
    
    Error::set_error(SUCCESS);
    return b;
}

void
RecursiveBlock::add_back_return_facts(FactMgr* fm, std::vector<const Fact*>& facts) const
{
    if (!func->is_built()) {
        Statement::add_back_return_facts(fm, facts);
    } else {
        RecursiveFactMgr *rec_fm = get_rec_fact_mgr_for_func(func);
        merge_facts(facts, rec_fm->get_global_ret_facts());
    }
}

/**
 * Merges facts with all of the output facts of return statements in the function whose fact manager is fm.
 * The relevant return statements are those belong to the sub-block after the recursive call.
 */
void
RecursiveBlock::add_back_post_return_facts(FactMgr* fm, std::vector<const Fact*>& facts) const
{
    assert(outermost_rec_stmt);
    if (eType == eReturn) {
        merge_facts(facts, fm->map_facts_out[this]);
    } else {
        vector<const Block*> blks;
        get_blocks(blks);
        for (size_t i=0; i<blks.size(); i++) {
            for (size_t j=0; j<blks[i]->stms.size(); j++) {
                if (blks[i]->stms[j]->stm_id > outermost_rec_stmt->stm_id) {
                    blks[i]->stms[j]->add_back_return_facts(fm, facts);
                }
            }

        }
    }
}

/**
 * Once generated the sub-block before the recursive call, as well as the recursive call itself,
 * verify whether some statement caused the analyzer to fail during the 2nd iteration of the function body
 * (in most case, a null/dead pointer dereference would do it).
 * If so, delete the statement in which analyzer fails and all subsequent statements up to the recursive call.
 *
 * Also performs effect analysis.
 */
bool
RecursiveBlock::immediate_rec_call_post_creation_analysis(RecursiveCGContext& rec_cg_context, const Effect& pre_effect)
{
    RecursiveFactMgr *rec_fm = get_rec_fact_mgr_for_func(func);
    CGContext *cg_context = rec_cg_context.get_curr_cg_context();
    FactMgr *fm = rec_fm->get_curr_fact_mgr();
    
    // set map values for the current block
    fm->map_visited[this] = true;
    set_accumulated_effect(*cg_context);
    vector<const Fact*> post_facts = fm->global_facts;
    FactMgr::update_facts_for_oos_vars(local_vars, post_facts);
    fm->remove_rv_facts(post_facts);
    fm->set_fact_out(this, post_facts);
    
    // perform DFA analysis
    int fail_index;
    bool visit_once = false;
    FactVec facts_copy = fm->map_facts_in[this];
    FactVec outputs = fm->global_facts;
    prepare_for_next_iteration(outputs, rec_cg_context);
    while (!immediate_rec_call_find_fixed_point(outputs, rec_cg_context, fail_index, visit_once)) {
        if (fail_index == before_block_size)
            return false; // cannot recover from a probelm in the recursive call
        
        size_t i;
        rec_cg_context.set_curr_cg_context(cg_context);
        rec_fm->set_curr_fact_mgr(fm);
        for (i = fail_index; i < before_block_size; i++) {
            remove_stmt(stms[i]);
            i = fail_index - 1;
            before_block_size--;
        }
        
        // if we delete some statements, next visit must go through statements (no shortcut)
        visit_once = true;
        
        // clean up the map from previous analysis that might include facts caused by deleted statements
        rec_fm->rec_call_reset_map_fact_mgrs(this);
        outputs = facts_copy;
        
        // reset incoming effects
        rec_cg_context.rec_call_reset_map_cg_contexts(pre_effect);
    }
    
    // update the maps map_fact_mgrs and map_cg_contexts
    const Statement *rec_if, *rec_block, *rec_stmt;
    get_rec_stmts(rec_if, rec_block, rec_stmt);
    rec_fm->update_map_fact_mgrs(rec_if, rec_block, rec_stmt);
    rec_cg_context.update_map_cg_contexts(rec_if, rec_block, rec_stmt);
    
    return true;
}

/**
 * DFA analysis for a sub-block before the recursive call in a recursive function
 *
 * params:
 *    outputs: the outputs env after generating the current block
 *    rec_cg_context: code generation context for the recursive function
 *    fail_index: records which statement in this block caused analyzer to fail
 *    visit_once: when is true, the statements in this block must be visited at least once
 */
bool
RecursiveBlock::immediate_rec_call_find_fixed_point(FactVec outputs, RecursiveCGContext& rec_cg_context, int& fail_index, bool visit_once) const
{
    RecursiveFactMgr *rec_fm = get_rec_fact_mgr_for_func(func);
    CGContext *cg_context = rec_cg_context.get_curr_cg_context();
    FactMgr *fm = rec_fm->get_curr_fact_mgr();
    FactVec inputs;
    
    size_t i;
    static int g = 0;
    int cnt = 0;
    do {
        // if reached the maximum number of fact sets in an inclusive fact set,
        // include the inputs from the previous iteration in this one
        if (fm->map_visited[this]) {
            if (cnt++ > 7) {
                // takes too many iterations to reach a fixed point, must be something wrong
                assert(0);
            }
            
            merge_facts(inputs, outputs);
        } else {
            inputs = outputs;
        }
        
        // if we have never visited the block, force the visitor to go through all statements at least once
        if (!visit_once) {
            int shortcut = shortcut_analysis(inputs, *cg_context); 
            if (shortcut == 0) return true;
        }
        
        outputs = inputs;
        // add facts for locals
        for (i=0; i<local_vars.size(); i++) {
            const Variable* v = local_vars[i];
            FactMgr::add_new_var_fact(v, outputs);
        }
        
        // revisit statements with new inputs
        cg_context->not_to_remove_rv_facts = false;
        for (i=0; i<stms.size(); i++) {
            int h = g++;
            if (h == 2585)
                BREAK_NOP;		// for debugging
            if (!stms[i]->analyze_with_edges_in(outputs, *cg_context)) {
                fail_index = i;
                return false;
            }
        }
        cg_context->not_to_remove_rv_facts = false;
        
        // set map values for the current block
        fm->set_fact_in(this, inputs);
        FactVec post_facts = outputs;
        FactMgr::update_facts_for_oos_vars(local_vars, post_facts);
        fm->set_fact_out(this, post_facts);
        fm->map_visited[this] = true;
        set_accumulated_effect(*cg_context);
        visit_once = false;
        
        // prepare for the next iteration
        prepare_for_next_iteration(outputs, rec_cg_context);
        cg_context = rec_cg_context.get_curr_cg_context();
        fm = rec_fm->get_curr_fact_mgr();
    } while (true);
    return true;
}


/**
 * Creates a new context and fact manager from current for the next iteration.
 * In addition, performs a caller-to-callee handover with outputs.
 */
void
RecursiveBlock::prepare_for_next_iteration(FactVec& outputs, RecursiveCGContext& rec_cg_context) const
{
    RecursiveFactMgr *rec_fm = get_rec_fact_mgr_for_func(func);
    CGContext *cg_context = rec_cg_context.get_curr_cg_context();
    FactMgr *fm = rec_fm->get_curr_fact_mgr();
    vector<const Block*> call_chain;
    
    // create a new context for the next iteration
    if (rec_cg_context.get_num_cg_contexts() < rec_cg_context.get_max_cg_contexts()) {
        //Effect *effect_accum = new Effect();
        Effect *effect_accum = new Effect(cg_context->get_accum_effect());
        CGContext *new_context = new CGContext(*cg_context, func, cg_context->get_effect_context(), effect_accum);
        rec_cg_context.add_cg_context(new_context);
        call_chain = new_context->call_chain;
    }
    
    // create a new fact manager for the next iteration
    if (rec_fm->get_num_fact_mgrs() < rec_fm->get_max_fact_mgrs()) {
        FactMgr* new_fm = new FactMgr(fm);
        new_fm->clear_map_visited();
        rec_fm->add_fact_mgr(call_chain, new_fm);
        fm = new_fm;
    }
    fm->caller_to_callee_handover(rec_call, outputs);
}

/**
 * Gets the statements containing the recursive call.
 */
void
RecursiveBlock::get_rec_stmts(const Statement*& rec_if, const Statement*& rec_block, const Statement*& rec_stmt)
{
    if (outermost_rec_stmt->eType == eInvoke) {
        rec_if = NULL;
        rec_block = NULL;
        rec_stmt = outermost_rec_stmt;
    } else {
        rec_if = outermost_rec_stmt;
        rec_block = dynamic_cast<const StatementIf*>(rec_if)->get_false_branch();
        rec_stmt = dynamic_cast<const Block*>(rec_block)->stms.front();
    }
}

/**
 * Once generated the sub-blocks up to the recursive calls in every function participating the recursive call cycle,
 * verify whether some statement caused the analyzer to fail during the 2nd iteration of the function body
 * (in most case, a null/dead pointer dereference would do it).
 * If so, delete the statement in which analyzer fails and all subsequent statements up to the recursive call.
 *
 * Also performs effect analysis.
 */
bool
RecursiveBlock::mutually_rec_call_post_creation_analysis(MutuallyRecursiveFunction *first_func)
{
    RecursiveBlock *first_body = dynamic_cast<RecursiveBlock *>(first_func->blocks[0]);
    RecursiveCGContext &first_rec_cgc = *get_rec_cg_context_for_func(first_func);
    RecursiveFactMgr *first_rec_fm = get_rec_fact_mgr_for_func(first_func);
    
    MutuallyRecursiveFunction *last_func = first_func->get_prev_func();
    RecursiveBlock *last_body = dynamic_cast<RecursiveBlock *>(last_func->blocks[0]);
    RecursiveCGContext &last_rec_cgc = *get_rec_cg_context_for_func(last_func);
    RecursiveFactMgr *last_rec_fm = get_rec_fact_mgr_for_func(last_func);
    FactMgr *last_fm = last_rec_fm->get_curr_fact_mgr();
    
    // set map values for the body of each function in the recursive call cycle
    MutuallyRecursiveFunction *curr_func = first_func;
    const int num_funcs = first_func->get_num_funcs();
    for (int i = first_func->get_index(); i < num_funcs; i++, curr_func = curr_func->get_next_func()) {
        RecursiveBlock *curr_body = dynamic_cast<RecursiveBlock *>(curr_func->blocks[0]);
        RecursiveCGContext &rec_cg_context = *get_rec_cg_context_for_func(curr_func);
        RecursiveFactMgr *rec_fm = get_rec_fact_mgr_for_func(curr_func);
        CGContext *cg_context = rec_cg_context.get_curr_cg_context();
        FactMgr *fm = rec_fm->get_curr_fact_mgr();
        
        fm->map_visited[curr_body] = true;
        curr_body->set_accumulated_effect(*cg_context);
        vector<const Fact*> post_facts = fm->global_facts;
        FactMgr::update_facts_for_oos_vars(curr_body->local_vars, post_facts);
        fm->remove_rv_facts(post_facts);
        fm->set_fact_out(curr_body, post_facts);
    }

    // perform DFA analysis
    int fail_index;
    bool visit_once = false;
    MutuallyRecursiveFunction *failed_func;
    FactVec outputs = last_fm->global_facts;
    last_body->prepare_for_next_function(outputs, last_rec_cgc);
    while (!mutually_rec_call_find_fixed_point(outputs, first_func, fail_index, failed_func, visit_once)) {
        RecursiveBlock *failed_body = dynamic_cast<RecursiveBlock *>(failed_func->blocks[0]);
        if (fail_index == failed_body->before_block_size)
            return false; // cannot recover from a probelm in the recursive call
        
        RecursiveCGContext &failed_rec_cgc = *get_rec_cg_context_for_func(failed_func);
        RecursiveFactMgr *failed_rec_fm = get_rec_fact_mgr_for_func(failed_func);
        failed_rec_cgc.set_curr_cg_context(failed_rec_cgc.map_cg_contexts.begin()->second);
        failed_rec_fm->set_curr_fact_mgr(failed_rec_fm->map_fact_mgrs.begin()->second);
        
        for (size_t i = fail_index; i < failed_body->before_block_size; i++) {
            failed_body->remove_stmt(failed_body->stms[i]);
            i = fail_index - 1;
            failed_body->before_block_size--;
        }
        
        // if we delete some statements, next visit must go through statements (no shortcut)
        visit_once = true;
        
        // clean up the map from previous analysis that might include facts caused by deleted statements
        first_rec_fm->rec_call_reset_map_fact_mgrs(first_body);
        outputs = first_func->first_pre_facts;
        
        // reset incoming effects
        first_rec_cgc.rec_call_reset_map_cg_contexts(first_func->first_pre_effect);

        // clean the maps map_fact_mgrs and map_cg_contexts for each function in the cycle but the first
        curr_func = first_func->get_next_func();
        for (int i = curr_func->get_index(); i < num_funcs; i++, curr_func = curr_func->get_next_func()) {
            RecursiveCGContext &rec_cg_context = *get_rec_cg_context_for_func(curr_func);
            RecursiveFactMgr *rec_fm = get_rec_fact_mgr_for_func(curr_func);
            rec_cg_context.map_cg_contexts.clear();
            rec_fm->map_fact_mgrs.clear();
        }
    }
    
    // update the maps map_fact_mgrs and map_cg_contexts for the first and last functions in the cycle
    const Statement *rec_if, *rec_block, *rec_stmt;
    first_body->get_rec_stmts(rec_if, rec_block, rec_stmt);
    last_rec_fm->update_map_fact_mgrs_for_adjacent(first_func, rec_if, rec_block, rec_stmt);
    last_rec_cgc.update_map_cg_contexts_for_adjacent(first_func, rec_if, rec_block, rec_stmt);
    
    return true;
}

/**
 * Creates a new context and fact manager from current for revisiting the next function.
 * In addition, performs a caller-to-callee handover with outputs.
 */
void
RecursiveBlock::prepare_for_next_function(FactVec& outputs, RecursiveCGContext& rec_cg_context) const
{
    MutuallyRecursiveFunction *next_func = dynamic_cast<MutuallyRecursiveFunction *>(func)->get_next_func();
    RecursiveCGContext &next_rec_cgc = *get_rec_cg_context_for_func(next_func);
    RecursiveFactMgr *next_rec_fm = get_rec_fact_mgr_for_func(next_func);
    
    CGContext *cg_context = rec_cg_context.get_curr_cg_context();
    FactMgr *next_fm = next_rec_fm->get_curr_fact_mgr();

    // create a new context for the next function
    vector<const Block*> call_chain;
    if (next_rec_cgc.get_num_cg_contexts() < next_rec_cgc.get_max_cg_contexts()) {
        Effect *effect_accum = new Effect(cg_context->get_accum_effect());
        Effect *effect_context = new Effect(cg_context->get_effect_context());
        CGContext *new_context = new CGContext(*cg_context, next_func, *effect_context, effect_accum);
        next_rec_cgc.add_cg_context(new_context);
        call_chain = new_context->call_chain;
    }
    
    // create a new fact manager for the next function
    if (next_rec_fm->get_num_fact_mgrs() < next_rec_fm->get_max_fact_mgrs()) {
        FactMgr* new_fm = new FactMgr(next_fm);
        new_fm->clear_map_visited();
        next_rec_fm->add_fact_mgr(call_chain, new_fm);
        next_fm = new_fm;
    }
    next_fm->caller_to_callee_handover(rec_call, outputs);
}

/**
 * DFA analysis for the sub-blocks up to the recursive calls in every function participating the recursive call cycle.
 *
 * params:
 *    outputs: the outputs env after generating the body of the last function in the recursive call cycle
 *    first_func: the first function in the recursive call cycle
 *    fail_index: records which statement in the body of failed_func caused analyzer to fail
 *    failed_func: records which function in the recursive call cycle caused analyzer to fail
 *    visit_once: when is true, the statements in the body of the first fucntion must be visited at least once
 */
bool
RecursiveBlock::mutually_rec_call_find_fixed_point(FactVec outputs, MutuallyRecursiveFunction *first_func, int& fail_index,
                                                   MutuallyRecursiveFunction *&failed_func, bool visit_once)
{
    static int g = 0;
    int cnt = 0;
    FactVec inputs;
    MutuallyRecursiveFunction *curr_func;
    for (curr_func = first_func; ; curr_func = curr_func->get_next_func()) {
        RecursiveBlock *curr_body = dynamic_cast<RecursiveBlock *>(curr_func->blocks[0]);
        RecursiveCGContext &rec_cg_context = *get_rec_cg_context_for_func(curr_func);
        RecursiveFactMgr *rec_fm = get_rec_fact_mgr_for_func(curr_func);
        CGContext *cg_context = rec_cg_context.get_curr_cg_context();
        FactMgr *fm = rec_fm->get_curr_fact_mgr();
        
        // if reached the maximum number of fact sets in an inclusive fact set,
        // include the inputs from the previous iteration in this one
        if (fm->map_visited[curr_body]) {
            if (cnt++ > 7) {
                // takes too many iterations to reach a fixed point, must be something wrong
                assert(0);
            }
            
            inputs = fm->map_facts_in[curr_body];
            merge_facts(inputs, outputs);
        } else {
            inputs = outputs;
        }
        
        // if we have never visited the block, force the visitor to go through all statements at least once
        if (!visit_once && curr_func->is_first()) {
            int shortcut = curr_body->shortcut_analysis(inputs, *cg_context);
            if (shortcut == 0) return true;
        }
        
        outputs = inputs;
        // add facts for locals
        for (size_t i = 0; i < curr_body->local_vars.size(); i++) {
            const Variable* v = curr_body->local_vars[i];
            FactMgr::add_new_var_fact(v, outputs);
        }
        
        // revisit statements with new inputs
        cg_context->not_to_remove_rv_facts = false;
        for (size_t i = 0; i < curr_body->stms.size(); i++) {
            int h = g++;
            if (h == 2585)
                BREAK_NOP;		// for debugging
            if (!curr_body->stms[i]->analyze_with_edges_in(outputs, *cg_context)) {
                fail_index = i;
                failed_func = curr_func;
                return false;
            }
        }
        cg_context->not_to_remove_rv_facts = false;
        
        // set map values for the current block
        fm->set_fact_in(curr_body, inputs);
        FactVec post_facts = outputs;
        FactMgr::update_facts_for_oos_vars(curr_body->local_vars, post_facts);
        fm->set_fact_out(curr_body, post_facts);
        fm->map_visited[curr_body] = true;
        curr_body->set_accumulated_effect(*cg_context);
        visit_once = false;
        
        // prepare for the next iteration
        curr_body->prepare_for_next_function(outputs, rec_cg_context);
    }
    return true;
}

/**
 * Once generated the sub-block after the recursive call,
 * verify whether some statement caused the analyzer to fail during the 2nd iteration of the function body
 * (in most case, a null/dead pointer dereference would do it).
 * If so, delete the statement in which analyzer fails and all subsequent statements.
 *
 * Also performs effect analysis.
 */
void
RecursiveBlock::immediate_rec_func_post_creation_analysis(RecursiveCGContext& rec_cg_context)
{
    RecursiveFactMgr *rec_fm = get_rec_fact_mgr_for_func(func);
    CGContext *cg_context = rec_cg_context.map_cg_contexts.rbegin()->second;
    FactMgr* fm = rec_fm->map_fact_mgrs.rbegin()->second;
    rec_cg_context.set_curr_cg_context(cg_context);
    rec_fm->set_curr_fact_mgr(fm);
    
    update_maps_for_curr_blk(rec_cg_context);
    
    // perform DFA analysis
    int fail_index;
    bool visit_once = false;
    rec_cg_context.reset_effect_accums();
    while (!immediate_rec_func_find_fixed_point(rec_cg_context, fail_index, visit_once)) {
        rec_cg_context.set_curr_cg_context(cg_context);
        rec_fm->set_curr_fact_mgr(fm);
        
        size_t i, len;
        len = stms.size();
        for (i = fail_index; i < len; i++) {
            remove_stmt(stms[i]);
            i = fail_index - 1;
            len = stms.size();
        }
        
        // if we delete some statements, next visit must go through statements (no shortcut)
        visit_once = true;
        
        // clean up the map from previous analysis that might include facts caused by deleted statements
        rec_fm->rec_func_reset_map_fact_mgrs(this);
        
        // reset incoming effects
        rec_cg_context.rec_func_reset_map_cg_contexts();
    }
    
    // make sure we add back return statement for blocks that require it and had such statement deleted
    // only do this for top-level block of a function which requires a return statement
    if (parent == 0 && func->need_return_stmt() && !must_return()) {
        CGContext *cg_context = rec_cg_context.merged_first_cg_context;
        FactMgr* fm = rec_fm->merged_first_fact_mgr;
        rec_cg_context.set_curr_cg_context(cg_context);
        rec_fm->set_curr_fact_mgr(fm);
        Statement* sr = append_return_stmt(*cg_context);
        fm->set_fact_out(this, fm->map_facts_out[sr]);
    }
}

/**
 * Updates the fact managers and the contexts in the corresponding maps for the current block.
 */
void
RecursiveBlock::update_maps_for_curr_blk(RecursiveCGContext& rec_cg_context) const
{
    RecursiveFactMgr *rec_fm = get_rec_fact_mgr_for_func(func);
    map<vector<const Block*>, CGContext*>& map_cg_contexts = rec_cg_context.map_cg_contexts;
    map<vector<const Block*>, FactMgr*>& map_fact_mgrs = rec_fm->map_fact_mgrs;
    map<vector<const Block*>, CGContext*>::iterator iter_cgc;
    map<vector<const Block*>, FactMgr*>::iterator iter_fm;
    
    for (iter_cgc = map_cg_contexts.begin(), iter_fm = map_fact_mgrs.begin();
         iter_cgc != map_cg_contexts.end() && iter_fm != map_fact_mgrs.end(); iter_cgc++, iter_fm++) {
        CGContext *cg_context = iter_cgc->second;
        FactMgr *fm = iter_fm->second;
        
        fm->map_visited[this] = true;
        set_accumulated_effect(*cg_context);
        FactMgr::update_facts_for_oos_vars(local_vars, fm->global_facts);
        fm->remove_rv_facts(fm->global_facts);
        fm->set_fact_out(this, fm->global_facts);
        
        if (iter_cgc == map_cg_contexts.begin() && iter_fm == map_fact_mgrs.begin()) { // first element
            if (rec_cg_context.merged_first_cg_context && rec_fm->merged_first_fact_mgr) {
                rec_cg_context.merged_first_cg_context->add_effect(*cg_context->get_effect_accum());
                merge_facts(rec_fm->merged_first_fact_mgr->global_facts, fm->global_facts);

            } else {
                Effect *effect_accum = new Effect(cg_context->get_accum_effect());
                rec_cg_context.merged_first_cg_context = new CGContext(*cg_context, cg_context->get_effect_context(), effect_accum);
                rec_fm->merged_first_fact_mgr = new FactMgr(fm);
            }
        }
    }
}

/**
 * DFA analysis for a sub-block after the recursive call in a recursive function
 *
 * params:
 *    rec_cg_context: code generation context for the recursive function
 *    fail_index: records which statement in this block caused analyzer to fail
 *    visit_once: when is true, the statements in this block must be visited at least once
 */
bool
RecursiveBlock::immediate_rec_func_find_fixed_point(RecursiveCGContext& rec_cg_context, int& fail_index, bool visit_once) const
{
    RecursiveFactMgr *rec_fm = get_rec_fact_mgr_for_func(func);
    map<vector<const Block*>, CGContext*>& map_cg_contexts = rec_cg_context.map_cg_contexts;
    map<vector<const Block*>, FactMgr*>& map_fact_mgrs = rec_fm->map_fact_mgrs;
    map<vector<const Block*>, CGContext*>::iterator iter_cgc;
    map<vector<const Block*>, FactMgr*>::iterator iter_fm;
    
    static int g = 0;
    int cnt = 0;
    int min_num_iterations = rec_fm->get_num_fact_mgrs();
    do {
        if (cnt++ > 7 + min_num_iterations) {
            // takes too many iterations to reach a fixed point, must be something wrong
            assert(0);
        }
        
        rec_cg_context.prepare_for_curr_iteration();
        rec_fm->prepare_for_curr_iteration();
        
        // if we have never visited the block, force the visitor to go through all statements at least once
        if (!visit_once && cnt >= min_num_iterations) {
            assert(rec_fm->get_num_fact_mgrs() == 1);
            CGContext *cg_context = rec_cg_context.map_cg_contexts.rbegin()->second;
            FactMgr* fm = rec_fm->map_fact_mgrs.rbegin()->second;
            rec_cg_context.set_curr_cg_context(cg_context);
            rec_fm->set_curr_fact_mgr(fm);
            
            int shortcut = shortcut_post_analysis(fm->global_facts, *cg_context);
            if (shortcut == 0) return true;
        }
        
        // revisit statements with new inputs
        for (size_t i=before_block_size + 1; i<stms.size(); i++) {
            for (iter_cgc = map_cg_contexts.begin(), iter_fm = map_fact_mgrs.begin();
                 iter_cgc != map_cg_contexts.end() && iter_fm != map_fact_mgrs.end(); iter_cgc++, iter_fm++) {
                CGContext *cg_context = iter_cgc->second;
                FactMgr *fm = iter_fm->second;
                rec_cg_context.set_curr_cg_context(cg_context);
                rec_fm->set_curr_fact_mgr(fm);
                
                // add facts for locals
                for (size_t j=0; j<local_vars.size(); j++) {
                    const Variable* v = local_vars[j];
                    FactMgr::add_new_var_fact(v, fm->global_facts);
                }

                int h = g++;
                if (h == 2585)
                    BREAK_NOP;		// for debugging
                if (!stms[i]->analyze_with_edges_in(fm->global_facts, *cg_context)) {
                    fail_index = i;
                    return false;
                }
            }
        }
        
        // set map values for the current block
        update_maps_for_curr_blk(rec_cg_context);
        visit_once = false;
    } while (true);
    return true;
}

/* return code:
 *    0 means we successfully take a shortcut
 *    1 means the shortcut fails due to effect conflict
 *    2 means there is no shortcut
 */
int
RecursiveBlock::shortcut_post_analysis(vector<const Fact*>& inputs, CGContext& cg_context) const
{
    FactMgr* fm = get_fact_mgr_for_func(func);
    // the output facts of control statement (break/continue/goto) has removed local facts
    // thus can not take this shortcut. (The facts we get should represent all variables
    // visible in subsequent statement)
    Statement *first_stm = stms[before_block_size + 1];
    if (same_facts(inputs, fm->map_facts_in[first_stm]) && !is_ctrl_stmt() && !contains_unfixed_goto())
    {
        //cg_context.get_effect_context().Output(cout);
        //print_facts(inputs);
        //fm->map_stm_effect[this].Output(cout);
        if (cg_context.in_conflict(fm->map_stm_effect[this])) {
            return 1;
        }
        inputs = fm->map_facts_out[this];
        cg_context.add_effect(fm->map_stm_effect[this]);
        fm->map_accum_effect[this] = *(cg_context.get_effect_accum());
        return 0;
    }
    return 2;
}

void
RecursiveBlock::mutually_rec_func_post_creation_analysis(MutuallyRecursiveFunction *first_func)
{
    
    // TODO: complete the entire analysis
    
    
    // finish the generation of each function in the cycle but the first
    MutuallyRecursiveFunction *curr_func = first_func->get_next_func();
    int num_funcs = first_func->get_num_funcs();
    for (int i = curr_func->get_index(); i < num_funcs; i++, curr_func = curr_func->get_next_func()) {
        curr_func->finish_generation();
    }
}
// **************************************************************************** <<
