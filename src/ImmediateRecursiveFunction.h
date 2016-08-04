// ****************************** ExtendedCsmith ****************************** >>
//
//  ImmediateRecursiveFunction.h
//  extended-csmith
//
//  Created by eitan mashiah on 3.8.2016.
//  Copyright © 2016 eitan mashiah. All rights reserved.
//

#ifndef ImmediateRecursiveFunction_h
#define ImmediateRecursiveFunction_h

#include <string>
#include <vector>
using namespace std;

#include "Function.h"

class ImmediateRecursiveFunction: public Function
{
public:
    ImmediateRecursiveFunction(const std::string &name, const Type *return_type);
    
    virtual ~ImmediateRecursiveFunction();
        
private:
    
};

#endif /* ImmediateRecursiveFunction_h */
// **************************************************************************** <<
