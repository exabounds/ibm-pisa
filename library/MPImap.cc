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

#include "MPImap.h"

/*
    This class allows printing a trace of MPI calls in DIMEMAS format. 
    The trace is the communication-compute graph of an MPI application.
    It only works using the decoupled version of PISA (passDecoupled).
    It supports only MPI implementations (no OpenMP).
*/

MPImap::MPImap(Module *M, 
               pthread_mutex_t *print_lock, 
               int thread_id,
               int processor_id, 
               MPIdb *DB, 
               pthread_mutex_t *db_lock,
               std::unique_ptr<InstructionMix> *wkld_mix, 
               int _accMode) : InstructionAnalysis(M, thread_id, processor_id),
  ls_lock(print_lock), db(DB), db_lock(db_lock),
  mix(wkld_mix), async_send(), async_recv(),
  previousCount(0), index(0), inFunction(0), mode(0), tmpDelta(0), accMode(_accMode) {}

void MPImap::printHeader() {
    pthread_mutex_lock(this->ls_lock);
    errs() << "thread_id,processor_id,current_index,type,subtype,completion_status,num_inst_diff,local_index_recv,other_id,other_index\n"; 
    pthread_mutex_unlock(this->ls_lock);
}

void MPImap::extract_MPI_Init(MPIcall *update) {
    pthread_mutex_lock(this->ls_lock);
        errs() << "dim:1:"  << processor_id << ":" << thread_id << ":" << update->dif << "\n";
        errs() << "dim:20:" << processor_id << ":" << thread_id << ":" << "50000003:31\n";
        errs() << "dim:10:" << processor_id << ":" << thread_id << ":0:1:0:0:0:0" << "\n";
        errs() << "dim:20:" << processor_id << ":" << thread_id << ":" << "50000003:0\n";
    pthread_mutex_unlock(this->ls_lock);
}

void MPImap::extract_MPI_Finalize(MPIcall *update) {
    pthread_mutex_lock(this->ls_lock);
        errs() << "dim:1:"  << processor_id << ":" << thread_id << ":" << update->dif << "\n";
        errs() << "dim:20:" << processor_id << ":" << thread_id << ":" << "50000003:32\n";
        errs() << "dim:0:" << processor_id << ":" << thread_id << "\n";
        errs() << "dim:20:" << processor_id << ":" << thread_id << ":" << "50000003:0\n";
    pthread_mutex_unlock(this->ls_lock);
}

void MPImap::extract_MPI_Send(MPIcall *update, struct mpi_call_notification_msg *msg) {
    update->type = PUT_SEND_INFO;
    update->id_s = processor_id; 
    update->id_r = msg->other;
    update->tag = msg->tag;
    update->comm = msg->comm;
    update->msgSize = msg->size * msg->datatype;
}

void MPImap::extract_MPI_Sendrecv(MPIcall *update, MPIcall *update2, struct mpi_call_notification_msg *msg) {
    update->type = PUT_SEND_INFO;
    update->id_s = processor_id;
    update->id_r = msg->other; 
    update->tag = msg->tag;

    update2->type = PUT_RECV_INFO;
    update2->id_s = msg->other2;
    update2->id_r = processor_id;
    update2->tag = msg->tag2;
    update2->comm = msg->comm;

    update->comm = update2->comm;
}

void MPImap::extract_MPI_Recv(MPIcall *update, struct mpi_call_notification_msg *msg) {
    update->type = PUT_RECV_INFO;
    update->id_s = msg->other; 
    update->id_r = processor_id;
    update->tag = msg->tag; 
    update->comm = msg->comm;
    update->msgSize = msg->size * msg->datatype;
}

