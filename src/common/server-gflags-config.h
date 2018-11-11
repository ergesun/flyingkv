/**
 * This work copyright Chao Sun(qq:296449610) and licensed under
 * a Creative Commons Attribution 3.0 Unported License(https://creativecommons.org/licenses/by/3.0/).
 */

#ifndef FLYINGKV_GFLAGS_CONFIG_H
#define FLYINGKV_GFLAGS_CONFIG_H

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
 * server
 */
DECLARE_bool(init_daemon);

/**
 * rpc
 */
DECLARE_int32(rpc_port);
DECLARE_int32(rpc_server_threads_cnt);
DECLARE_int32(rpc_io_threads_cnt);

/**
 * access control center
 */
DECLARE_string(acc_conf_path);


/**
 * kv
 */
DECLARE_string(kv_type);
DECLARE_uint32(kv_check_wal_size_tick);
DECLARE_uint32(kv_do_checkpoint_wal_size);
DECLARE_string(wal_dir);
DECLARE_string(wal_type);
DECLARE_uint32(wal_entry_write_version);
DECLARE_uint32(wal_max_segment_size);
DECLARE_uint32(wal_batch_read_size);
DECLARE_string(cp_dir);
DECLARE_string(cp_type);
DECLARE_uint32(cp_entry_write_version);
DECLARE_uint32(cp_batch_read_size);

#endif //FLYINGKV_GFLAGS_CONFIG_H
