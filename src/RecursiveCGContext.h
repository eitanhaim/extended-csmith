// ****************************** ExtendedCsmith ****************************** >>
//
//  RecursiveCGContext.h
//  extended-csmith
//
//  Created by eitan mashiah on 3.8.2016.
//  Copyright © 2016 eitan mashiah. All rights reserved.
//

#ifndef RecursiveCGContext_h
#define RecursiveCGContext_h

class Block;
class CGContext;
class Function;

#include <vector>
#include <map>
using namespace std;


// A c generator context for recursive functions
class RecursiveCGContext
{
public:
    RecursiveCGContext(const Function* func, CGContext* cg_context);
    
private:
    // maps call chains to contexts
    map<vector<const Block*>, CGContext*> map_cg_contexts;
    
    // the context for the current call chain
    CGContext* curr_cg_context;
    
    // the current function being analyzed
    const Function* func;
};

#endif /* RecursiveCGContext_h */
// **************************************************************************** <<
