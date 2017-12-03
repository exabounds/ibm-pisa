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

#include "mpi_sync_server.h"

#define MAXEVENTS 128

MPIdb *DB = NULL;

static int boot_server(int argc, char **argv) {
    int sockfd = -1;

    if (argc != 4) {
        fprintf(stderr, "Usage: %s SERVER_IP PORT MAX_CLIENTS\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "Error: cannot open socket\n");
        exit(EXIT_FAILURE); 
    }

    int portno;
    sscanf(argv[2], "%d", &portno);

    // Add info for SERVER_IP:PORT
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(portno);

    // Bind socket
    int ret = -1;
    ret = bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (ret < 0) {
        fprintf(stderr, "Error: cannot bind socket\n");
        exit(EXIT_FAILURE);
    }

    // Change socket to listening mode
    int max_clients;
    sscanf(argv[3], "%d", &max_clients);
    listen(sockfd, max_clients + 1);

    return sockfd;
}

static void accept_new_connections(int sockfd, int epollfd) {
    struct sockaddr in_addr;
    socklen_t in_len;
    int infd = -1;

    in_len = sizeof(in_addr);
    infd = accept(sockfd, &in_addr, &in_len);
    if (infd < 0) {
        if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
            // We have processed all incoming connections.
            return;
        else {
            fprintf(stderr, "Error: accept\n");
            return;
        }
    }

    // Add socket to the list of fds to monitor
    struct epoll_event event;
    event.data.fd = infd;
    event.events = EPOLLIN;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, infd, &event);
}

static void init_db(void) {
    DB = malloc(sizeof(MPIdb));
    DB->s_call = NULL;
    DB->r_call = NULL;
}

static struct MPIcall_node * create_new_MPIcall_node(MPIcall *info) {
    struct MPIcall_node *node = NULL;

    node = malloc(sizeof(struct MPIcall_node));
    memcpy(&node->info, info, sizeof(MPIcall));
    node->next = NULL;

    return node;
} 

static void insert_db(struct MPIcall_node *node) {
    struct MPIcall_node **sr = &DB->r_call;

    if (node->info.type == PUT_SEND_INFO)
        sr = &DB->s_call;

    struct MPIcall_node *iter = *sr;
    // The list is empty
    if (!iter) {
        *sr = node;
        return;
    }

    // Add at the end of the list
    while (iter->next)
        iter = iter->next;
    iter->next = node;
}

static void update_db(MPIcall *update) {
    if (!DB)
        init_db();

    struct MPIcall_node *node = create_new_MPIcall_node(update);
    insert_db(node);
}

static int items_are_equal(MPIcall *a, MPIcall *b) {
    if ((a->id_s != b->id_s) && (a->id_s != MPI_ANY_SOURCE) && (b->id_s != MPI_ANY_SOURCE))
        return 0;

    if ((a->id_r != b->id_r) && (a->id_r != MPI_ANY_SOURCE) && (b->id_r != MPI_ANY_SOURCE))
        return 0;

    if ((a->tag != b->tag) && (a->tag != MPI_ANY_TAG) && (b->tag != MPI_ANY_TAG))
        return 0;

    if (a->comm != b->comm)
        return 0;

    return 1;
}

static void clean_node(struct MPIcall_node *v) {
    free(v);
}

static void remove_from_db(struct MPIcall_node **sr, struct MPIcall_node *v) {
    struct MPIcall_node *iter = *sr;

    if (!iter) {
        fprintf(stderr, "This should not happen!\n");
        exit(EXIT_FAILURE);
    }

    if (*sr == v) {
        // We must remove the first node of the linked list
        *sr = v->next;
        clean_node(v);
        return;
    }

    while (iter->next) {
        if (iter->next == v) {
            iter->next = v->next;
            clean_node(v);
            return;
        }

        iter = iter->next;
    }
}

static void query_db(MPIcall *query) {
    if (!DB) {
        query->type = INFO_NOT_FOUND;
        fprintf(stderr, "\t----> INFO_NOT_FOUND\n");
        return;
    }

    struct MPIcall_node **sr = &DB->r_call;

    if (query->type == GET_SEND_INFO)
        sr = &(DB->s_call);

    struct MPIcall_node *iter = *sr;

    while (iter) {
        if (items_are_equal(&iter->info, query)) {
            // Retrieve information.
            query->issue_cycle = iter->info.issue_cycle;
            query->inst =  iter->info.inst; 
            fprintf(stderr, "\t---> query found with ic=%llu\n", iter->info.issue_cycle);

            // Delete from db since it is not anymore useful
            remove_from_db(sr, iter);
            return;
        }

        iter = iter->next;
    }

    // Set type to INFO_NOT_FOUND in order to notify the upper function
    // that the query was not successful.
    query->type = INFO_NOT_FOUND;
    fprintf(stderr, "\t----> INFO_NOT_FOUND\n");
}

static void send_response(int fd, MPIcall *query) {
    // check query
    query_db(query);

    // send message back
    write(fd, query, sizeof(MPIcall));
}

static void process_msg(int fd) {
    int count = 1;
    MPIcall update;

    count = read(fd, &update, sizeof(update));
    if (count == 0) {
        // Connection closed
        // Closing the descriptor will also make epoll
        // remove it from the set of descriptors which
        // are monitored
        close(fd);
        return;
    }

    fprintf(stderr, " msg %d: type=%d id_s=%d id_r=%d tag=%d comm=%p ic=%llu\n",
        count, update.type, update.id_s, update.id_r, update.tag, update.comm, update.issue_cycle);
    if (update.type == PUT_SEND_INFO || update.type == PUT_RECV_INFO)
        // We don't have to send anything back.
        // Just update the DB.
        update_db(&update);
    else if (update.type == GET_SEND_INFO || update.type == GET_RECV_INFO) 
        send_response(fd, &update);
}

int main(int argc, char **argv) {
    int sockfd = boot_server(argc, argv);

    int epollfd = epoll_create(MAXEVENTS);
    if (epollfd < 0) {
        fprintf(stderr, "Error: epoll_create - %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Initialize epoll event
    struct epoll_event *events = NULL, event;
    event.data.fd = sockfd;
    event.events = EPOLLIN;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &event);

    // Buffer where events are returned
    events = calloc(MAXEVENTS, sizeof(event));

    // Main loop
    while (1) {
        int n, i;

        fprintf(stderr, "before wait\n");
        n = epoll_wait(epollfd, events, MAXEVENTS, -1);
        for (i = 0; i < n; i++) {
            if (events[i].data.fd == sockfd &&
                (events[i].events & EPOLLIN)) {
                // New connection
                fprintf(stderr, "accept new connection\n");
                accept_new_connections(sockfd, epollfd);
            } else if (events[i].events & EPOLLIN) {
                fprintf(stderr, "process msg\n");
                process_msg(events[i].data.fd);
            } else {
                // Nothing to be done
                fprintf(stderr, "Nothing to be done\n");
            }
        }
        fprintf(stderr, "n after epoll_wait = %d\n", n);
    }

    free(events);
    close(sockfd);

    return 0;
}
