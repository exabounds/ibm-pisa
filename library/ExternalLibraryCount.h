/*******************************************************************************
 * (C) Copyright IBM Corporation 2017
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *    IBM Algorithms & Machines team
 *******************************************************************************/ 

class ExternalLibraryCount;

#ifndef LLVM_EXTERNAL_LIBRARY_COUNT__H
#define LLVM_EXTERNAL_LIBRARY_COUNT__H

#include "InstructionAnalysis.h"
#include "utils.h"
#include "JSONmanager.h"

class ExternalLibraryCount: public InstructionAnalysis {
protected:

    // Case insensitive string-compare operator
    struct caseInsensitiveOperator {
        bool operator() (const std::string & s1, const std::string & s2) const;
    };

    // String is a the function name (case insensitive), int is the counter of the number of calls.
    // We do not store information about arguments type nor argument values.
    // Case insensitive names to support FORTRAN conventions.
    map<string, unsigned long long, caseInsensitiveOperator> callCount;

public:
    ExternalLibraryCount(Module *M, int thread_id, int processor_id);
    void visit(Instruction &I);
    void JSONdump(JSONmanager *JSONwriter);
};

#endif // LLVM_EXTERNAL_LIBRARY_COUNT__H

