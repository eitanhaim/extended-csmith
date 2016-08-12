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
: Block(b, block_size)
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
    
    CGContext& cg_context = *(rec_cg_context.get_curr_cg_context());
    Function *curr_func = cg_context.get_current_func();
    assert(curr_func);
    
    bool is_immediate_or_first_mutually = true;
    MutuallyRecursiveFunction *mr_func = NULL;
    if (curr_func->func_type == eMutuallyRecursive) {
        mr_func = dynamic_cast<MutuallyRecursiveFunction *>(curr_func);
        is_immediate_or_first_mutually = mr_func->is_first();
    }
    
    RecursiveBlock *b = new RecursiveBlock(cg_context.get_current_block(), CGOptions::max_block_size());
    b->func = curr_func;
    b->looping = false;
    b->in_array_loop = false;
    
    // push this block onto the variable scope stack
    curr_func->stack.push_back(b);
    curr_func->blocks.push_back(b);
    
    // record global facts at this moment so that subsequent statement
    // inside the block doesn't ruin it
    FactMgr* fm = get_fact_mgr_for_func(curr_func);
    fm->set_fact_in(b, fm->global_facts);
    Effect pre_effect = cg_context.get_accum_effect();
    
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
        Effect pre_effect = cg_context.get_accum_effect();
        
        Statement *s = Statement::make_random(cg_context);        
        if (!s || s->must_return()) {
            cg_context.reset_effect_accum(pre_effect);
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
        b->stms.push_back(Statement::make_random_recursive(cg_context, eInvoke));
    else
        b->stms.push_back(Statement::make_random_recursive(cg_context, eIfElse));
    
    if (Error::get_error() != SUCCESS) {
        curr_func->stack.pop_back();
        delete b;
        return NULL;
    }
    
    // perform DFA analysis after generating the recursive call
    if (curr_func->func_type == eImmediateRecursive)
        b->post_immediate_rec_call_creation_analysis(rec_cg_context, pre_effect);
    else
       b->post_mutually_rec_call_creation_analysis(rec_cg_context, pre_effect);

    if (Error::get_error() != SUCCESS) {
        curr_func->stack.pop_back();
        delete b;
        return NULL;
    }

    // generate the sub-block after the recursive call
    for (int i = 0; i < b->after_block_size; i++) {
        FactVec pre_facts = fm->global_facts;
        Effect pre_effect = cg_context.get_accum_effect();
        
        Statement *s = Statement::make_random(cg_context);
        if (!s) {
            cg_context.reset_effect_accum(pre_effect);
            fm->restore_facts(pre_facts);
            delete s;
            i--;
        } else {
            b->stms.push_back(s);
        }
    }
    if (Error::get_error() != SUCCESS) {
        curr_func->stack.pop_back();
        delete b;
        return NULL;
    }

    // TODO: check this func after finishing analyses
    // append nested loop if some must-read/write variables hasn't been accessed
    if (b->need_nested_loop(cg_context) && cg_context.blk_depth < CGOptions::max_blk_depth()) {
        b->append_nested_loop(cg_context);
    }
    
    // perform DFA analysis after the complete creation
    if (curr_func->func_type == eImmediateRecursive)
        b->post_immediate_rec_func_creation_analysis(rec_cg_context, pre_effect);
    else
        b->post_mutually_rec_func_creation_analysis(rec_cg_context, pre_effect);

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
RecursiveBlock::post_immediate_rec_call_creation_analysis(RecursiveCGContext& rec_cg_context, const Effect& pre_effect)
{
    RecursiveFactMgr *rec_fm = get_rec_fact_mgr_for_func(func);

    int fail_index;
    bool visit_once = false;
    while (!find_fixed_point(rec_cg_context, fail_index, visit_once)) {
        size_t i;
        for (i = fail_index; i < before_block_size; i++) {
            remove_stmt(stms[i]); // TODO: check this out
            i = fail_index - 1;
            before_block_size--;
        }
        
        // if we delete some statements, next visit must go through statements (no shortcut)
        visit_once = true;
        
        // clean up the map from previous analysis that might include facts caused by deleted statements
        rec_fm->reset_map_fact_mgrs(this);
        
        // reset incoming effects
        rec_cg_context.reset_map_cg_contexts(pre_effect);
    }
    
    //fm->global_facts = fm->map_facts_out[this];
    // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
    // TODO: check this out
    /*
    failed = !revisit(fm->global_facts, new_context);
    // incorporate facts from revisit
    if (!failed) {
        assert(cg_context.get_current_block());
        cg_context.add_visible_effect(*new_context.get_effect_accum(), cg_context.get_current_block());
        Effect& func_effect = func->feffect;
        func_effect.add_external_effect(*new_context.get_effect_accum(), cg_context.call_chain);
    }
     */
    // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
}

/**
 * DFA analysis for a body of a recursive function
 *
 * params:
 *    rec_cg_context: code generation context for the recursive function
 *    fail_index: records which statement in this block caused analyzer to fail
 *    visit_one: when is true, the statements in this block must be visited at least once
 */

bool
RecursiveBlock::find_fixed_point(RecursiveCGContext& rec_cg_context, int& fail_index, bool visit_once) const
//{return false;}
{
    RecursiveFactMgr *rec_fm = get_rec_fact_mgr_for_func(func);
    
    // contexts and fact managers for the current and previous iteration
    //CGContext *prev_cg_context = rec_cg_context.get_curr_cg_context();
    //FactMgr *prev_fm = rec_fm->get_curr_fact_mgr();
    
    create_for_next_iteration(rec_cg_context);
    CGContext *cg_context = rec_cg_context.get_curr_cg_context();
    FactMgr *fm = rec_fm->get_curr_fact_mgr();
    
    FactVec inputs = fm->global_facts;
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
            
            // TODO: recheck
            FactVec &prev_inputs = fm->map_facts_in[this];
            merge_facts(inputs, prev_inputs);
        }
        
        // if we have never visited the block, force the visitor to go through all statements at least once
        if (!visit_once) {
             // TODO: check regrading fm->map_stm_effect/map_facts_out/map_facts_out[this]
            int shortcut = shortcut_analysis(inputs, *cg_context);
            if (shortcut == 0) return true;
        }
        
        FactVec outputs = inputs;
        // add facts for locals
        for (i=0; i<local_vars.size(); i++) {
            const Variable* v = local_vars[i];
            FactMgr::add_new_var_fact(v, outputs);
        }
        
        // revisit statements with new inputs
        for (i=0; i<stms.size(); i++) {
            int h = g++;
            if (h == 2585)
                BREAK_NOP;		// for debugging
            
            // TODO: check regrading fm->map_stm_effect/map_facts_out/map_facts_out[this/...]
            if (!stms[i]->analyze_with_edges_in(outputs, *cg_context)) {
                fail_index = i;
                return false;
            }
        }
        
        // TODO: think on the above and then decide the next iter
        fm->set_fact_in(this, inputs);
        //FactMgr::update_facts_for_oos_vars(local_vars, outputs);
        fm->set_fact_out(this, outputs); // TODO: same debate
        fm->map_visited[this] = true;
        // compute accumulated effect
        set_accumulated_effect(*cg_context); // TODO: same debate
        visit_once = false;
        
        // maby to add: inputs = outputs
    } while (true);
    return true;
}


