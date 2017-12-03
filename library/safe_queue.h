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

#ifndef _LIBANALYSIS_SAFE_QUEUE_H
#define _LIBANALYSIS_SAFE_QUEUE_H

#include <queue>
#include "utils.h"
#include "MPIcnfSupport.h"

using namespace std;

class safe_queue {
private:
    queue<struct message> data;
    pthread_mutex_t lock;

public:
    safe_queue();
    void push_back(struct message);
    struct message pop_front();
    unsigned long long size();
};

#endif // _LIBANALYSIS_SAFE_QUEUE_H
