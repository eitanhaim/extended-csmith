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
    
    void add_back_return_facts(FactMgr* fm, std::vector<const Fact*>& facts) const;
    
    void add_back_post_return_facts(FactMgr* fm, std::vector<const Fact*>& facts) const;
    
    void immediate_rec_call_post_creation_analysis(RecursiveCGContext& rec_cg_context, const Effect& pre_effect);
    
    void mutually_rec_call_post_creation_analysis(RecursiveCGContext& rec_cg_context, const Effect& pre_effect);

    void immediate_rec_func_post_creation_analysis(RecursiveCGContext& rec_cg_context);
    
    void mutually_rec_func_post_creation_analysis(RecursiveCGContext& rec_cg_context);
        
    bool immediate_rec_call_find_fixed_point(FactVec outputs, RecursiveCGContext& rec_cg_context, int& fail_index, bool visit_once) const;
    
    bool immediate_rec_func_find_fixed_point(RecursiveCGContext& rec_cg_context, int& fail_index, bool visit_once) const;
    
    // the size of the sub-block before the recursive call
    unsigned int before_block_size;
    
    // the size of the sub-block after the recursive call
    unsigned int after_block_size;
    
    // the recursive call generated in this block
    FunctionInvocationUser *rec_call;
    
    // the outermost statement containing the recursive call
    Statement *outermost_rec_stmt;
    
private:
    void prepare_for_next_iteration(FactVec& outputs, RecursiveCGContext& rec_cg_context) const;
    
    void get_rec_stmts(const Statement*& rec_if, const Statement*& rec_block, const Statement*& rec_stmt);
    
    void update_maps_for_curr_blk(RecursiveCGContext& rec_cg_context) const;
    
    int shortcut_post_analysis(vector<const Fact*>& inputs, CGContext& cg_context) const;
    
    // the actual number of statements in this block
    int actual_block_size;
};

#endif /* RecursiveBlock_h */
// **************************************************************************** <<