void MPImap::extract_MPI_Wait(MPIcall *update, char *completion, struct mpi_call_notification_msg *msg) {
    assert(msg);
    
    void *real_addr = msg->req_data.req_addr;
    unsigned long long current_index = update->inst;
    unsigned long long instructions = update->dif;
    list<MPIinfo>::iterator it;

    // Check if the MPI_Test is on an MPI_ISend call
    bool found = false;
    for(it = async_send.begin(); it != async_send.end(); it++) {
        if (it->real_addr == real_addr) {
            found = true;
            break;
        }
    }

    // If the MPI_Test is performed on an MPI_ISend call
    if (found == true) {
        unsigned long long index = it->info.inst;
        unsigned long long dif = it->info.dif;

        pthread_mutex_lock(this->ls_lock);
        if (!completion) {
            /* Non-Dimemas output */
            // errs() << thread_id << "," << processor_id << "," << update->inst << ",w,,," << update->dif << ",,," << index << "\n";
            if (this->inFunction >= 1) {
                errs() << "dim:1:"  << processor_id << ":" << thread_id << ":" << instructions << "\n";
                errs() << "dim:20:" << processor_id << ":" << thread_id << ":" << "50000001:5\n";
                errs() << "dim:0:" << processor_id << ":" << thread_id << "\n";
                errs() << "dim:20:" << processor_id << ":" << thread_id << ":" << "50000001:0\n";
            }
            async_send.erase(it);
        } else {
            /* Unsuccessful MPI_Test on an MPI_Isend call */
            if (!strcmp(completion,"f")) {
                /* Non-Dimemas output */
                // errs() << thread_id << "," << processor_id << "," << update->inst << ",t,s,f," << update->dif << ",,," << index << "\n";
                if (this->inFunction >= 1) {
                    errs() << "dim:1:"  << processor_id << ":" << thread_id << ":" << instructions << "\n";
                    errs() << "dim:20:" << processor_id << ":" << thread_id << ":" << "50000003:39\n";
                    errs() << "dim:20:" << processor_id << ":" << thread_id << ":" << "50000003:0\n";
                }
            } else {
                /* Successful MPI_Test on an MPI_Isend call */
                /* Non-Dimemas output */
                // errs() << thread_id << "," << processor_id << "," << update->inst << ",t,s,t," << update->dif << ",,," << index << "\n";
                if (this->inFunction >= 1) {
                    errs() << "dim:1:"  << processor_id << ":" << thread_id << ":" << instructions << "\n";
                    // How to signal a succesful MPI_Test on an MPI_Isend call - MPI_Test or MPI_Wait
                    errs() << "dim:20:" << processor_id << ":" << thread_id << ":" << "50000003:39\n";
                    errs() << "dim:20:" << processor_id << ":" << thread_id << ":" << "50000003:0\n";
                }
                async_send.erase(it);
            }
        }
        pthread_mutex_unlock(this->ls_lock);
        return;
    // If the MPI_Test is performed on an MPI_IRecv call
    } else {

        // Check if the MPI_Test is on an MPI_IRecv
        for(it = async_recv.begin(); it != async_recv.end(); it++) {
                if (it->real_addr == real_addr) break;
        }

        // In principle, this should not happen. It means that an MPI_Test has 
        // been performed on a non-existing request address.
        assert(it!=async_recv.end());

        unsigned long long index = it->info.inst;
        unsigned long long dif = it->info.dif;

        // FIXME: remove this assert! It should be used only when the completion field is TRUE
        // assert(msg->source>=0);
        // memcpy(update, &it->info, sizeof(MPIcall));
        // if (update->id_s == -1) update->id_s = msg->source;

        pthread_mutex_lock(this->ls_lock);
        if (!completion) {
            // assert(msg->source>=0);
            memcpy(update, &it->info, sizeof(MPIcall));
            update->dif = instructions;
            // if (update->id_s == -1) update->id_s = msg->source;

            /* Non-Dimemas output */
            // errs() << thread_id << "," << processor_id << "," << update->inst << ",w,,," << update->dif << ",,," << index << "\n";
            if (this->inFunction >= 1) {
                errs() << "dim:1:"  << processor_id << ":" << thread_id << ":" << instructions << "\n";
                errs() << "dim:20:" << processor_id << ":" << thread_id << ":" << "50000001:5\n";
                errs() << "dim:3:" << processor_id << ":" << thread_id << ":" << update->id_s << ":-1:" << update->msgSize << ":" << update->tag << ":" << update->comm << ":" << "2" << "\n";
                errs() << "dim:20:" << processor_id << ":" << thread_id << ":" << "50000001:0\n";
            }
                async_recv.erase(it);
                pthread_mutex_unlock(this->ls_lock);
                return;
        } else {
            if (!strcmp(completion,"f")) {
                memcpy(update, &it->info, sizeof(MPIcall));
                update->dif = instructions;
                /* Non-Dimemas output */
                // errs() << thread_id << "," << processor_id << "," << update->inst << ",t,r,f," << update->dif << ",,," << index << "\n";
                if (this->inFunction >= 1) {
                    errs() << "dim:1:"  << processor_id << ":" << thread_id << ":" << instructions << "\n";
                    errs() << "dim:20:" << processor_id << ":" << thread_id << ":" << "50000003:39\n";
                    errs() << "dim:20:" << processor_id << ":" << thread_id << ":" << "50000003:0\n";
                }
                pthread_mutex_unlock(this->ls_lock);
                return;
            }
                }
        pthread_mutex_unlock(this->ls_lock);

        // This should not happen unless MPI_STATUS_IGNORE is used.
        assert(msg->req_data.source>=0);
        memcpy(update, &it->info, sizeof(MPIcall));
        if (update->id_s == -1) 
            update->id_s = msg->req_data.source;
        update->dif = instructions;

        pthread_mutex_lock(this->db_lock);
        list<MPIcall>::iterator iter = db->s_call->begin();
        list<MPIcall>::iterator iter_end = db->s_call->end();

        unsigned long long index_isend = 0;
        int i = 0;
        for (; iter != iter_end; iter++, i++) {
            MPIcall it_addr = *iter;
            if (items_are_equal(&it_addr,update)) {
                index_isend = it_addr.inst;
                db->s_call->erase(iter);
                break;
            }
        }
        pthread_mutex_unlock(this->db_lock);

        pthread_mutex_lock(this->ls_lock);
        /* Non-Dimemas output */
        // errs() << thread_id << "," << processor_id << "," << current_index << ",t,r,t," << instructions << "," << index << "," << msg->source << "," << index_isend <<  "\n";
        if (this->inFunction >= 1) {
            errs() << "dim:1:"  << processor_id << ":" << thread_id << ":" << instructions << "\n";
            errs() << "dim:20:" << processor_id << ":" << thread_id << ":" << "50000003:39\n";
            errs() << "dim:3:"  << processor_id << ":" << thread_id << ":" << update->id_s << ":-1:" << update->msgSize << ":" << update->tag << ":" << update->comm << ":" << "2" << "\n";
            errs() << "dim:20:" << processor_id << ":" << thread_id << ":" << "50000003:0\n";
        }
        async_recv.erase(it);
        pthread_mutex_unlock(this->ls_lock);

        return;
    }
        
    fprintf(stderr, "MPI_Wait or MPI_Test does not match any MPI_Isend/Irecv.\n");
    assert(false);
    //exit(EXIT_FAILURE);
}

