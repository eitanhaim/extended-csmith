// ****************************** ExtendedCsmith ****************************** >>
//
//  MutuallyRecursiveCall.cpp
//  extended-csmith
//
//  Created by eitan mashiah on 7.8.2016.
//  Copyright © 2016 eitan mashiah. All rights reserved.
//

#include "MutuallyRecursiveCall.h"
#include "MutuallyRecursiveFunction.h"
#include "Function.h"
#include "Block.h"
#include "Variable.h"
#include "FactMgr.h"
#include "Error.h"

MutuallyRecursiveCall::MutuallyRecursiveCall(Function *target, bool isBackLink, const SafeOpFlags *flags)
    : FunctionInvocationUser(target, isBackLink, flags, eMutuallyRecursiveCall)
{
    // nothing to do
}

MutuallyRecursiveCall::~MutuallyRecursiveCall()
{
    // nothing to do
}

/** Builds parameters first, then the function body. */
MutuallyRecursiveCall*
MutuallyRecursiveCall::build_invocation_and_function(CGContext &cg_context)
{
    FactMgr* caller_fm = get_fact_mgr(&cg_context);
    Effect running_eff_context(cg_context.get_effect_context());
    Function* func = MutuallyRecursiveFunction::make_random_signature(cg_context);
    
    vector<const Expression*> param_values;
    size_t i;
    for (i = 0; i < func->param.size(); i++) {
        Effect param_eff_accum;
        CGContext param_cg_context(cg_context, running_eff_context, &param_eff_accum);
        Variable* v = func->param[i];
        
        // to avoid too much function invocations as parameters
        Expression *p = Expression::make_random_param(param_cg_context, v->type, &v->qfer);
        p->check_and_set_cast(v->type);  // typecast, if needed
        param_values.push_back(p);
        
        // Update the "running effect context": the context that we must use
        // when we generate subsequent parameters within this invocation.
        running_eff_context.add_effect(param_eff_accum);
        
        // Update the total effect of this invocation, too.
        cg_context.merge_param_context(param_cg_context);
    }
    
    MutuallyRecursiveCall* mr_call = new MutuallyRecursiveCall(func, false, NULL);
    mr_call->param_value = param_values;
    
    // hand-over from caller to callee
    FactMgr* fm = get_fact_mgr_for_func(func);
    fm->global_facts = caller_fm->global_facts;
    fm->caller_to_callee_handover(mr_call, fm->global_facts);
    
    // create function body
    Effect effect_accum;
    func->generate_body_with_known_params(cg_context, effect_accum);
    
    // post creation processing
    FactVec ret_facts = fm->map_facts_out[func->body];
    func->body->add_back_return_facts(fm, ret_facts);
    mr_call->save_return_fact(ret_facts);
    
    // remove facts related to passing parameters
    //FactMgr::update_facts_for_oos_vars(func->param, fm->global_facts);
    fm->setup_in_out_maps(true);
    // hand-over from callee to caller: points-to facts
    renew_facts(caller_fm->global_facts, ret_facts);
    
    // hand-over from callee to caller: effects
    func->accum_eff_context.add_external_effect(cg_context.get_effect_context());
    Effect& func_effect = func->feffect;
    func_effect.add_external_effect(effect_accum, cg_context.call_chain);
    cg_context.add_visible_effect(effect_accum, cg_context.get_current_block());
    
    // hand-over from callee to caller: new global variables
    Function* caller_func = cg_context.get_current_func();
    caller_func->new_globals.insert(caller_func->new_globals.end(), func->new_globals.begin(), func->new_globals.end());
    // include facts for globals just created
    for (i=0; i<func->new_globals.size(); i++) {
        const Variable* var = func->new_globals[i];
        caller_fm->add_new_var_fact_and_update_inout_maps(NULL, var);
    }
    
    func->visited_cnt = 1;
    return mr_call;
}

/** Builds the last call in the corresponding recursive call cycle without performing a post-analysis. */
bool
MutuallyRecursiveCall::build_invocation(CGContext &cg_context)
{
    Effect running_eff_context(cg_context.get_effect_context());
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
    
    return true;
}


// **************************************************************************** <<
