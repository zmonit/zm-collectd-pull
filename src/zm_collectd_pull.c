/*  =========================================================================
    zm_collectd_pull - Main daemon

    Copyright (c) the Contributors as noted in the AUTHORS file.  This file is part
    of zmon.it, the fast and scalable monitoring system.

    This Source Code Form is subject to the terms of the Mozilla Public License, v.
    2.0. If a copy of the MPL was not distributed with this file, You can obtain
    one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

/*
@header
    zm_collectd_pull - Main daemon
@discuss
@end
*/

#include "zm_collectd_pull_classes.h"

int main (int argc, char *argv [])
{
    bool verbose = false;
    int argn;
    for (argn = 1; argn < argc; argn++) {
        if (streq (argv [argn], "--help")
        ||  streq (argv [argn], "-h")) {
            puts ("zm-collectd-pull [options] ...");
            puts ("  --verbose / -v         verbose test output");
            puts ("  --help / -h            this information");
            return 0;
        }
        else
        if (streq (argv [argn], "--verbose")
        ||  streq (argv [argn], "-v"))
            verbose = true;
        else {
            printf ("Unknown option: %s\n", argv [argn]);
            return 1;
        }
    }
    //  Insert main code here
    if (verbose)
        zsys_info ("zm_collectd_pull - Main daemon");

    //TODO: configuration for malamute
    zactor_t *malamute = zactor_new (mlm_server, "malamute");
    if (verbose)
        zstr_sendx (malamute, "VERBOSE", NULL);

    zstr_sendx (malamute, "BIND", "inproc://malamute", NULL);

    zactor_t *server = zactor_new (zm_collectd_pull_actor, "cli");
    if (verbose)
        zstr_sendx (server, "VERBOSE", NULL);

    zstr_sendx (server, "START", NULL);

    while (true)
    {
        char *msg = zstr_recv (server);
        if (!msg)
            break;
        zsys_debug ("%s", msg);
        zstr_free (&msg);
    }

    zactor_destroy (&server);
    zactor_destroy (&malamute);
    return 0;
}
