// ****************************** ExtendedCsmith ****************************** >>
//
//  MutuallyRecursiveCall.h
//  extended-csmith
//
//  Created by eitan mashiah on 7.8.2016.
//  Copyright © 2016 eitan mashiah. All rights reserved.
//

#ifndef MutuallyRecursiveCall_h
#define MutuallyRecursiveCall_h

#include "FunctionInvocationUser.h"

class MutuallyRecursiveCall: public FunctionInvocationUser
{
public:
    MutuallyRecursiveCall(Function *target, bool isBackLink, const SafeOpFlags *flags);
    
    virtual ~MutuallyRecursiveCall();
    
    static MutuallyRecursiveCall* build_invocation_and_function(CGContext &cg_context);
    
    bool build_invocation(CGContext &cg_context);
    
private:
    
};

#endif /* MutuallyRecursiveCall_h */
// **************************************************************************** <<
