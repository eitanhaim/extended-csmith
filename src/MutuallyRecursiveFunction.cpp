// ****************************** ExtendedCsmith ****************************** >>
//
//  MutuallyRecursiveFunction.cpp
//  extended-csmith
//
//  Created by eitan mashiah on 3.8.2016.
//  Copyright © 2016 eitan mashiah. All rights reserved.
//

#include "MutuallyRecursiveFunction.h"
//#include "CGOptions.h"

const int MutuallyRecursiveFunction::max_funcs_in_recursive_call_cycle = CGOptions::max_funcs_in_recursive_call_cycle();

MutuallyRecursiveFunction::MutuallyRecursiveFunction(const std::string &name, const Type *return_type)
: Function(name, return_type, eMutuallyRecursive)
{
    // nothing to do
}

MutuallyRecursiveFunction::~MutuallyRecursiveFunction()
{
    // nothing to do
}

// **************************************************************************** <<
