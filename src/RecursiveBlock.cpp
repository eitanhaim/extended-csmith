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
    for (int i = 0; i < b->before_block_size; i++) {
        FactVec pre_facts = fm->global_facts;
        Effect pre_effect = cg_context->get_accum_effect();
        
        Statement *s = Statement::make_random(*cg_context);
        if (!s || s->must_return()) {
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
    
    // perform DFA analysis after generating the recursive call
    if (curr_func->func_type == eImmediateRecursive)
        b->immediate_rec_call_post_creation_analysis(rec_cg_context, pre_effect);
    else
        b->mutually_rec_call_post_creation_analysis(rec_cg_context, pre_effect);

    if (Error::get_error() != SUCCESS) {
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
        if (s->must_return()) {
            break;
        }
        s->update_maps();
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
        b->mutually_rec_func_post_creation_analysis(rec_cg_context);

    if (Error::get_error() != SUCCESS) {
        curr_func->stack.pop_back();
        delete b;
        return NULL;
    }
    
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
 * Once generated the sub-block before the recursive call, as well as the recursive call,
 * verify whether some statement caused the analyzer to fail during the 2nd iteration of the function body
 * (in most case, a null/dead pointer dereference would do it).
 * If so, delete the statement in which analyzer fails and all subsequent statements up to the recursive call.
 *
 * Also performs effect analysis.
 */
void
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
        if (fail_index == before_block_size) {
            Error::set_error(ERROR);
            return;
        }
        
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
}

/**
 * DFA analysis for a sub-block before the recursive call in a recursive function
 *
 * params:
 *    outputs: the outputs env after generating the current block
 *    rec_cg_context: code generation context for the recursive function
 *    fail_index: records which statement in this block caused analyzer to fail
 *    visit_one: when is true, the statements in this block must be visited at least once
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
        Effect *effect_accum = new Effect();
        //Effect effect_context = cg_context->get_effect_context();
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
 * Once generated the sub-block before the recursive call, as well as the recursive call,
 * verify whether some statement caused the analyzer to fail during the 2nd iteration of the function body
 * (in most case, a null/dead pointer dereference would do it).
 * If so, delete the statement in which analyzer fails and all subsequent statements up to the recursive call.
 *
 * Also performs effect analysis.
 */
void
RecursiveBlock::mutually_rec_call_post_creation_analysis(RecursiveCGContext& rec_cg_context, const Effect& pre_effect)
{
    // TODO: complete
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
    CGContext *cg_context = rec_cg_context.get_curr_cg_context();
    FactMgr* fm = rec_fm->get_curr_fact_mgr();
    
    vector<const Fact*> post_facts = fm->global_facts;
    update_maps_for_curr_blk(rec_cg_context);
    
    // perform DFA analysis
    int fail_index;
    bool visit_once = false;
    rec_cg_context.reset_effect_accums();
    while (!immediate_rec_func_find_fixed_point(rec_cg_context, fail_index, visit_once)) {
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
        fm->global_facts = post_facts;
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
    
    Effect eff;
    FactVec outputs;
    for (iter_cgc = map_cg_contexts.begin(), iter_fm = map_fact_mgrs.begin();
         iter_cgc != map_cg_contexts.end() && iter_fm != map_fact_mgrs.end(); iter_cgc++, iter_fm++) {
        CGContext *cg_context = iter_cgc->second;
        FactMgr *fm = iter_fm->second;
        
        fm->map_visited[this] = true;
        set_accumulated_effect(*cg_context);
        FactMgr::update_facts_for_oos_vars(local_vars, fm->global_facts);
        fm->remove_rv_facts(fm->global_facts);
        fm->set_fact_out(this, fm->global_facts);
        
        eff.add_effect(fm->map_stm_effect[this]);
        merge_facts(outputs, fm->global_facts);
    }
    
    rec_fm->merged_fact_mgr->map_visited[this] = true;
    rec_fm->merged_fact_mgr->set_fact_out(this, outputs);
    rec_fm->merged_fact_mgr->map_stm_effect[this] = eff;
}

/**
 * DFA analysis for a sub-block after the recursive call in a recursive function
 *
 * params:
 *    rec_cg_context: code generation context for the recursive function
 *    fail_index: records which statement in this block caused analyzer to fail
 *    visit_one: when is true, the statements in this block must be visited at least once
 */
bool
RecursiveBlock::immediate_rec_func_find_fixed_point(RecursiveCGContext& rec_cg_context, int& fail_index, bool visit_once) const
{
    RecursiveFactMgr *rec_fm = get_rec_fact_mgr_for_func(func);
    CGContext *cg_context = rec_cg_context.get_curr_cg_context();
    FactVec inputs = rec_fm->get_pre_facts();
    map<vector<const Block*>, CGContext*>& map_cg_contexts = rec_cg_context.map_cg_contexts;
    map<vector<const Block*>, FactMgr*>& map_fact_mgrs = rec_fm->map_fact_mgrs;
    map<vector<const Block*>, CGContext*>::iterator iter_cgc;
    map<vector<const Block*>, FactMgr*>::iterator iter_fm;
    
    size_t i;
    static int g = 0;
    int cnt = 0;
    int min_num_iterations = rec_fm->get_num_fact_mgrs();
    do {
        if (cnt++ > 7 + min_num_iterations) {
            // takes too many iterations to reach a fixed point, must be something wrong
            assert(0);
        }
        
        rec_cg_context.prepare_for_curr_iteration();
        rec_fm->prepare_for_curr_iteration(inputs);
        
        // if we have never visited the block, force the visitor to go through all statements at least once
        if (!visit_once && cnt >= min_num_iterations) {
            int shortcut = shortcut_post_analysis(inputs, *cg_context);
            if (shortcut == 0) return true;
        }
        
        FactVec outputs = inputs;
        // add facts for locals
        for (i=0; i<local_vars.size(); i++) {
            const Variable* v = local_vars[i];
            FactMgr::add_new_var_fact(v, outputs);
        }
        
        // revisit statements with new inputs
        for (i=before_block_size + 1; i<stms.size(); i++) {
            for (iter_cgc = map_cg_contexts.begin(), iter_fm = map_fact_mgrs.begin();
                 iter_cgc != map_cg_contexts.end() && iter_fm != map_fact_mgrs.end(); iter_cgc++, iter_fm++) {
                CGContext *cg_context = iter_cgc->second;
                FactMgr *fm = iter_fm->second;
                rec_cg_context.set_curr_cg_context(cg_context);
                rec_fm->set_curr_fact_mgr(fm);

                int h = g++;
                if (h == 2585)
                    BREAK_NOP;		// for debugging
                if (!stms[i]->analyze_with_edges_in(fm->global_facts, *cg_context)) {
                    fail_index = i;
                    return false;
                }
                
                // update merged_cg_context and merged_fact_mgr
                if (iter_cgc == map_cg_contexts.begin() && iter_fm == map_fact_mgrs.begin()) {
                    Effect effect_accum = cg_context->get_accum_effect();
                    Effect eff_stm = cg_context->get_effect_stm();
                    rec_cg_context.merged_cg_context->reset_effect_accum(effect_accum);
                    rec_cg_context.merged_cg_context->reset_effect_stm(eff_stm);
                    rec_fm->merged_fact_mgr->global_facts = fm->global_facts;
                } else {
                    rec_cg_context.merged_cg_context->add_effect(*cg_context->get_effect_accum());
                    merge_facts(rec_fm->merged_fact_mgr->global_facts, fm->global_facts);
                }
            }
            
            // set the map values of merged_cg_context and merged_fact_mgr
            rec_fm->merged_fact_mgr->set_fact_out(this, rec_fm->merged_fact_mgr->global_facts);
            rec_fm->merged_fact_mgr->map_accum_effect[this] = *(rec_cg_context.merged_cg_context->get_effect_accum());
            rec_fm->merged_fact_mgr->map_stm_effect[this] = rec_cg_context.merged_cg_context->get_effect_stm();
        }
        rec_cg_context.set_curr_cg_context(rec_cg_context.merged_cg_context);
        rec_fm->set_curr_fact_mgr(rec_fm->merged_fact_mgr);
        
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
RecursiveBlock::mutually_rec_func_post_creation_analysis(RecursiveCGContext& rec_cg_context)
{
    // TODO: complete
}
// **************************************************************************** <<