void MPImap::extract_MPI_Bcast(MPIcall *update, struct mpi_call_notification_msg *msg) {
    // Sender of the broadcast is the root node
    update->id_s = msg->other;
    update->id_r = processor_id;

    if (update->id_s == update->id_r) {
        // This is the root node
        update->type = PUT_SEND_INFO;
    } else {
        update->type = PUT_RECV_INFO;
    }

    update->comm = msg->comm;
}

void MPImap::extract_MPI_Reduce(MPIcall *update, struct mpi_call_notification_msg *msg) {
    // Root is the receiver of the reduce update
    update->id_s = processor_id;
    update->id_r = msg->other;
    update->type = PUT_SEND_INFO;
    update->comm = msg->comm;
}

void MPImap::extract_MPI_Allreduce(MPIcall *update, struct mpi_call_notification_msg *msg) {
    update->id_s = processor_id;
    update->type = PUT_SEND_INFO;
        update->msgSize = msg->size * msg->datatype;
    update->comm = msg->comm;

    pthread_mutex_lock(this->ls_lock);
    if (this->inFunction >= 1) {
        /* TODO This format extracted from an Extrae trace: why do we need te 0: records? */
        errs() << "dim:20:" << processor_id << ":" << thread_id << ":" << "50000002:10\n";
        errs() << "dim:10:" << processor_id << ":" << thread_id << ":11:" << update->comm << ":0:0:" << update->msgSize << ":" << msg->datatype << "\n";
        errs() << "dim:0:" << processor_id << ":" << thread_id << "\n";
        errs() << "dim:20:" << processor_id << ":" << thread_id << ":" << "50000002:0\n";
    }
    pthread_mutex_unlock(this->ls_lock);

    /*
    update->processor_id = processor_id;
    update->comm = msg->comm;
    // update->inst = index from the visit function
    update->comm_size = msg->comm_size;
    update->flag = 0;
    */
}

void MPImap::extract_MPI_Scatter(MPIcall *update, struct mpi_call_notification_msg *msg) {
    // Root is the sender
    // - all PUT_RECV_INFO to root (with second)
    update->id_s = msg->other;
    update->id_r = processor_id;
    update->type = PUT_RECV_INFO;
    update->comm = msg->comm;
}

void MPImap::extract_MPI_Isend(MPIcall *update, struct mpi_call_notification_msg *msg) {
    update->type = PUT_SEND_INFO;
    update->id_s = processor_id; 
    update->id_r = msg->other;
    update->tag = msg->tag;
    update->comm = msg->comm;
    update->msgSize = msg->size * msg->datatype;

    // Extract real address of the request variable.
    // This will be mapped inside the corresponding hash in order to
    // identify the corresponding MPIcall structure when the MPI_Wait
    // call will be issued.
    void *addr_req = msg->req_data.req_addr;

    MPIinfo info;
    info.real_addr = addr_req;
    memcpy(&info.info, update, sizeof(MPIcall));
    async_send.push_back(info);

    pthread_mutex_lock(this->ls_lock);
    /* Non-Dimemas output */
    // errs() << thread_id << "," << processor_id << "," << update->inst << ",s,,," << update->dif << "\n";

    /* DIMEMAS trace */
    if (this->inFunction >= 1) {
        errs() << "dim:1:" << processor_id << ":" << thread_id << ":" << update->dif << "\n";
        errs() << "dim:20:" << processor_id << ":" << thread_id << ":" << "50000001:3\n";
        errs() << "dim:2:" << processor_id << ":" << thread_id << ":" << update->id_r << ":-1:" << update->msgSize << ":" << update->tag << ":" << update->comm << ":" << "2" << "\n";
        errs() << "dim:20:" << processor_id << ":" << thread_id << ":" << "50000001:0\n";
    }
    pthread_mutex_unlock(this->ls_lock);
}

