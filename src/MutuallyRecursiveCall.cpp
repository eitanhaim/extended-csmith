// ****************************** ExtendedCsmith ****************************** >>
//
//  MutuallyRecursiveCall.cpp
//  extended-csmith
//
//  Created by eitan mashiah on 7.8.2016.
//  Copyright © 2016 eitan mashiah. All rights reserved.
//

#include "MutuallyRecursiveCall.h"

MutuallyRecursiveCall::MutuallyRecursiveCall(Function *target, bool isBackLink, const SafeOpFlags *flags)
: FunctionInvocationUser(target, isBackLink, flags, eMutuallyRecursiveCall)
{
    // nothing to do
}

MutuallyRecursiveCall::~MutuallyRecursiveCall()
{
    // nothing to do
}

// **************************************************************************** <<
