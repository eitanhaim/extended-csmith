// ****************************** ExtendedCsmith ****************************** >>
//
//  RecursiveBlock.h
//  extended-csmith
//
//  Created by eitan mashiah on 4.8.2016.
//  Copyright © 2016 eitan mashiah. All rights reserved.
//

#ifndef RecursiveBlock_h
#define RecursiveBlock_h

#include "Block.h"

class RecursiveCGContext;

// A block representing a body of a recursive function
class RecursiveBlock : public Block
{
public:
    RecursiveBlock(Block* b, int block_size);
    
    virtual ~RecursiveBlock(void);
    
    // Factory method
    static RecursiveBlock *make_random(RecursiveCGContext& rec_cg_context);
    
    //void add_back_return_facts(FactMgr* fm, std::vector<const Fact*>& facts) const;
    
private:
    
};

#endif /* RecursiveBlock_h */
// **************************************************************************** <<
