// ****************************** ExtendedCsmith ****************************** >>
//
//  ImmediateRecursiveCall.h
//  extended-csmith
//
//  Created by eitan mashiah on 7.8.2016.
//  Copyright © 2016 eitan mashiah. All rights reserved.
//

#ifndef ImmediateRecursiveCall_h
#define ImmediateRecursiveCall_h

#include "FunctionInvocationUser.h"


class ImmediateRecursiveCall: public FunctionInvocationUser
{
public:
    ImmediateRecursiveCall(Function *target, bool isBackLink, const SafeOpFlags *flags);
    
    virtual ~ImmediateRecursiveCall();
    
    bool build_invocation(RecursiveCGContext &rec_cg_context);
    
    bool find_fixed_point(vector<const Fact*> inputs, vector<const Fact*>& post_facts,
                          CGContext& cg_context, int& fail_index, bool visit_once) const;
    
private:
    
    void post_creation_analysis(RecursiveCGContext& rec_cg_context);
};


#endif /* ImmediateRecursiveCall_h */
// **************************************************************************** <<
