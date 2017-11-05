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

    zsys_init ();
    // FIXME: code is here to really quickly iterate with collectd client, it
    // will be transformed to actor later on
    lcc_connection_t *conn;
    int r = lcc_connect ("/var/run/collectd-unixsock", &conn);
    assert (r == 0);

    lcc_identifier_t *ret_ident;
    size_t ret_ident_num;

    r = lcc_listval (conn, &ret_ident, &ret_ident_num);
    assert (r == 0);

    for (size_t i = 0; i < ret_ident_num; i++) {
        char id[1024];
        r = lcc_identifier_to_string (conn, id, sizeof (id), ret_ident + i);
        assert (r == 0);
    
        zsys_info ("i=%zu, id=%s", i, id);

    /*
      lcc_identifier_t ident;
      r = lcc_string_to_identifier(conn, &ident, id);
      assert (r ==0);
    */

      size_t ret_values_num = 0;
      gauge_t *ret_values = NULL;
      char **ret_values_names = NULL;
      r = lcc_getval (conn, ret_ident + i, &ret_values_num, &ret_values, &ret_values_names);
      assert (r == 0);
      for (size_t j = 0; j < ret_values_num; j++)
        zsys_info ("\tj=%zu, %s=%e\n", j, ret_values_names[j], ret_values[j]);
    }

    LCC_DESTROY (conn);

    return 0;
}