void MPImap::extract_MPI_Irecv(MPIcall *update, struct mpi_call_notification_msg *msg) {
    update->type = PUT_RECV_INFO;
    update->id_s = msg->other; 
    update->id_r = processor_id;
    update->tag = msg->tag; 
    update->comm = msg->comm;
    update->msgSize = msg->size * msg->datatype;

    // Extract real address of the request variable.
    // This will be mapped inside the corresponding hash in order to
    // identify the corresponding MPIcall structure when the MPI_Wait
    // call will be issued.
    void *addr_req = msg->req_data.req_addr;

    MPIinfo info;
    info.real_addr = addr_req;
    memcpy(&info.info, update, sizeof(MPIcall));
    async_recv.push_back(info);

    pthread_mutex_lock(this->ls_lock);
    /* Non-Dimemas output */
    // errs() << thread_id << "," << processor_id << "," << update->inst << ",r,,," << update->dif << "\n";

    if (this->inFunction >= 1) {
        errs() << "dim:1:" << processor_id << ":" << thread_id << ":" << update->dif << "\n";
        errs() << "dim:20:" << processor_id << ":" << thread_id << ":" << "50000001:4\n";
        errs() << "dim:3:" << processor_id << ":" << thread_id << ":" << update->id_s << ":-1:" << update->msgSize << ":" << update->tag << ":" << update->comm << ":" << "1" << "\n";
        errs() << "dim:20:" << processor_id << ":" << thread_id << ":" << "50000001:0\n";
    }
    
    pthread_mutex_unlock(this->ls_lock);
}

void MPImap::extract_info(string call_name, MPIcall *update, MPIcall *update2, struct mpi_call_notification_msg *msg) {
    if (mpiMatchName(call_name, "MPI_Send")) {
        extract_MPI_Send(update, msg);
    } else if (mpiMatchName(call_name, "MPI_Recv")) {
        extract_MPI_Recv(update, msg);
    } else if (mpiMatchName(call_name, "MPI_Isend")) {
        if (mode == 1) {
            if (accMode == 1) { tmpDelta += update->dif; }
            update->dif = tmpDelta;
            tmpDelta = 0;
        }
        extract_MPI_Isend(update, msg);
    } else if (mpiMatchName(call_name, "MPI_Irecv")) {
            if (mode == 1) {
                if (accMode == 1) { tmpDelta += update->dif; }
                update->dif = tmpDelta;
                tmpDelta = 0;
            }
        extract_MPI_Irecv(update, msg);
    } else if (mpiMatchName(call_name, "MPI_Wait")) { 
        extract_MPI_Wait(update, NULL, msg);
    } else if (mpiMatchName(call_name, "MPI_Test")) {
        if (msg->flag) {
            if (mode == 1 && accMode == 1) { tmpDelta += update->dif; }
                    else if (mode == 0) { tmpDelta = update->dif; }
                    mode = 0;
            update->dif = tmpDelta;
            extract_MPI_Wait(update, "t", msg);
        } else {
            if (mode == 0) { mode = 1; tmpDelta = update->dif; }
            else if (accMode == 1) { tmpDelta += update->dif; }
            // extract_MPI_Wait(update, "f", msg);
        }
    } else if (mpiMatchName(call_name, "MPI_Bcast")) {
        extract_MPI_Bcast(update, msg);
    } else if (mpiMatchName(call_name, "MPI_Reduce")) {
        extract_MPI_Reduce(update, msg);
    } else if (mpiMatchName(call_name, "MPI_Gather")) {
        extract_MPI_Reduce(update, msg);
    } else if (mpiMatchName(call_name, "MPI_Gatherv")) {
        extract_MPI_Reduce(update, msg);
    } else if (mpiMatchName(call_name, "MPI_Allreduce")) {
        extract_MPI_Allreduce(update, msg);
    } else if (mpiMatchName(call_name, "MPI_Allgather")) {
        extract_MPI_Allreduce(update, msg);
    } else if (mpiMatchName(call_name, "MPI_Allgatherv")) {
        extract_MPI_Allreduce(update, msg);
    } else if (mpiMatchName(call_name, "MPI_Alltoall")) {
        extract_MPI_Allreduce(update, msg);
    } else if (mpiMatchName(call_name, "MPI_Alltoallv")) {
        extract_MPI_Allreduce(update, msg);
    } else if (mpiMatchName(call_name, "MPI_Scatter")) {
        extract_MPI_Scatter(update, msg);
    } else if (mpiMatchName(call_name, "MPI_Scatterv")) {
        extract_MPI_Scatter(update, msg);
    } else if (mpiMatchName(call_name, "MPI_Init")) {
        extract_MPI_Init(update);
    } else if (mpiMatchName(call_name, "MPI_Finalize")) {
        extract_MPI_Finalize(update);
    }

    if (mpiMatchName(call_name, "MPI_Sendrecv") || mpiMatchName(call_name, "MPI_Sendrecv_replace"))
        extract_MPI_Sendrecv(update, update2, msg);
}

