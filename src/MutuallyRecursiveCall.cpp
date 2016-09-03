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

/** 
 * Builds parameters first, then the function body. 
 */
MutuallyRecursiveCall*
MutuallyRecursiveCall::build_invocation_and_function(CGContext &cg_context)
{
    FactMgr* caller_fm = get_fact_mgr(&cg_context);
    Effect running_eff_context(cg_context.get_effect_context());
    MutuallyRecursiveFunction* mr_func = MutuallyRecursiveFunction::make_random_signature(cg_context);
    
    vector<const Expression*> param_values;
    size_t i;
    for (i = 0; i < mr_func->param.size(); i++) {
        Effect param_eff_accum;
        CGContext param_cg_context(cg_context, running_eff_context, &param_eff_accum);
        Variable* v = mr_func->param[i];
        
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
    
    MutuallyRecursiveCall* mr_call = new MutuallyRecursiveCall(mr_func, false, NULL);
    mr_call->param_value = param_values;
    
    // hand-over from caller to callee
    FactMgr* fm = get_fact_mgr_for_func(mr_func);
    fm->global_facts = caller_fm->global_facts;
    fm->caller_to_callee_handover(mr_call, fm->global_facts);
    
    // create first sub-block of the function
    Effect *effect_accum = new Effect();
    mr_func->generate_first_sub_block(cg_context, *effect_accum);
    
    if (!mr_func->body) {
        mr_call->failed = true;
        mr_call->func = NULL;
        mr_func->remove_from_lists();
        delete mr_func;
    }
    
    return mr_call;
}

/** 
 * Builds the last call in the corresponding recursive call cycle without performing a post-analysis.
 */
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
