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

#include "safe_queue.h"

safe_queue::safe_queue() {
    pthread_mutex_init(&lock, NULL);
}

void safe_queue::push_back(struct message msg) {
    pthread_mutex_lock(&lock);
    data.push(msg);
    pthread_mutex_unlock(&lock);
}

struct message safe_queue::pop_front() {
    struct message msg;

    while (size() == 0)
        sched_yield();

    pthread_mutex_lock(&lock);

    if (data.size() > 0) {
        msg = data.front();
        data.pop();
    } else 
        memset(&msg, 0, sizeof(msg));

    pthread_mutex_unlock(&lock);

    return msg;
}

unsigned long long safe_queue::size() {
    unsigned long long size = 0;

    pthread_mutex_lock(&lock);
    size = data.size();
    pthread_mutex_unlock(&lock);

    return size;
}
