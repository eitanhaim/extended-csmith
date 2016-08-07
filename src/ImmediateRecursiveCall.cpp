// ****************************** ExtendedCsmith ****************************** >>
//
//  ImmediateRecursiveCall.cpp
//  extended-csmith
//
//  Created by eitan mashiah on 7.8.2016.
//  Copyright © 2016 eitan mashiah. All rights reserved.
//

#include "ImmediateRecursiveCall.h"

ImmediateRecursiveCall::ImmediateRecursiveCall(Function *target, bool isBackLink, const SafeOpFlags *flags)
    : FunctionInvocationUser(target, isBackLink, flags, eImmediateRecursiveCall)
{
    // nothing to do
}

ImmediateRecursiveCall::~ImmediateRecursiveCall()
{
    // nothing to do
}

// **************************************************************************** <<
