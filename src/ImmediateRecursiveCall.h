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
    
private:
    
};


#endif /* ImmediateRecursiveCall_h */
// **************************************************************************** <<
