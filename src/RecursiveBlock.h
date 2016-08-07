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
    
    int get_actual_block_size() { return actual_block_size; }
    
    // factory method
    static RecursiveBlock *make_random(RecursiveCGContext& rec_cg_context);
        
    void add_back_return_facts(FactMgr* fm, std::vector<const Fact*>& facts) const;
    
private:
    int actual_block_size;
};

#endif /* RecursiveBlock_h */
// **************************************************************************** <<
