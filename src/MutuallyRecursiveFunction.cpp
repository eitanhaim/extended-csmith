// ****************************** ExtendedCsmith ****************************** >>
//
//  MutuallyRecursiveFunction.cpp
//  extended-csmith
//
//  Created by eitan mashiah on 3.8.2016.
//  Copyright © 2016 eitan mashiah. All rights reserved.
//

#include "MutuallyRecursiveFunction.h"
#include "RecursiveFactMgr.h"
#include "RecursiveCGContext.h"
#include "RecursiveBlock.h"
#include "DepthSpec.h"
#include "Variable.h"
#include "FactMgr.h"
#include "Error.h"

/** 
 * constructor for the first function in the corresponding recursive call cycle  
 */
MutuallyRecursiveFunction::MutuallyRecursiveFunction(const std::string &name, const Type *return_type, int num_funcs)
    : Function(name, return_type, eMutuallyRecursive),
      prev_func(NULL),
      next_func(NULL),
      first_func(this),
      num_funcs(num_funcs),
      is_building_before(false),
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
      prev_func(prev_func),
      first_func(prev_func->first_func),
      num_funcs(prev_func->num_funcs),
      is_building_before(false),
      index(prev_func->index + 1)
{
    prev_func->next_func = this;
    
    if (is_last()) {
        next_func = first_func;
        first_func->prev_func = this;
    } else {
        next_func = NULL;
    }
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
    v.push_back(CGOptions::max_funcs_in_recursive_call_cycle() - 2);
    VectorFilter filter(v, NOT_FILTER_OUT);
    filter.disable(fDefault);
    return rnd_upto(CGOptions::max_funcs_in_recursive_call_cycle() - 1, &filter) + 2;
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

/**
 * Generates the first sub-block of the function,
 * i.e. the sub-block before the recursive call, as well the recursive call itself.
 */
void
MutuallyRecursiveFunction::generate_first_sub_block(const CGContext &prev_context, Effect& effect_accum)
{
    if (build_state != UNBUILT) {
        cerr << "warning: ignoring attempt to regenerate func" << endl;
        return;
    }
    
    build_state = BUILDING;
    FactMgr* fm = get_fact_mgr_for_func(this);
    CGContext *cg_context = new CGContext(this, prev_context.get_effect_context(), &effect_accum);
    cg_context->extend_call_chain(prev_context);
    
    // inherit proper no-read/write directives from caller
    VariableSet no_reads, no_writes, must_reads, must_writes, frame_vars;
    prev_context.find_reachable_frame_vars(fm->global_facts, frame_vars);
    prev_context.get_external_no_reads_writes(no_reads, no_writes, frame_vars);
    RWDirective rwd(no_reads, no_writes, must_reads, must_writes);
    cg_context->rw_directive = &rwd;
    cg_context->flags = 0;
    
    // create fact manager and context suitable for this recursive function,
    // then fill in the function body
    RecursiveFactMgr* rec_fm = new RecursiveFactMgr(cg_context->call_chain, fm);
    add_recursive_fact_mgr(rec_fm);
    RecursiveCGContext* rec_cg_context = new RecursiveCGContext(cg_context);
    add_recursive_cg_context(rec_cg_context);
    body = RecursiveBlock::make_random(*rec_cg_context);
}

/**
 *
 */
void
MutuallyRecursiveFunction::finish_generation()
{
    //RecursiveCGContext &rec_cg_context = *get_rec_cg_context_for_func(this);
    //RecursiveFactMgr *rec_fm = get_rec_fact_mgr_for_func(this);
    //CGContext *cg_context = rec_cg_context.get_curr_cg_context();
    //FactMgr *fm = rec_fm->get_curr_fact_mgr();

    body->set_depth_protect(true);
    
    compute_summary();
    
    make_return_const();
    ERROR_RETURN();
    
    // Mark this function as built.
    build_state = BUILT;
    
    // TODO: check this out
    /*
    // post creation processing
    FactVec ret_facts = fm->map_facts_out[body];
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
    */
}
// **************************************************************************** <<
