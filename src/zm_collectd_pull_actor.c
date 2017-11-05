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
    if (r != 0) {
        zsys_error ("zm_collectd_pull (%s): Failed to connect to collectd: %s",
                self->name,
                lcc_strerror (self->conn));
        self->terminated = true;
        return -1;
    }

    r = mlm_client_connect (self->client, self->endpoint, 5000, self->name);
    if (r != 0) {
        zsys_error ("zm_collectd_pull (%s): Failed to connect to malamute",
                self->name);
        self->terminated = true;
        return -1;
    }
    r = mlm_client_set_producer (self->client, ZM_PROTO_METRIC_STREAM);
    if (r != 0) {
        zsys_error ("zm_collectd_pull (%s): Failed to set client as a producer",
                self->name);
        self->terminated = true;
        return -1;
    }

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
    else
    if (streq (command, "ENDPOINT")) {
        char *endpoint = zmsg_popstr (request);
        if (!endpoint)
            zsys_error ("zm_collectd_pull (%s): ENDPOINT expects one parameter");
        else {
            zstr_free (&self->endpoint);
            self->endpoint = strdup (endpoint);
        }
        zstr_free (&endpoint);
    }
    else
    if (streq (command, "COLLECTD-SOCKET")) {
        char *collectd_socket = zmsg_popstr (request);
        if (!collectd_socket)
            zsys_error ("zm_collectd_pull (%s): COLLECTD-SOCKET expects one parameter");
        else {
            zstr_free (&self->collectd_socket);
            self->collectd_socket = strdup (collectd_socket);
        }
        zstr_free (&collectd_socket);
    }
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

    // adapted from src/collectdctl.c
#define RET_IDENT_DESTROY                                                      \
  do {                                                                         \
    if (ret_ident != NULL)                                                     \
      free(ret_ident);                                                         \
    ret_ident_num = 0;                                                         \
  } while (0)

#define RET_VALUES_DESTROY                                                            \
  do {                                                                         \
    if (ret_values != NULL)                                                    \
      free(ret_values);                                                        \
    if (ret_values_names != NULL) {                                            \
      for (size_t i = 0; i < ret_values_num; ++i)                              \
        free(ret_values_names[i]);                                             \
      free(ret_values_names);                                                  \
    }                                                                          \
    ret_values_num = 0;                                                        \
  } while (0)

    r = lcc_listval (self->conn, &ret_ident, &ret_ident_num);
    if (r != 0) {
        zsys_error ("zm_collectd_pull (%s): Failed to list values from collectd: %s",
                self->name,
                lcc_strerror (self->conn));
        return;
    }

    for (size_t i = 0; i < ret_ident_num; i++) {
        char id[1024];
        r = lcc_identifier_to_string (self->conn, id, sizeof (id), ret_ident + i);
        if (r != 0) {
            zsys_error ("zm_collectd_pull (%s): Failed to convert collectd identifier to string: %s",
                    self->name,
                    lcc_strerror (self->conn));
            RET_IDENT_DESTROY;
            return;
        }

        if (self->verbose)
            zsys_info ("zm_collectd_pull (%s): i=%zu, id=%s", self->name, i, id);


        size_t ret_values_num = 0;
        gauge_t *ret_values = NULL;
        char **ret_values_names = NULL;
        r = lcc_getval (self->conn, ret_ident + i, &ret_values_num, &ret_values, &ret_values_names);
        if (r != 0) {
            zsys_error ("zm_collectd_pull (%s): Failed to get valued for %s: %s",
                    self->name,
                    id,
                    lcc_strerror (self->conn));
            RET_IDENT_DESTROY;
            RET_VALUES_DESTROY;
            return;
        }

        char *sep = strchr (id, '/');
        char *device = "";
        char *type = id;
        if (sep) {
            type = sep + 1;
            sep = '\0';
            device = id;
        }
        zm_proto_set_id (self->msg, ZM_PROTO_METRIC);
        zm_proto_set_device (self->msg, device);
        zm_proto_set_time (self->msg, zclock_time ());
        zm_proto_set_ttl (self->msg, 5000);

        zm_proto_set_type (self->msg, type);
        for (size_t j = 0; j < ret_values_num; j++) {
            if (self->verbose)
                zsys_info ("zm_collectd_pull (%s): \tj=%zu, %s=%e\n",
                        self->name,
                        j,
                        ret_values_names[j],
                        ret_values[j]);
            char *value = zsys_sprintf ("%e", ret_values [j]);
            zm_proto_set_value (self->msg, value);
            zstr_free (&value);
            break;
        }

        zm_proto_set_unit (self->msg, "");

        zm_proto_send_mlm (self->msg, self->client, "subject");

        RET_VALUES_DESTROY;
    }

    RET_IDENT_DESTROY;
}

//  --------------------------------------------------------------------------
//  This is the actor which runs in its own thread.

void
zm_collectd_pull_actor (zsock_t *pipe, void *args)
{
    zm_collectd_pull_actor_t * self = zm_collectd_pull_actor_new (pipe, args);
    if (!self)
        return;          //  Interrupted

    if (args) {
        char *foo = (char*) args;
        zstr_free (&self->name);
        self->name = strdup (foo);
    }

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

    zfile_t *collectd_sock = zfile_new ("/var/run", "collectd-unixsock");
    assert (collectd_sock);
    bool collectd_sock_writeable = zfile_is_writeable (collectd_sock);
    zfile_close (collectd_sock);
    zfile_destroy (&collectd_sock);

    zactor_t *malamute = zactor_new (mlm_server, "malamute");
    if (verbose)
        zstr_sendx (malamute, "VERBOSE", NULL);

    zstr_sendx (malamute, "BIND", "inproc://malamute-unit-test", NULL);

    zactor_t *self = zactor_new (zm_collectd_pull_actor, "collectd-unit-test");
    assert (self);

    if (verbose)
        zstr_sendx (self, "VERBOSE", NULL);

    zstr_sendx (self, "ENDPOINT", "inproc://malamute-unit-test", NULL);
    zstr_sendx (self, "COLLECTD-SOCKET", "/var/run/collectd-unixsock");

    if (collectd_sock_writeable) {
        zstr_sendx (self, "START", NULL);
        zclock_sleep (3000);
    }
    else
        printf ("SKIPPED, collectd unix socket not accessible: ");

    zactor_destroy (&self);
    zactor_destroy (&malamute);

    printf ("OK\n");
}