/**
 * Creates a new context and fact manager from current for the next iteration.
 */
void
RecursiveBlock::create_for_next_iteration(RecursiveCGContext& rec_cg_context) const
{
    RecursiveFactMgr *rec_fm = get_rec_fact_mgr(&rec_cg_context);
    CGContext &cg_context = *(rec_cg_context.get_curr_cg_context());
    FactMgr *fm = rec_fm->get_curr_fact_mgr();
    
    // create a new context for the next iteration
    Effect effect_accum;
    Effect effect_context = cg_context.get_effect_context();
    CGContext new_context(cg_context, func, effect_context, &effect_accum);
    rec_cg_context.add_cg_context(&new_context);
    
    // create a new fact manager for the next iteration
    FactMgr* new_fm = new FactMgr(fm);
    new_fm->clear_map_visited();
    new_fm->caller_to_callee_handover(rec_call, new_fm->global_facts);
    rec_fm->add_fact_mgr(new_context.call_chain, new_fm);
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
RecursiveBlock::post_mutually_rec_call_creation_analysis(RecursiveCGContext& rec_cg_context, const Effect& pre_effect)
{
    // TODO: complete
}

void
RecursiveBlock::post_immediate_rec_func_creation_analysis(RecursiveCGContext& rec_cg_context, const Effect& pre_effect)
{
    // TODO: complete
}

void
RecursiveBlock::post_mutually_rec_func_creation_analysis(RecursiveCGContext& rec_cg_context, const Effect& pre_effect)
{
    // TODO: complete
}
// **************************************************************************** <<
