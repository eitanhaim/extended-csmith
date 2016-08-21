// ****************************** ExtendedCsmith ****************************** >>
//
//  RecursiveCGContext.h
//  extended-csmith
//
//  Created by eitan mashiah on 3.8.2016.
//  Copyright © 2016 eitan mashiah. All rights reserved.
//

#ifndef RecursiveCGContext_h
#define RecursiveCGContext_h

#include <vector>
#include <map>
#include "Effect.h"

class Block;
class CGContext;
class Function;
class Effect;
class Statement;

using namespace std;


// A code generation context for recursive functions
class RecursiveCGContext
{
public:
    RecursiveCGContext(CGContext* cg_context);
    
    ~RecursiveCGContext(void);
    
    CGContext* get_curr_cg_context() const { return curr_cg_context; }
    
    void set_curr_cg_context(CGContext* cgc) { curr_cg_context = cgc; }
        
    int get_num_cg_contexts() const { return map_cg_contexts.size(); }
    
    int get_max_cg_contexts() const { return max_cg_contexts; }
    
    Function* get_func() const { return func; }
    
    void add_cg_context(CGContext* cg_context);
    
    void rec_call_reset_map_cg_contexts(const Effect& pre_effect);
    
    void rec_func_reset_map_cg_contexts();
    
    void update_map_cg_contexts(const Statement* rec_if, const Statement*rec_block, const Statement* rec_stmt);
    
    void init_merged_cg_context();
    
    void save_pre_effects();
    
    void reset_effect_accums();
    
    void prepare_for_curr_iteration();

    // maps call chains to contexts
    map<vector<const Block*>, CGContext*> map_cg_contexts;
    
    // the context constituting the merging of all the context in the map
    CGContext* merged_cg_context;
    
    // the context constituting the merging of all the first contexts in the map
    CGContext* merged_first_cg_context;

private:
    // the maximum number of contexts in this recursive context
    const int max_cg_contexts;
    
    // the context for the current call chain
    CGContext* curr_cg_context;
    
    // the current function being analyzed
    Function* const func;
    
    // maps call chains to the effect_accum of the contexts,
    // before generating the sub-block following the recursive call
    map<vector<const Block*>, Effect> map_pre_effects;
    
    // maps call chains to deleted contexts
    map<vector<const Block*>, CGContext*> map_deleted_cg_contexts;
};

#endif /* RecursiveCGContext_h */
// **************************************************************************** <<
