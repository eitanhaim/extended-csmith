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

RecursiveCGContext::RecursiveCGContext(const Function* func, CGContext* cg_context)
    : curr_cg_context(cg_context),
      func(func)
{
    map_cg_contexts[cg_context->call_chain] = cg_context;
}

// **************************************************************************** <<