void MPImap::send_msg_to_db(MPIcall *msg) {
    // pthread_mutex_lock(this->ls_lock);
    // fprintf(stderr, "PUT t=%d id_s=%d id_r=%d\n", msg->type, msg->id_s, msg->id_r);
    // pthread_mutex_unlock(this->ls_lock);

    pthread_mutex_lock(this->db_lock);

    if (msg->type == PUT_SEND_INFO)
        db->s_call->push_back(*msg);
    else
        db->r_call->push_back(*msg);

    pthread_mutex_unlock(this->db_lock);
}

int MPImap::items_are_equal(MPIcall *a, MPIcall *b) {
    //fprintf(stderr, "ITEMS EQUALITY CHECK a_id_s=%d a_id_r=%d a_tag=%d a_comm=%d b_id_s=%d b_id_r=%d b_tag=%d b_comm=%d", 
    //  a->id_s, a->id_r, a->tag, a->comm, b->id_s, b->id_r, b->tag, b->comm);

    if (a->id_s != b->id_s && b->id_s != MPI_ANY_SOURCE && a->id_s != MPI_ANY_SOURCE)
        return 0;

    if (a->id_r != b->id_r)
        return 0;

    if (a->tag != b->tag && a->tag != MPI_ANY_TAG && b->tag != MPI_ANY_TAG)
        return 0;

    if (a->comm != b->comm)
        return 0;

    if (b->id_s == MPI_ANY_SOURCE) {
            b->id_s = a->id_s;
    }

    return 1;
}

void MPImap::recv_msg_from_db(MPIcall *query) {
    pthread_mutex_lock(this->db_lock);

    list<MPIcall>::iterator it = db->r_call->begin();
    list<MPIcall>::iterator it_end = db->r_call->end();

    if (query->type == GET_SEND_INFO) {
        it = db->s_call->begin();
        it_end = db->s_call->end();
    }

    int i = 0;
    for (; it != it_end; it++, i++) {
    // fprintf(stderr, "type = %d %d\n", query->type, i);
        MPIcall it_addr = *it;
    // fprintf(stderr, "iterator = %p\n", (void *)&(*it));
        if (items_are_equal(&it_addr, query)) {
            // Retrieve information.
            query->inst =  it_addr.inst; 
            query->dif = it_addr.dif;

            // Delete from db since it is not anymore useful
            if (query->type == GET_SEND_INFO)
                db->s_call->erase(it);
            else
                db->r_call->erase(it);

            pthread_mutex_unlock(this->db_lock);

            return;
        }
    }

    // Set type to INFO_NOT_FOUND in order to notify the upper function
    // that the query was not successful.
    query->type = INFO_NOT_FOUND;
    pthread_mutex_unlock(this->db_lock);
}

void MPImap::update_MPI_Isend(MPIcall *update) {
    // Send message to DB
    send_msg_to_db(update);
}

void MPImap::update_MPI_Irecv(MPIcall *update) {
    // Send message to DB
    // send_msg_to_db(update);
}

void MPImap::update_MPI_Init(MPIcall *update) {
    return;
}

void MPImap::update_MPI_Finalize(MPIcall *update) {
    return;
}

