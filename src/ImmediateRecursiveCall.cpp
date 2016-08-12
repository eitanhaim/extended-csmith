// ****************************** ExtendedCsmith ****************************** >>
//
//  ImmediateRecursiveCall.cpp
//  extended-csmith
//
//  Created by eitan mashiah on 7.8.2016.
//  Copyright © 2016 eitan mashiah. All rights reserved.
//

#include "ImmediateRecursiveCall.h"
#include "Function.h"
#include "Variable.h"
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

/**
 * Builds a new immediate recursive call without performing a post-analysis.
 */
bool
ImmediateRecursiveCall::build_invocation(CGContext &cg_context)
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
