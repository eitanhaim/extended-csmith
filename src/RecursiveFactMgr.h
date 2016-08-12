// ****************************** ExtendedCsmith ****************************** >>
//
//  RecursiveFactMgr.h
//  extended-csmith
//
//  Created by eitan mashiah on 3.8.2016.
//  Copyright © 2016 eitan mashiah. All rights reserved.
//

#ifndef RecursiveFactMgr_h
#define RecursiveFactMgr_h

class Block;
class FactMgr;
class Function;
class Statement;

#include <vector>
#include <map>
#include "FactMgr.h"

using namespace std;


// A fact manager for recursive functions
class RecursiveFactMgr
{
public:
    RecursiveFactMgr(vector<const Block*> call_chain, FactMgr* fact_mgr);
    
    ~RecursiveFactMgr(void);
    
    FactMgr* get_curr_fact_mgr() const { return curr_fact_mgr; }
    
    int get_num_fact_mgrs() const { return num_fact_mgrs; }
    
    const Function* get_func() const { return func; }
    
    void add_fact_mgr(vector<const Block*> call_chain, FactMgr* fact_mgr);
    
    void reset_map_fact_mgrs(const Statement* stm);
    
private:
    // maps call chains to fact managers
    map<vector<const Block*>, FactMgr*> map_fact_mgrs;
    
    // the fact manager for the current call chain
    FactMgr* curr_fact_mgr;
    
    // the number of fact managers in this recursive fact manager
    const int num_fact_mgrs;
    
    // the current function being analyzed
    const Function* func;
};

#endif /* RecursiveFactMgr_h */
// **************************************************************************** <<
