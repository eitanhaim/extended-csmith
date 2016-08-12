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

/**
 * Removes from the map all of the elements except the first one.
 * In addition, resets incoming effects in the left context.
 */
void
RecursiveCGContext::reset_map_cg_contexts(const Effect& pre_effect)
{
    map<vector<const Block*>, CGContext*>::iterator iter = map_cg_contexts.begin();
    curr_cg_context = iter->second;
    iter++;
    map_cg_contexts.erase(iter, map_cg_contexts.end());
    curr_cg_context->reset_effect_accum(pre_effect);
}

// **************************************************************************** <<
