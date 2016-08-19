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
class CGContext;

#include <vector>
#include <map>
#include "FactMgr.h"
#include "CGContext.h"

using namespace std;


// A fact manager for recursive functions
class RecursiveFactMgr
{
public:
    RecursiveFactMgr(vector<const Block*> call_chain, FactMgr* fact_mgr);
    
    ~RecursiveFactMgr(void);
    
    FactMgr* get_curr_fact_mgr() const { return curr_fact_mgr; }
    
    FactMgr* get_fact_mgr(CGContext* cgc) { return map_fact_mgrs[cgc->call_chain]; }
    
    void set_curr_fact_mgr(FactMgr* fm) { curr_fact_mgr = fm; }
        
    int get_num_fact_mgrs() const { return map_fact_mgrs.size(); }
    
    int get_max_fact_mgrs() const { return max_fact_mgrs; }
    
    FactVec get_pre_facts() const { return pre_facts; }
    
    const Function* get_func() const { return func; }
    
    FactVec get_global_ret_facts() const { return global_ret_facts; }
    
    void add_fact_mgr(vector<const Block*> call_chain, FactMgr* fact_mgr);
    
    void rec_call_reset_map_fact_mgrs(const Statement* stm);
    
    void rec_func_reset_map_fact_mgrs(const Statement* stm);
    
    void update_map_fact_mgrs(const Statement* rec_if, const Statement*rec_block, const Statement* rec_stmt);
    
    void init_merged_fact_mgr();
    
    void save_pre_facts();
    
    void prepare_for_curr_iteration(FactVec& inputs);
    
    // maps call chains to fact managers
    map<vector<const Block*>, FactMgr*> map_fact_mgrs;
    
    // the fact manager constituting the merging of all the fact managers in the map
    FactMgr* merged_fact_mgr;
    
private:
    // the fact set to be returned to the non-recursive function,
    // which called the current function at first
    FactVec global_ret_facts;

    // the fact manager for the current call chain
    FactMgr* curr_fact_mgr;
    
    // the maximum number of fact managers in this recursive fact manager
    const int max_fact_mgrs;
    
    // the current function being analyzed
    const Function* func;
    
    // maps call chains to the global_facts of the fact managers,
    // before generating the sub-block following the recursive call
    map<vector<const Block*>, FactVec> map_pre_facts;
    
    // the global facts of merged_fact_mgr,
    // before generating the sub-block following the recursive call
    FactVec pre_facts;

        // maps call chains to fact managers
    map<vector<const Block*>, FactMgr*> map_deleted_fact_mgrs;
};

#endif /* RecursiveFactMgr_h */
// **************************************************************************** <<