void MPImap::send_query_and_update(MPIcall *update) {
    int print_query_once = 0;
    // Send query
    while (1) {
        MPIcall msg_recv;
        memcpy(&msg_recv, update, sizeof(MPIcall));
        recv_msg_from_db(&msg_recv);
        if (msg_recv.type != INFO_NOT_FOUND) {
            // Server transmited a proper response.
            pthread_mutex_lock(this->ls_lock);
            // fprintf(stderr, "QUERY t=%d id_s=%d id_r=%d\n", update->type, update->id_s, update->id_r);
            // fprintf(stderr, "FOUND t=%d id_s=%d id_r=%d\n", msg_recv.type, msg_recv.id_s, msg_recv.id_r);
            
            /* Non-Dimemas output */
            // errs() << thread_id << "," << processor_id << ",";
            if (update->type == GET_SEND_INFO) {
                /* Non-Dimemas output */
                // errs() << update->inst << ",r,,," << update->dif << ",," << update->id_s << ",,\n";

                /* DIMEMAS trace */
                if (this->inFunction >= 1) {
                    errs() << "dim:1:" << processor_id << ":" << thread_id << ":" << update->dif << "\n";
                    errs() << "dim:20:" << processor_id << ":" << thread_id << ":" << "50000001:2\n"; 
                    errs() << "dim:3:" << processor_id << ":" << thread_id << ":" << update->id_s << ":-1:" << update->msgSize << ":" << update->tag << ":" << update->comm << ":" << "0" << "\n"; 
                    errs() << "dim:20:" << processor_id << ":" << thread_id << ":" << "50000001:0\n";
                }
            }
            else if (update->type == GET_RECV_INFO) {
                /* Non-Dimemas output */
                // errs() << update->inst << ",s,,," << update->dif << ",," << update->id_r << ",,\n";

                /* DIMEMAS trace */
                if (this->inFunction >= 1) {
                    errs() << "dim:1:" << processor_id << ":" << thread_id << ":" << update->dif << "\n";
                    errs() << "dim:20:" << processor_id << ":" << thread_id << ":" << "50000001:1\n";
                    errs() << "dim:2:" << processor_id << ":" << thread_id << ":" << update->id_r << ":-1:" << update->msgSize << ":" << update->tag << ":" << update->comm << ":" << "2" << "\n";
                    errs() << "dim:20:" << processor_id << ":" << thread_id << ":" << "50000001:0\n";
                }
            }
            pthread_mutex_unlock(this->ls_lock);
            break;
        }  else {
            if (!print_query_once) {
                // pthread_mutex_lock(this->ls_lock);
                // fprintf(stderr, "QUERY NOT FOUND t=%d id_s=%d id_r=%d\n", update->type, update->id_s, update->id_r);
                // pthread_mutex_unlock(this->ls_lock);
            }
            print_query_once = 1;
        }
    }
}

void MPImap::update_MPI_Send(MPIcall *update, struct mpi_call_notification_msg *msg) {
    // Send message to DB
    send_msg_to_db(update);

    // Prepare query
    update->type = GET_RECV_INFO;
    send_query_and_update(update);
}

void MPImap::update_MPI_Recv(MPIcall *update, struct mpi_call_notification_msg *msg) {
    // Send message to DB
    send_msg_to_db(update);

    // Prepare query
    update->type = GET_SEND_INFO;
    send_query_and_update(update);
}

void MPImap::update_MPI_Sendrecv(MPIcall *update, MPIcall *update2, struct mpi_call_notification_msg *msg) {
    // Send PUT_SEND_INFO to DB
    update->type = PUT_SEND_INFO;
    send_msg_to_db(update);

    // Send PUT_RECV_INFO to DB
    update2->type = PUT_RECV_INFO;
    send_msg_to_db(update2);

    // Prepare query for GET_RECV_INFO
    update->type = GET_RECV_INFO;
    send_query_and_update(update);

    // Prepare query for GET_SEND_INFO
    update2->type = GET_SEND_INFO;
    send_query_and_update(update2);
}

void MPImap::update_MPI_Wait(MPIcall *update, struct mpi_call_notification_msg *msg) {
    return;
    // send_query_and_update(update);
}

void MPImap::send_msg_to_all_comm(MPIcall *update, int size) {
    int i;

    // This is the root node it should send (sizeof(COM) - 1) messages
    for (i = 0; i < size; i++)
        if (update->type == PUT_SEND_INFO && i != update->id_s) {
            update->id_r = i;
            send_msg_to_db(update);
        } else if (update->type == PUT_RECV_INFO && i != update->id_r) {
            update->id_s = i;
            send_msg_to_db(update);
        }

    if (update->type == PUT_SEND_INFO)
        update->id_r = update->id_s;
    else if (update->type == PUT_RECV_INFO)
        update->id_s = update->id_r;
}

void MPImap::send_query_and_update_comm(MPIcall *update, int size) {
    int i;

    // This is the root node it should send (sizeof(COM) - 1) messages
    for (i = 0; i < size; i++)
        if (update->type == GET_RECV_INFO && i != update->id_s) {
            update->id_r = i;
            send_query_and_update(update);
        } else if (update->type == GET_SEND_INFO && i != update->id_r) {
            update->id_s = i;
            send_query_and_update(update);
        }

    if (update->type == GET_RECV_INFO)
        update->id_r = update->id_s;
    else if (update->type == GET_SEND_INFO)
        update->id_s = update->id_r;
}

