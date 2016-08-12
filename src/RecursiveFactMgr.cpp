// ****************************** ExtendedCsmith ****************************** >>
//
//  RecursiveFactMgr.cpp
//  extended-csmith
//
//  Created by eitan mashiah on 3.8.2016.
//  Copyright © 2016 eitan mashiah. All rights reserved.
//

#include "RecursiveFactMgr.h"
#include "Block.h"
#include "FactMgr.h"
#include "Function.h"

RecursiveFactMgr::RecursiveFactMgr(vector<const Block*> call_chain, FactMgr* fact_mgr)
    : curr_fact_mgr(fact_mgr),
      func(fact_mgr->func),
      num_fact_mgrs(CGOptions::max_fact_sets_in_inclusive_fact_set())
{
    map_fact_mgrs[call_chain] = fact_mgr;
}

RecursiveFactMgr::~RecursiveFactMgr(void)
{
    // nothing to do
}

void
RecursiveFactMgr::add_fact_mgr(vector<const Block*> call_chain, FactMgr* fact_mgr)
{
    map_fact_mgrs[call_chain] = fact_mgr;
    curr_fact_mgr = fact_mgr;
}

/**
 * Removes from the map all of the elements except the first one.
 * In addition, resets input/output env of this statement and all statements included to empty,
 * in the left fact manager.
 */
void
RecursiveFactMgr::reset_map_fact_mgrs(const Statement* stm)
{
    map<vector<const Block*>, FactMgr*>::iterator iter = map_fact_mgrs.begin();
    curr_fact_mgr = iter->second;
    iter++;
    map_fact_mgrs.erase(iter, map_fact_mgrs.end());
    curr_fact_mgr->reset_stm_fact_maps(stm);
}


// **************************************************************************** <<
