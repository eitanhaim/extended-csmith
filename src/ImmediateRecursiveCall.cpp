// ****************************** ExtendedCsmith ****************************** >>
//
//  ImmediateRecursiveCall.cpp
//  extended-csmith
//
//  Created by eitan mashiah on 7.8.2016.
//  Copyright © 2016 eitan mashiah. All rights reserved.
//

#include "ImmediateRecursiveCall.h"
#include "RecursiveCGContext.h"
#include "RecursiveFactMgr.h"
#include "FactMgr.h"
#include "Function.h"
#include "Variable.h"
#include "Block.h"
#include "Error.h"

ImmediateRecursiveCall::ImmediateRecursiveCall(Function *target, bool isBackLink, const SafeOpFlags *flags)
    : FunctionInvocationUser(target, isBackLink, flags, eImmediateRecursiveCall)
{
    // nothing to do
}

ImmediateRecursiveCall::~ImmediateRecursiveCall()
{
    // nothing to do
}

bool
ImmediateRecursiveCall::build_invocation(RecursiveCGContext &rec_cg_context)
{
    CGContext& cg_context = rec_cg_context.get_curr_cg_context();
    Effect running_eff_context(cg_context.get_effect_context());
    //FactMgr* fm = get_fact_mgr(&cg_context);
    
    // generate the recursive call
    for (int i = 0; i < func->param.size(); i++) {
        Effect param_eff_accum;
        CGContext param_cg_context(cg_context, running_eff_context, &param_eff_accum);
        Variable* v = func->param[i];
        
        // to avoid too much function invocations as parameters
        Expression *p = Expression::make_random_param(param_cg_context, v->type, &v->qfer);
        ERROR_GUARD(false);
        p->check_and_set_cast(v->type);  // typecast, if needed
        param_value.push_back(p);
        
        // Update the "running effect context": the context that we must use
        // when we generate subsequent parameters within this invocation.
        running_eff_context.add_effect(param_eff_accum);
        
        // Update the total effect of this invocation, too.
        cg_context.merge_param_context(param_cg_context);
    }
    
    // perform DFA analysis after creation
    post_creation_analysis(rec_cg_context);
    
    return !failed;
}

/**
 * Once generated the sub-block before the recursive call, as well as the recursive call,
 * verify whether some statement caused the analyzer to fail during the 2nd iteration of the function body 
 * (in most case, a null/dead pointer dereference would do it).
 * If so, delete the statement in which analyzer fails and all subsequent statemets
 *
 * Also performs effect analysis.
 */
void
ImmediateRecursiveCall::post_creation_analysis(RecursiveCGContext& rec_cg_context)
{
    failed = false;
    CGContext& cg_context = rec_cg_context.get_curr_cg_context();
    RecursiveFactMgr *rec_fm = get_rec_fact_mgr_for_func(func);
    FactMgr* fm = rec_fm->get_curr_fact_mgr();
    Block* before_block = func->body;  // the sub-block before the recursive call
    
    // create a new context for the next iteration
    Effect effect_accum;
    Effect effect_context = cg_context.get_effect_context();
    effect_context.add_effect(rec_cg_context.get_pre_effect_context());
    CGContext new_context(cg_context, func, effect_context, &effect_accum);
    rec_cg_context.add_cg_context(&new_context);
    
    // create a new fact manager for the next iteration
    FactMgr* new_fm = new FactMgr(fm);
    new_fm->clear_map_visited();
    new_fm->caller_to_callee_handover(this, new_fm->global_facts);
    rec_fm->add_fact_mgr(new_context.call_chain, new_fm);
    
    // start fixed-point analysis
    /*
    int index;
    bool need_revisit = true;
    vector<const Fact*> facts_copy = fm->map_facts_in[before_block];
    // reset the accumulative effect
    cg_context.reset_effect_accum(pre_effect);
    
    while (!find_fixed_point(facts_copy, post_facts, cg_context, index, need_revisit)) {
        size_t i, len;
        len = before_block->stms.size();
        for (i = index; i < len; i++) {
            before_block->remove_stmt(before_block->stms[i]);
            i = index - 1;
            len = stms.size();
        }
        
        // if we delete some statements, next visit must go through statements (no shortcut)
        need_revisit = true;
        
        // clean up in/out map from previous analysis that might include facts caused by deleted statements
        fm->reset_stm_fact_maps(this);
        
        // reset incoming effects
        cg_context.reset_effect_accum(pre_effect);
    }
    fm->global_facts = fm->map_facts_out[this];
     */
    
    // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
    // TODO: check this out
    failed = !revisit(fm->global_facts, new_context);
    // incorporate facts from revisit
    if (!failed) {
        assert(cg_context.get_current_block());
        cg_context.add_visible_effect(*new_context.get_effect_accum(), cg_context.get_current_block());
        Effect& func_effect = func->feffect;
        func_effect.add_external_effect(*new_context.get_effect_accum(), cg_context.call_chain);
    }
    // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


}

/**
 * DFA analysis for a block:
 *
 * we must considers all kinds of blocks: block for for-loops; block for if-true and if-false; block for
 * function body; block that loops; block has jump destination insdie; block being a jump destination itself
 * (in the case of "continue" in for-loops). All of them must be taken care in this function.
 *
 * params:
 *    inputs: the inputs env before entering block
 *    cg_context: code generation context
 *    fail_index: records which statement in this block caused analyzer to fail
 *    visit_one: when is true, the statements in this block must be visited at least once
 */
bool
ImmediateRecursiveCall::find_fixed_point(vector<const Fact*> inputs, vector<const Fact*>& post_facts,
                                         CGContext& cg_context, int& fail_index, bool visit_once) const
{
    FactMgr* fm = get_fact_mgr(&cg_context);
    // include outputs from all back edges leading to this block
    size_t i;
    static int g = 0;
    vector<const CFGEdge*> edges;
    int cnt = 0;
    do {
        // if we have never visited the block, force the visitor to go through all statements at least once
        if (fm->map_visited[this]) {
            if (cnt++ > 7) {
                // takes too many iterations to reach a fixed point, must be something wrong
                assert(0);
            }
            find_edges_in(edges, false, true);
            for (i=0; i<edges.size(); i++) {
                const Statement* src = edges[i]->src;
                //assert(fm->map_visited[src]);
                merge_facts(inputs, fm->map_facts_out[src]);
            }
        }
        if (!visit_once) {
            int shortcut = shortcut_analysis(inputs, cg_context);
            if (shortcut == 0) return true;
        }
        //if (shortcut == 1) return false;
        
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
            if (!stms[i]->analyze_with_edges_in(outputs, cg_context)) {
                fail_index = i;
                return false;
            }
        }  
        fm->set_fact_in(this, inputs);
        post_facts = outputs;
        FactMgr::update_facts_for_oos_vars(local_vars, outputs);
        fm->set_fact_out(this, outputs);
        fm->map_visited[this] = true; 
        // compute accumulated effect
        set_accumulated_effect(cg_context);
        visit_once = false;
    } while (true);  
    return true;
}

// **************************************************************************** <<