void MPImap::update_MPI_Bcast(MPIcall *update, struct mpi_call_notification_msg *msg) {
    // Steps:
    // - all receivers PUT_RECV_INFO
    // - root queries for GET_RECV_INFOs
    // - root determines the maximum
    // - root PUT_SEND_INFOs
    // - meanwhile, all receivers query for GET_SEND_INFO

    // Send messages to DB
    if (update->id_s == update->id_r) {
        update->type = PUT_SEND_INFO;
        send_msg_to_all_comm(update, msg->comm_size);
    } else {
        update->type = PUT_RECV_INFO;
        send_msg_to_db(update);
    }

    // Queries and updates
    if (update->id_s == update->id_r) {
        update->type = GET_RECV_INFO;
        send_query_and_update_comm(update, msg->comm_size);
    } else {
        update->type = GET_SEND_INFO;
        send_query_and_update(update);
    }
}

void MPImap::update_MPI_Reduce(MPIcall *update, struct mpi_call_notification_msg *msg) {
    // Steps:
    // - all senders PUT_SEND_INFO
    // - root queries for GET_SEND_INFOs
    // - root det the maximum
    // - root PUT_RECV_INFOs
    // - meanwhile, all receivers query for GET_RECV_INFO

    // Note: all nodes in the communicator sends messages. Including root node.
    update->type = PUT_SEND_INFO;
    send_msg_to_db(update);

    // Determine maximum
    if (update->id_s == update->id_r) {
        update->type = PUT_RECV_INFO;
        send_msg_to_all_comm(update, msg->comm_size);
        send_msg_to_db(update);
    }

    // Send updates
    if (update->id_s == update->id_r) {
        update->type = GET_SEND_INFO;
        send_query_and_update_comm(update, msg->comm_size);
        send_query_and_update(update);
    }

    // Final queries
    update->type = GET_RECV_INFO;
    send_query_and_update(update);
}

void MPImap::update_MPI_Allreduce(MPIcall *update, struct mpi_call_notification_msg *msg) {
    // Steps:
    // - all PUT_SEND_INFO to COMM + self (with first)
    // - all GET_SEND_INFO from COMM + self (on second)
    // - everyone should have the maximum 
    // - all PUT_RECV_INFO to COMM + self (with second)
    // - all GET_RECV_INFO to COMM + self (on first)

    // Note: all nodes in the communicator sends messages. Including root node.
    update->type = PUT_SEND_INFO;
    send_msg_to_all_comm(update, msg->comm_size);
    send_msg_to_db(update);

    update->type = PUT_RECV_INFO;
    send_msg_to_all_comm(update, msg->comm_size);
    send_msg_to_db(update);

    update->type = GET_SEND_INFO;
    send_query_and_update_comm(update, msg->comm_size);
    send_query_and_update(update);

    update->type = GET_RECV_INFO;
    send_query_and_update_comm(update, msg->comm_size);
    send_query_and_update(update);

    /*
    update->type = PUT_ALLREDUCE_INFO;
    send_msg_to_db(update);

    for (i = 0; i < update->comm_size; i++) {
        // i is the rank of a process inside the communicator
        send_query_allreduce(i, update->comm)
        |--> this function will increment the flag field of the query result (inside the DB!)
            - if the flag equals the comm_size field, than the entry can be erased from the DB
            - this is done because the DB has to keep the entry until all the communicator 
            processes gets the information.
        if recv_msg_from_db(&msg_recv) is called, the flag field will be updated inside the function
            - only if the query is found
    }

    */
}

void MPImap::update_MPI_Scatter(MPIcall *update, struct mpi_call_notification_msg *msg) {
    // Steps:
    // - all PUT_RECV_INFO to root (with second)
    // - root GET_RECV_INFO from COMM + self (on first)
    // - root has the maximum
    // - root PUT_SEND_INFO to COMM + self (with first)
    // - all GET_SEND_INFO from root (on second)

    // Note: all nodes in the communicator receives messages. Including root node.
    send_msg_to_db(update);

    if (update->id_s == update->id_r) {
        update->type = PUT_SEND_INFO;
        send_msg_to_all_comm(update, msg->comm_size);
        send_msg_to_db(update);
    }

    if (update->id_s == update->id_r) {
        update->type = GET_RECV_INFO;
        send_query_and_update_comm(update, msg->comm_size);
        send_query_and_update(update);
    }

    update->type = GET_SEND_INFO;
    send_query_and_update(update);
}

