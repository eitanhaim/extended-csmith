// ****************************** ExtendedCsmith ****************************** >>
//
//  MutuallyRecursiveFunction.cpp
//  extended-csmith
//
//  Created by eitan mashiah on 3.8.2016.
//  Copyright © 2016 eitan mashiah. All rights reserved.
//

#include "MutuallyRecursiveFunction.h"
#include "DepthSpec.h"
#include "Variable.h"
#include "FactMgr.h"
#include "Error.h"

const int MutuallyRecursiveFunction::max_funcs_in_recursive_call_cycle = CGOptions::max_funcs_in_recursive_call_cycle();

/** 
 * constructor for the first function in the corresponding recursive call cycle  
 */
MutuallyRecursiveFunction::MutuallyRecursiveFunction(const std::string &name, const Type *return_type, int num_funcs)
    : Function(name, return_type, eMutuallyRecursive),
      next_func(NULL),
      first_func(this),
      num_funcs(num_funcs),
      index(0)
{
    // nothing to do
}

/** 
 * constructor for a non-first function in the corresponding recursive call cycle  
 */
MutuallyRecursiveFunction::MutuallyRecursiveFunction(const std::string &name, const Type *return_type,
                                                     MutuallyRecursiveFunction *prev_func)
    : Function(name, return_type, eMutuallyRecursive),
      first_func(prev_func->first_func),
      num_funcs(prev_func->num_funcs),
      index(prev_func->index + 1)
{
    prev_func->next_func = this;
    
    if (is_last())
        next_func = first_func;
    else
        next_func = NULL;
}

MutuallyRecursiveFunction::~MutuallyRecursiveFunction()
{
    // nothing to do
}

/** 
 * Chooses a random number representing the number of functions in a recursive call cycle. 
 */
unsigned int
MutuallyRecursiveFunction::MutuallyRecursiveFunctionProbability()
{
    vector<unsigned int> v;
    v.push_back(max_funcs_in_recursive_call_cycle - 2);
    VectorFilter filter(v, NOT_FILTER_OUT);
    filter.disable(fDefault);
    return rnd_upto(max_funcs_in_recursive_call_cycle - 1, &filter) + 2;
}

/** 
 * Generates the signature, but not the body. 
 */
MutuallyRecursiveFunction*
MutuallyRecursiveFunction::make_random_signature(const CGContext& cg_context)
{
    const Type *type = RandomReturnType();
    
    DEPTH_GUARD_BY_TYPE_RETURN(dtFunction, NULL);
    ERROR_GUARD(NULL);
    MutuallyRecursiveFunction *caller = dynamic_cast<MutuallyRecursiveFunction *>(cg_context.get_current_func());
    MutuallyRecursiveFunction *f = new MutuallyRecursiveFunction(RandomFunctionName(), type, caller);
    
    // dummy variable representing return variable, we don't care about the type, so use 0
    string rvname = f->name + "_" + "rv";
    CVQualifiers ret_qfer = CVQualifiers::random_qualifiers(type, Effect::READ, cg_context, true);
    ERROR_GUARD(NULL);
    f->rv = Variable::CreateVariable(rvname, type, NULL, &ret_qfer);
    GenerateParameterList(*f);
    add_fact_mgr(new FactMgr(f));
    if (CGOptions::inline_function() && rnd_flipcoin(InlineFunctionProb))
        f->is_inlined = true;
    
    return f;
}

// **************************************************************************** <<
