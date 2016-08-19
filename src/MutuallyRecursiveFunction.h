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

class MutuallyRecursiveFunction: public Function
{
public:
    MutuallyRecursiveFunction(const std::string &name, const Type *return_type, int num_funcs);
    
    MutuallyRecursiveFunction(const std::string &name, const Type *return_type, MutuallyRecursiveFunction *prev_func);
    
    virtual ~MutuallyRecursiveFunction();
    
    MutuallyRecursiveFunction* get_next_func() const { return next_func; }
    
    MutuallyRecursiveFunction* get_first_func() const { return first_func; }
    
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

    // the maximum number of functions in a recursive call cycle
    static const int max_funcs_in_recursive_call_cycle;
    
    // flag indicating whether the sub-block before the recursive call is built now
    bool is_building_before;
    
private:
    // the previous function in the corresponding recursive call cycle
    MutuallyRecursiveFunction* next_func;
    
    // the first function in the corresponding recursive call cycle
    MutuallyRecursiveFunction* first_func;
        
    // the number of functions in the corresponding recursive call cycle
    const int num_funcs;
    
    // the index of this function in its recursive cycle
    const int index;
};

#endif /* MutuallyRecursiveFunction_h */
// **************************************************************************** <<
