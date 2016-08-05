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

RecursiveBlock *
RecursiveBlock::make_random(RecursiveCGContext& rec_cg_context)
{
    DEPTH_GUARD_BY_TYPE_RETURN(dtBlock, NULL);
    
    CGContext& cg_context = *rec_cg_context.get_curr_cg_context();
    Function *curr_func = cg_context.get_current_func();
    assert(curr_func);
    
    RecursiveBlock *b = new RecursiveBlock(cg_context.get_current_block(), CGOptions::max_block_size());
    b->func = curr_func;
    b->looping = false;
    
    // push this block onto the variable scope stack
    curr_func->stack.push_back(b);
    curr_func->blocks.push_back(b);
    
    // record global facts at this moment so that subsequent statement
    // inside the block doesn't ruin it
    FactMgr* fm = get_fact_mgr_for_func(curr_func);
    fm->set_fact_in(b, fm->global_facts);
    Effect pre_effect = cg_context.get_accum_effect();
    
    // choose a random number in the range 0...(max_block_size - 1),
    // representing the actual size of this block
    //unsigned int max = BlockProbability(*b);
    unsigned int max = 3;
    if (Error::get_error() != SUCCESS) {
        curr_func->stack.pop_back();
        delete b;
        return NULL;
    }
    unsigned int i;
    if (b->stm_id == 1)
        BREAK_NOP;			// for debugging
    for (i = 0; i <= max; ++i) {
        Statement *s = Statement::make_random(cg_context);
        // In the exhaustive mode, Statement::make_random could return NULL;
        if (!s)
            break;
        b->stms.push_back(s);
        if (s->must_return()) {
            break;
        }
    }
    
    if (Error::get_error() != SUCCESS) {
        curr_func->stack.pop_back();
        delete b;
        return NULL;
    }
    
    // append nested loop if some must-read/write variables hasn't been accessed
    if (b->need_nested_loop(cg_context) && cg_context.blk_depth < CGOptions::max_blk_depth()) {
        b->append_nested_loop(cg_context);
    }
    
    // perform DFA analysis after creation
    b->post_creation_analysis(cg_context, pre_effect);
    
    if (Error::get_error() != SUCCESS) {
        curr_func->stack.pop_back();
        delete b;
        return NULL;
    }
    
    curr_func->stack.pop_back();
    if (Error::get_error() != SUCCESS) {
        //curr_func->stack.pop_back();
        delete b;
        return NULL;
    }
    
    // ISSUE: in the exhaustive mode, do we need a return statement here
    // if the last statement is not?
    Error::set_error(SUCCESS);
    return b;
}

// **************************************************************************** <<
