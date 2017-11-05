/*  =========================================================================
    zm_collectd_pull_actor - zm device actor

    Copyright (c) the Contributors as noted in the AUTHORS file.  This file is part
    of zmon.it, the fast and scalable monitoring system.

    This Source Code Form is subject to the terms of the Mozilla Public License, v.
    2.0. If a copy of the MPL was not distributed with this file, You can obtain
    one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

#ifndef ZM_COLLECTD_PULL_ACTOR_H_INCLUDED
#define ZM_COLLECTD_PULL_ACTOR_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif


//  @interface
//  Create new zm_collectd_pull_actor actor instance.
//  @TODO: Describe the purpose of this actor!
//
//      zactor_t *zm_collectd_pull_actor = zactor_new (zm_collectd_pull_actor, NULL);
//
//  Destroy zm_collectd_pull_actor instance.
//
//      zactor_destroy (&zm_collectd_pull_actor);
//
//  Enable verbose logging of commands and activity:
//
//      zstr_send (zm_collectd_pull_actor, "VERBOSE");
//
//  Start zm_collectd_pull_actor actor.
//
//      zstr_sendx (zm_collectd_pull_actor, "START", NULL);
//
//  Stop zm_collectd_pull_actor actor.
//
//      zstr_sendx (zm_collectd_pull_actor, "STOP", NULL);
//
//  This is the zm_collectd_pull_actor constructor as a zactor_fn;
ZM_COLLECTD_PULL_EXPORT void
    zm_collectd_pull_actor_actor (zsock_t *pipe, void *args);

//  Self test of this actor
ZM_COLLECTD_PULL_EXPORT void
    zm_collectd_pull_actor_test (bool verbose);
//  @end

#ifdef __cplusplus
}
#endif

#endif
