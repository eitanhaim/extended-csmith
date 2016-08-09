// ****************************** ExtendedCsmith ****************************** >>
//
//  RecursiveCGContext.cpp
//  extended-csmith
//
//  Created by eitan mashiah on 3.8.2016.
//  Copyright © 2016 eitan mashiah. All rights reserved.
//

#include "RecursiveCGContext.h"
#include "Block.h"
#include "CGContext.h"
#include "Function.h"

RecursiveCGContext::RecursiveCGContext(CGContext* cg_context)
    : curr_cg_context(cg_context),
      pre_effect_context(cg_context->get_effect_context()), 
      pre_effect_accum(cg_context->get_accum_effect()),
      func(cg_context->get_current_func()),
      num_cg_contexts(CGOptions::max_fact_sets_in_inclusive_fact_set())
{
    map_cg_contexts[cg_context->call_chain] = cg_context;
}

RecursiveCGContext::~RecursiveCGContext(void)
{
    // nothing to do
}

void
RecursiveCGContext::add_cg_context(CGContext* cg_context)
{
    map_cg_contexts[cg_context->call_chain] = cg_context;
    curr_cg_context = cg_context;
}

// **************************************************************************** <<
