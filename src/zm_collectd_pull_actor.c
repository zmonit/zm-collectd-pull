/*  =========================================================================
    zm_collectd_pull_actor - zm device actor

    Copyright (c) the Contributors as noted in the AUTHORS file.  This file is part
    of zmon.it, the fast and scalable monitoring system.

    This Source Code Form is subject to the terms of the Mozilla Public License, v.
    2.0. If a copy of the MPL was not distributed with this file, You can obtain
    one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

/*
@header
    zm_collectd_pull_actor - zm device actor
@discuss
@end
*/

#include "zm_collectd_pull_classes.h"

//  Structure of our actor

struct _zm_collectd_pull_actor_t {
    zsock_t *pipe;              //  Actor command pipe
    zpoller_t *poller;          //  Socket poller
    bool terminated;            //  Did caller ask us to quit?
    bool verbose;               //  Verbose logging enabled?
    //  Declare properties
    char *collectd_socket;      //  Name of collectd unix socket
    zm_proto_t *msg;            //  Msg to manipulate with
    lcc_connection_t *conn;     //  Connection to collectd
    char *name;                 //  Actor name
    char *endpoint;             //  Malamute endpoint
    mlm_client_t *client;       //  Malamute client
};


//  --------------------------------------------------------------------------
//  Create a new zm_collectd_pull_actor instance

static zm_collectd_pull_actor_t *
zm_collectd_pull_actor_new (zsock_t *pipe, void *args)
{
    zm_collectd_pull_actor_t *self = (zm_collectd_pull_actor_t *) zmalloc (sizeof (zm_collectd_pull_actor_t));
    assert (self);

    self->pipe = pipe;
    self->terminated = false;
    self->poller = zpoller_new (self->pipe, NULL);

    //  Initialize properties
    self->collectd_socket = strdup ("/var/run/collectd-unixsock");
    self->msg = zm_proto_new ();
    self->name = strdup ("zm_collectd_pull");
    self->endpoint = strdup ("inproc://malamute");
    self->client = mlm_client_new ();

    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the zm_collectd_pull_actor instance

static void
zm_collectd_pull_actor_destroy (zm_collectd_pull_actor_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zm_collectd_pull_actor_t *self = *self_p;

        //  Free actor properties
        LCC_DESTROY (self->conn);
        zstr_free (&self->collectd_socket);
        zm_proto_destroy (&self->msg);
        zstr_free (&self->name);
        zstr_free (&self->endpoint);
        mlm_client_destroy (&self->client);

        //  Free object itself
        zpoller_destroy (&self->poller);
        free (self);
        *self_p = NULL;
    }
}


//  Start this actor. Return a value greater or equal to zero if initialization
//  was successful. Otherwise -1.

static int
zm_collectd_pull_actor_start (zm_collectd_pull_actor_t *self)
{
    assert (self);

    //  TODO: Add startup actions
    int r = lcc_connect (self->collectd_socket, &self->conn);
    assert (r == 0);

    r = mlm_client_connect (self->client, self->endpoint, 5000, self->name);
    assert (r == 0);
    r = mlm_client_set_producer (self->client, ZM_PROTO_METRIC_STREAM);
    assert (r ==0);

    return 0;
}


//  Stop this actor. Return a value greater or equal to zero if stopping
//  was successful. Otherwise -1.

static int
zm_collectd_pull_actor_stop (zm_collectd_pull_actor_t *self)
{
    assert (self);

    //  TODO: Add shutdown actions

    return 0;
}


//  Here we handle incoming message from the node

static void
zm_collectd_pull_actor_recv_api (zm_collectd_pull_actor_t *self)
{
    //  Get the whole message of the pipe in one go
    zmsg_t *request = zmsg_recv (self->pipe);
    if (!request)
       return;        //  Interrupted

    char *command = zmsg_popstr (request);
    if (streq (command, "START"))
        zm_collectd_pull_actor_start (self);
    else
    if (streq (command, "STOP"))
        zm_collectd_pull_actor_stop (self);
    else
    if (streq (command, "VERBOSE"))
        self->verbose = true;
    else
    if (streq (command, "$TERM"))
        //  The $TERM command is send by zactor_destroy() method
        self->terminated = true;
    else {
        zsys_error ("invalid command '%s'", command);
        assert (false);
    }
    zm_collectd_pull_actor_stop (self);
    zstr_free (&command);
    zmsg_destroy (&request);
}

static void
zm_collectd_pull (zm_collectd_pull_actor_t *self)
{
    assert (self);
    lcc_identifier_t *ret_ident;
    size_t ret_ident_num;
    int r;

    r = lcc_listval (self->conn, &ret_ident, &ret_ident_num);
    assert (r == 0);

    for (size_t i = 0; i < ret_ident_num; i++) {
        char id[1024];
        r = lcc_identifier_to_string (self->conn, id, sizeof (id), ret_ident + i);
        assert (r == 0);

        zsys_info ("i=%zu, id=%s", i, id);


        size_t ret_values_num = 0;
        gauge_t *ret_values = NULL;
        char **ret_values_names = NULL;
        r = lcc_getval (self->conn, ret_ident + i, &ret_values_num, &ret_values, &ret_values_names);
        assert (r == 0);

        for (size_t j = 0; j < ret_values_num; j++)
            zsys_info ("\tj=%zu, %s=%e\n", j, ret_values_names[j], ret_values[j]);
    }
}

//  --------------------------------------------------------------------------
//  This is the actor which runs in its own thread.

void
zm_collectd_pull_actor (zsock_t *pipe, void *args)
{
    zm_collectd_pull_actor_t * self = zm_collectd_pull_actor_new (pipe, args);
    if (!self)
        return;          //  Interrupted

    //  Signal actor successfully initiated
    zsock_signal (self->pipe, 0);

    while (!self->terminated) {
        zsock_t *which = (zsock_t *) zpoller_wait (self->poller, 1000);
        if (which == self->pipe)
            zm_collectd_pull_actor_recv_api (self);
       //  Add other sockets when you need them.
        else
        if (zpoller_expired (self->poller))
            zm_collectd_pull (self);
    }
    zm_collectd_pull_actor_destroy (&self);
}

//  --------------------------------------------------------------------------
//  Self test of this actor.

void
zm_collectd_pull_actor_test (bool verbose)
{
    printf (" * zm_collectd_pull_actor: ");
    //  @selftest
    //  Simple create/destroy test
    zactor_t *self = zactor_new (zm_collectd_pull_actor, NULL);
    assert (self);

    zactor_destroy (&self);

    printf ("OK\n");
}
