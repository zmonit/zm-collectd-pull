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
    //  TODO: Declare properties
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

    //  TODO: Initialize properties

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

        //  TODO: Free actor properties

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
    zstr_free (&command);
    zmsg_destroy (&request);
}


//  --------------------------------------------------------------------------
//  This is the actor which runs in its own thread.

void
zm_collectd_pull_actor_actor (zsock_t *pipe, void *args)
{
    zm_collectd_pull_actor_t * self = zm_collectd_pull_actor_new (pipe, args);
    if (!self)
        return;          //  Interrupted

    //  Signal actor successfully initiated
    zsock_signal (self->pipe, 0);

    while (!self->terminated) {
        zsock_t *which = (zsock_t *) zpoller_wait (self->poller, 0);
        if (which == self->pipe)
            zm_collectd_pull_actor_recv_api (self);
       //  Add other sockets when you need them.
    }
    zm_collectd_pull_actor_destroy (&self);
}

//  --------------------------------------------------------------------------
//  Self test of this actor.

// If your selftest reads SCMed fixture data, please keep it in
// src/selftest-ro; if your test creates filesystem objects, please
// do so under src/selftest-rw.
// The following pattern is suggested for C selftest code:
//    char *filename = NULL;
//    filename = zsys_sprintf ("%s/%s", SELFTEST_DIR_RO, "mytemplate.file");
//    assert (filename);
//    ... use the "filename" for I/O ...
//    zstr_free (&filename);
// This way the same "filename" variable can be reused for many subtests.
#define SELFTEST_DIR_RO "src/selftest-ro"
#define SELFTEST_DIR_RW "src/selftest-rw"

void
zm_collectd_pull_actor_test (bool verbose)
{
    printf (" * zm_collectd_pull_actor: ");
    //  @selftest
    //  Simple create/destroy test
    zactor_t *zm_collectd_pull_actor = zactor_new (zm_collectd_pull_actor_actor, NULL);
    assert (zm_collectd_pull_actor);

    zactor_destroy (&zm_collectd_pull_actor);
    //  @end

    printf ("OK\n");
}
