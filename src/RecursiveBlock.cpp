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
#include "StatementIf.h"
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

static unsigned int
RecursiveBlockProbability(RecursiveBlock &rec_block)
{
    vector<unsigned int> v;
    v.push_back(rec_block.get_actual_block_size() - 1);
    VectorFilter filter(v, NOT_FILTER_OUT);
    filter.disable(fDefault);
    return rnd_upto(rec_block.get_actual_block_size(), &filter);
}

RecursiveBlock *
RecursiveBlock::make_random(RecursiveCGContext& rec_cg_context)
{
    DEPTH_GUARD_BY_TYPE_RETURN(dtBlock, NULL);
    
    CGContext& cg_context = rec_cg_context.get_curr_cg_context();
    Function *curr_func = cg_context.get_current_func();
    assert(curr_func);
    
    RecursiveBlock *b = new RecursiveBlock(cg_context.get_current_block(), CGOptions::max_block_size());
    b->func = curr_func;
    b->looping = false;
    b->in_array_loop = false;
    
    // push this block onto the variable scope stack
    curr_func->stack.push_back(b);
    curr_func->blocks.push_back(b);
    
    // record global facts at this moment so that subsequent statement
    // inside the block doesn't ruin it
    FactMgr* fm = get_fact_mgr_for_func(curr_func);
    fm->set_fact_in(b, fm->global_facts);
    //Effect pre_effect = cg_context.get_accum_effect(); // TODO: DELETE
    
    // choose the actual size of this block
    b->actual_block_size = BlockProbability(*b) + 1;
    if (Error::get_error() != SUCCESS) {
        curr_func->stack.pop_back();
        delete b;
        return NULL;
    }
    
    // choose the sizes of the sub-blocks before and after the recursive call
    unsigned int before_block_size = RecursiveBlockProbability(*b);
    if (Error::get_error() != SUCCESS) {
        curr_func->stack.pop_back();
        delete b;
        return NULL;
    }
    unsigned int after_block_size =  b->actual_block_size - before_block_size - 1;

    if (b->stm_id == 1)
        BREAK_NOP;			// for debugging
    
    // generate the sub-block before the recursive call
    for (int i = 0; i < before_block_size; i++) {
        FactVec facts_copy = fm->global_facts;
        Effect pre_effect = cg_context.get_accum_effect();

        Statement *s = Statement::make_random(cg_context);
        if (!s || s->must_return()) {
            cg_context.reset_effect_accum(pre_effect);
            fm->restore_facts(facts_copy);
            i--;
        } else {
            b->stms.push_back(s);
        }
    }
    
    if (Error::get_error() != SUCCESS) {
        curr_func->stack.pop_back();
        delete b;
        return NULL;
    }
    
    // generate a statement containing the recursive call
    if (b->contains_return())
        b->stms.push_back(Statement::make_random_recursive(rec_cg_context));
    else
        b->stms.push_back(StatementIf::make_random_recursive(rec_cg_context));
    
    if (Error::get_error() != SUCCESS) {
        curr_func->stack.pop_back();
        delete b;
        return NULL;
    }
    
    // TODO: continue!!!
    // generate the sub-block after the recursive call

    // append nested loop if some must-read/write variables hasn't been accessed
    if (b->need_nested_loop(cg_context) && cg_context.blk_depth < CGOptions::max_blk_depth()) {
        b->append_nested_loop(cg_context);
    }
    
    // perform DFA analysis after creation
    b->post_creation_analysis(cg_context, rec_cg_context.get_pre_effect_acuum());
    
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

void
RecursiveBlock::add_back_return_facts(FactMgr* fm, std::vector<const Fact*>& facts) const
{
    
}

// **************************************************************************** <<
