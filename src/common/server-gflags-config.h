/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef MINIKV_GFLAGS_CONFIG_H
#define MINIKV_GFLAGS_CONFIG_H

#include <gflags/gflags.h>

/**
 * glog
 */
DECLARE_string(glog_dir);
DECLARE_bool(glog_prefix);
DECLARE_int32(max_glog_size);
DECLARE_int32(glogbufsecs);
DECLARE_int32(mingloglevel);
DECLARE_bool(stop_glogging_if_full_disk);
DECLARE_bool(glogtostderr);
DECLARE_bool(alsoglogtostderr);
DECLARE_bool(colorglogtostderr);

/**
 * rpc
 */
DECLARE_int32(rpc_port);
DECLARE_int32(rpc_server_threads_cnt);
DECLARE_int32(rpc_io_threads_cnt);

/**
 * kv
 */
DECLARE_string(checkpoint_dir);
DECLARE_string(checkpoint_type)
DECLARE_string(wal_dir);
DECLARE_string(wal_type);
DECLARE_uint32(max_kv_pending_cnt);

#endif //MINIKV_GFLAGS_CONFIG_H
