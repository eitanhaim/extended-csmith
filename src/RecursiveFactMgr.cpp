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

// **************************************************************************** <<