void MPImap::update_info(string call_name, MPIcall *update, MPIcall *update2, struct mpi_call_notification_msg *msg) {
    if (mpiMatchName(call_name, "MPI_Isend")) {
        update_MPI_Isend(update);
    } else if (mpiMatchName(call_name, "MPI_Irecv")) {
        update_MPI_Irecv(update);
    } else if (mpiMatchName(call_name, "MPI_Test")) { 
        if (msg->flag)
            update_MPI_Wait(update, msg);
    }
    // return;
    if (mpiMatchName(call_name, "MPI_Send")) {
        update_MPI_Send(update, msg);
    } else if (mpiMatchName(call_name, "MPI_Recv")) {
        update_MPI_Recv(update, msg);
    } else if (mpiMatchName(call_name, "MPI_Isend")) {
        update_MPI_Isend(update);
    } else if (mpiMatchName(call_name, "MPI_Irecv")) {
        update_MPI_Irecv(update);
    } else if (mpiMatchName(call_name, "MPI_Wait")) { 
        update_MPI_Wait(update, msg);
    } else if (mpiMatchName(call_name, "MPI_Test")) {
        if (msg->flag)
            update_MPI_Wait(update, msg);
    } else if (mpiMatchName(call_name, "MPI_Bcast")) {
        update_MPI_Bcast(update, msg);
    } else if (mpiMatchName(call_name, "MPI_Reduce")) {
        update_MPI_Reduce(update, msg);
    } else if (mpiMatchName(call_name, "MPI_Gather")) {
        update_MPI_Reduce(update, msg);
    } else if (mpiMatchName(call_name, "MPI_Gatherv")) {
        update_MPI_Reduce(update, msg);
    } else if (mpiMatchName(call_name, "MPI_Allreduce")) {
        //update_MPI_Allreduce(update, msg);
    } else if (mpiMatchName(call_name, "MPI_Allgather")) {
        update_MPI_Allreduce(update, msg);
    } else if (mpiMatchName(call_name, "MPI_Allgatherv")) {
        update_MPI_Allreduce(update, msg);
    } else if (mpiMatchName(call_name, "MPI_Alltoall")) {
        update_MPI_Allreduce(update, msg);
    } else if (mpiMatchName(call_name, "MPI_Alltoallv")) {
        update_MPI_Allreduce(update, msg);
    } else if (mpiMatchName(call_name, "MPI_Scatter")) {
        update_MPI_Scatter(update, msg);
    } else if (mpiMatchName(call_name, "MPI_Scatterv")) {
        update_MPI_Scatter(update, msg);
    } else if (mpiMatchName(call_name, "MPI_Init")) {
        update_MPI_Init(update);
    } else if (mpiMatchName(call_name, "MPI_Finalize")) {
        update_MPI_Finalize(update);
    }

    if (mpiMatchName(call_name, "MPI_Sendrecv") || mpiMatchName(call_name , "MPI_Sendrecv_replace"))
        update_MPI_Sendrecv(update, update2, msg);
}

void MPImap::visit(Instruction *I, struct message *msg, int tag, int inFunction) {
    pthread_mutex_lock(this->ls_lock);
    if (tag == 0) {
        if (!I && inFunction == 0) { // end of the analyzed function 
            errs() << "end of function " << "\n";
            errs() << "dim:1:"  << processor_id << ":" << thread_id << ":" << (*mix)->getNumTotalInsts()-previousCount << "\n";
            pthread_mutex_unlock(this->ls_lock);
            return;
        }
        previousCount = (*mix)->getNumTotalInsts();
    }
    pthread_mutex_unlock(this->ls_lock);
        
    this->inFunction = inFunction;

    if (I) {
        MPIcall update, update2;
        memset(&update, 0, sizeof(MPIcall));
        memset(&update2, 0, sizeof(MPIcall));

        CallInst *call = cast<CallInst>(I);
        Function *_call = get_calledFunction(call);

        assert(mix);
        update.dif = (*mix)->getNumTotalInsts() - previousCount;
        previousCount = (*mix)->getNumTotalInsts();

        update.inst = index;
        update2.inst = update.inst;
        update2.dif = update.dif;
        update.I = I;
        update2.I = I;

        // if (_call->getName().str() == "MPI_Test")
        // fprintf(stderr,"MPI_Test name=%s flag=%d mpi.source=%d\n",_call->getName().str().c_str(), msg->data.mpi.flag, msg->data.mpi.source);

        // if (msg->data.mpi.flag && msg->data.mpi.source==-1)
        // assert(msg->data.mpi.source>0);

        extract_info(_call->getName().str(), &update, &update2, &msg->data.mpi);
        update_info(_call->getName().str(), &update, &update2, &msg->data.mpi);

        index++;
    }
}


