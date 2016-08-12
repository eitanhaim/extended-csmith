// ****************************** ExtendedCsmith ****************************** >>
//
//  RecursiveBlock.h
//  extended-csmith
//
//  Created by eitan mashiah on 4.8.2016.
//  Copyright © 2016 eitan mashiah. All rights reserved.
//

#ifndef RecursiveBlock_h
#define RecursiveBlock_h

#include "Block.h"
#include "FunctionInvocationUser.h"

class RecursiveCGContext;

using namespace std;

// A block representing a body of a recursive function
class RecursiveBlock : public Block
{
public:
    RecursiveBlock(Block* b, int block_size);
    
    virtual ~RecursiveBlock(void);
    
    int get_actual_block_size() { return actual_block_size; }
    
    // factory method
    static RecursiveBlock *make_random(RecursiveCGContext& rec_cg_context);
    
    void post_immediate_rec_call_creation_analysis(RecursiveCGContext& rec_cg_context, const Effect& pre_effect);
    
    void post_mutually_rec_call_creation_analysis(RecursiveCGContext& rec_cg_context, const Effect& pre_effect);

    void post_immediate_rec_func_creation_analysis(RecursiveCGContext& rec_cg_context, const Effect& pre_effect);
    
    void post_mutually_rec_func_creation_analysis(RecursiveCGContext& rec_cg_context, const Effect& pre_effect);

    void add_back_return_facts(FactMgr* fm, std::vector<const Fact*>& facts) const;
    
    bool find_fixed_point(RecursiveCGContext& rec_cg_context, int& fail_index, bool visit_once) const;
    
    void create_for_next_iteration(RecursiveCGContext& rec_cg_context) const;
    
    // the recursive call generated in this block
    FunctionInvocationUser *rec_call;
    
    // the size of the sub-block before the recursive call
    unsigned int before_block_size;
    
    // the size of the sub-block after the recursive call
    unsigned int after_block_size;
    
private:
    // the actual number of statements in this block
    int actual_block_size;
};

#endif /* RecursiveBlock_h */
// **************************************************************************** <<
