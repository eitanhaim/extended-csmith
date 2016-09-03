// ****************************** ExtendedCsmith ****************************** >>
//
//  MutuallyRecursiveFunction.h
//  extended-csmith
//
//  Created by eitan mashiah on 3.8.2016.
//  Copyright © 2016 eitan mashiah. All rights reserved.
//

#ifndef MutuallyRecursiveFunction_h
#define MutuallyRecursiveFunction_h

#include "Function.h"
#include "FactMgr.h"

class Effect;

class MutuallyRecursiveFunction: public Function
{
public:
    MutuallyRecursiveFunction(const std::string &name, const Type *return_type, int num_funcs);
    
    MutuallyRecursiveFunction(const std::string &name, const Type *return_type, MutuallyRecursiveFunction *prev_func);
    
    virtual ~MutuallyRecursiveFunction();
    
    MutuallyRecursiveFunction* get_next_func() const { return next_func; }
    
    MutuallyRecursiveFunction* get_prev_func() const { return prev_func; }
    
    MutuallyRecursiveFunction* get_first_func() const { return first_func; }
    
    int get_num_funcs() const { return num_funcs; }
    
    int get_index() const { return index; }
    
    /** 
     * Returns whether this function is the first one in its recursive call cycle. 
     */
    bool is_first() const { return index == 0; }
    
    /** 
     * Returns whether this function is the last one in its recursive call cycle. 
     */
    bool is_last() const { return index == num_funcs - 1; }
    
    static unsigned int MutuallyRecursiveFunctionProbability();
    
    static MutuallyRecursiveFunction* make_random_signature(const CGContext& cg_context);
    
    void generate_first_sub_block(const CGContext &prev_context, Effect& effect_accum);

    void finish_generation();
    
    // flag indicating whether the sub-block before the recursive call is built now
    bool is_building_before;
    
    // the effect_accum of the context at the beginning of the first function creation
    Effect first_pre_effect;
    
    // the global_facts of the fact manager at the beginning of the first function creation
    FactVec first_pre_facts;
    
private:
    // the next function in the corresponding recursive call cycle
    MutuallyRecursiveFunction* next_func;
    
    // the previous function in the corresponding recursive call cycle
    MutuallyRecursiveFunction* prev_func;
    
    // the first function in the corresponding recursive call cycle
    MutuallyRecursiveFunction* first_func;
        
    // the number of functions in the corresponding recursive call cycle
    const int num_funcs;
    
    // the index of this function in its recursive cycle
    const int index;
};

#endif /* MutuallyRecursiveFunction_h */
// **************************************************************************** <<
