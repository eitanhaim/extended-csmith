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

using namespace std;


// A code generation context for recursive functions
class RecursiveCGContext
{
public:
    RecursiveCGContext(CGContext* cg_context);
    
    ~RecursiveCGContext(void);
    
    CGContext* get_curr_cg_context() const { return curr_cg_context; }
    
    Function* get_func() const { return func; }
    
    void add_cg_context(CGContext* cg_context);
    
    void reset_map_cg_contexts(const Effect& pre_effect);
    
private:
    // maps call chains to contexts
    map<vector<const Block*>, CGContext*> map_cg_contexts;
    
    // the number of contexts in this recursive context
    const int num_cg_contexts;
    
    // the context for the current call chain
    CGContext* curr_cg_context;
    
    // the current function being analyzed
    Function* const func;
};

#endif /* RecursiveCGContext_h */
// **************************************************************************** <<
