// ****************************** ExtendedCsmith ****************************** >>
//
//  ImmediateRecursiveFunction.cpp
//  extended-csmith
//
//  Created by eitan mashiah on 3.8.2016.
//  Copyright © 2016 eitan mashiah. All rights reserved.
//

#include "ImmediateRecursiveFunction.h"

ImmediateRecursiveFunction::ImmediateRecursiveFunction(const std::string &name, const Type *return_type)
    : Function(name, return_type, eImmediateRecursive)
{
    // nothing to do
}

// **************************************************************************** <<
