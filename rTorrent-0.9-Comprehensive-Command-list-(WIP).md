# Comprehensive list of rTorrent 0.9 commands
#### Use of the deprecated commands is highly discouraged as those commands are subject to removal at any time.

The type column indicates whether it is a variable or a command. You can use ^x (Ctrl+x) to open the command input box and either enter a command like `directory.default.set=/some/path` or you can print the value of a variable to see what the current setting for a command is, like `print=$directory.default=`

Note: To print a variable, you have to include the equal sign at the end.

Many variables can also be used as commands, however commands are not intended to be used as variables.

### Operators
| Command | Deprecated Commands | Description | Type |
| ------- | :-------------------: | ----------- | ---- |
| `if` | - | Evaluates a condition to be either true or false and takes action based on that | Command |
| `not` | - | Inversion of `if` Evaluates a condition to be either NOT true or NOT false and takes action based on that | Command |
| `false` | - | Always returns false - used when you want to take action on something being false | Command |
| `and` | - | Evaluates 2 or more conditions and takes action based on whether both are true or if one is false | Command |
| `or` | - | Evaluates 2 or more conditions and takes action based on whether one is true, or both are false | Command |

### Misc
| Command | Deprecated Commands | Description | Type |
| ------- | ------------------- | ----------- | ---- |
| `encoding.add` | `encoding_list` | Configure filename encodings | Command |
| `keys.layout.set` | `key_layout` | Define keyboard layout for key bindings `<qwerty|azerty|qwertz|dvorak>` | Command |

### Execution
| Command | Deprecated Commands | Description | Type |
| ------- | ------------------- | ----------- | ---- |
| `execute2` | `execute` | See [COMMAND Execute](https://github.com/rakshasa/rtorrent/wiki/COMMAND-Execute) -- Tilde gets special treatment | Command |
| `execute.throw` | `execute_throw` | Same as above | Command |
| `execute.throw.bg` |  | Same as above but run the command in the background | Command |
| `execute.nothrow` | `execute_nothrow` | See [COMMAND Execute](https://github.com/rakshasa/rtorrent/wiki/COMMAND-Execute) | Command |
| `execute.nothrow.bg` |  | Same as above but run the command in the background | Command |
| `execute.capture` | `execute_capture` | See [COMMAND Execute](https://github.com/rakshasa/rtorrent/wiki/COMMAND-Execute) | Command |
| `execute.capture_nothrow` | `execute_capture_nothrow` | Same as above | Command |
| `execute.raw` | `execute_raw` | See [COMMAND Execute](https://github.com/rakshasa/rtorrent/wiki/COMMAND-Execute) -- Tilde does not get special treatment | Command |
| `execute.raw.bg` |  | Same as above but run the command in the background | Command |
| `execute.raw_nothrow` | `execute_raw_nothrow` | [COMMAND Execute](https://github.com/rakshasa/rtorrent/wiki/COMMAND-Execute) | Command |
| `execute.raw_nothrow.bg` |  | Same as above but run the command in the background | Command |

### Scheduling
| Command | Deprecated Commands | Description | Type |
| ------- | ------------------- | ----------- | ---- |
| `schedule2` | `schedule` | See [COMMAND Scheduling](https://github.com/rakshasa/rtorrent/wiki/COMMAND-Scheduling) | Command |
| `schedule_remove2` | `schedule_remove` | See [COMMAND Scheduling](https://github.com/rakshasa/rtorrent/wiki/COMMAND-Scheduling) | Command |

### Directory
| Command | Deprecated Commands | Description | Type |
| ------- | ------------------- | ----------- | ---- |
| `directory.default` | `get_directory` | Prints the default directory for downloaded torrent data | Variable |
| `directory.default.set` | `directory` | Sets the default directory for downloaded torrent data | Command |
| `directory.default.set` | `set_directory` | Same as above | Command |

### Ratio
| Command | Deprecated Commands | Description | Type |
| ------- | ------------------- | ----------- | ---- |
| `group.seeding.ratio.disable` | `ratio.disable` | | Command |
| `group.seeding.ratio.enable` | `ratio.enable` | | Command |
| `group2.seeding.ratio.max` | `ratio.max` | | Variable |
| `group2.seeding.ratio.max.set` | `ratio.max.set` | | Command |
| `group2.seeding.ratio.min` | `ratio.min` | | Variable |
| `group2.seeding.ratio.min.set` | `ratio.min.set` | | Command |
| `group2.seeding.ratio.upload`	| `ratio.upload` | | Variable |
| `group2.seeding.ratio.upload.set` | `ratio.upload.set` | | Command |

### Conversion
| Command | Deprecated Commands | Description | Type |
| ------- | ------------------- | ----------- | ---- |
| `convert.date` | `to_date` | | Command |
| `convert.elapsed_time` | `to_elapsed_time` | | Command |
| `convert.gm_date` | `to_gm_date` | | Command |
| `convert.gm_time` | `to_gm_time` | | Command |
| `convert.kb` | `to_kb` | | Command |
| `convert.mb` | `to_mb` | | Command |
| `convert.time` | `to_time` | | Command |
| `convert.throttle` | `to_throttle` | | Command |
| `convert.xb` | `to_xb` | | Command |

### Protocol
| Command | Deprecated Commands | Description | Type |
| ------- | ------------------- | ----------- | ---- |
| `protocol.connection.leech` | `get_connection_leech` | | Variable |
| `protocol.connection.leech.set` | `connection_leech` | | Command |
| `protocol.connection.leech.set` | `set_connection_leech` | | Command |
| `protocol.connection.seed` | `get_connection_seed` | | Variable |
| `protocol.connection.seed.set` | `connection_seed` | | Command |
| `protocol.connection.seed.set` | `set_connection_seed` | | Command |
| `protocol.encryption.set` | `encryption` | | Command |
| `protocol.pex` | `get_peer_exchange` | | Variable |
| `protocol.pex.set` | `peer_exchange` | | Command |
| `protocol.pex.set` | `set_peer_exchange` | | Command |

### Memory/Pieces
| Command | Deprecated Commands | Description | Type |
| ------- | ------------------- | ----------- | ---- |
| `pieces.memory.current` | `get_memory_usage` | | Variable |
| `pieces.memory.max` | `get_max_memory_usage` | | Variable |
| `pieces.memory.max.set` | `max_memory_usage` | | Command |
| `pieces.memory.max.set` | `set_max_memory_usage` | | Command |
| `pieces.hash.on_completion` | `get_check_hash` | | Variable |
| `pieces.hash.on_completion.set` | `check_hash` | | Command |
| `pieces.hash.on_completion.set` | `set_check_hash` | | Command |
| `pieces.preload.type` | `get_preload_type` | | Variable |
| `pieces.preload.min_size` | `get_preload_min_size` | | Variable |
| `pieces.preload.min_size.set` | `set_preload_min_size` | | Command |
| `pieces.preload.min_rate` | `get_preload_required_rate` | | Variable |
| `pieces.preload.min_rate.set` | `set_preload_required_rate` | | Command |
| `pieces.preload.type.set` | `set_preload_type` | | Command |
| `pieces.stats_preloaded` | `get_stats_preloaded` | | Variable |
| `pieces.stats_not_preloaded` | `get_stats_not_preloaded` | | Variable |
| `pieces.sync.always_safe` | `get_safe_sync` | | Variable |
| `pieces.sync.always_safe.set` | `set_safe_sync` | | Command |
| `pieces.sync.timeout` | `get_timeout_sync` | | Variable |
| `pieces.sync.timeout.set` | `set_timeout_sync` | | Command |
| `pieces.sync.timeout_safe` | `get_timeout_safe_sync` | | Variable |
| `pieces.sync.timeout_safe.set` | `set_timeout_safe_sync` | | Command |

### Throttle
| Command | Deprecated Commands | Description | Type |
| ------- | ------------------- | ----------- | ---- |
| `throttle.down` | `throttle_down` | | Variable |
| `throttle.down.max` | `get_throttle_down_max` | | Command |
| `throttle.down.rate` | `get_throttle_down_rate` | | Command |
| `throttle.global_down.max_rate` | `get_download_rate` | | Variable |
| `throttle.global_down.max_rate.set` | `set_download_rate` | | Command |
| `throttle.global_down.max_rate.set_kb` | `download_rate` | | Command |
| `throttle.global_down.rate` | `get_down_rate` | | Variable |
| `throttle.global_down.total` | `get_down_total` | | Variable |
| `throttle.global_up.max_rate` | `get_upload_rate` | | Variable |
| `throttle.global_up.max_rate.set` | `set_upload_rate` | | Command |
| `throttle.global_up.max_rate.set_kb` | `upload_rate` | | Command |
| `throttle.global_up.rate` | `get_up_rate` | | Variable |
| `throttle.global_up.total` | `get_up_total` | | Variable |
| `throttle.ip` | `throttle_ip` | | Variable |
| `throttle.max_downloads.set` | `max_downloads` | | Command |
| `throttle.max_downloads.div` | `get_max_downloads_div` | | Variable |
| `throttle.max_downloads.div.set` | `max_downloads_div` | | Command |
| `throttle.max_downloads.div.set` | `set_max_downloads_div` | | Command |
| `throttle.max_downloads.global` | `get_max_downloads_global` | | Variable |
| `throttle.max_downloads.global.set` | `max_downloads_global` | | Command |
| `throttle.max_downloads.global.set` | `set_max_downloads_global` | | Command |
| `throttle.max_peers.normal` | `get_max_peers` | | Variable |
| `throttle.max_peers.normal.set` | `max_peers` | | Command |
| `throttle.max_peers.normal.set` | `set_max_peers` | | Command |
| `throttle.max_peers.seed` | `get_max_peers_seed` | | Variable |
| `throttle.max_peers.seed.set` | `max_peers_seed` | | Command |
| `throttle.max_peers.seed.set` | `set_max_peers_seed` | | Command |
| `throttle.max_uploads` | `get_max_uploads` | | Variable |
| `throttle.max_uploads.set` | `max_uploads` | | Command |
| `throttle.max_uploads.set` | `set_max_uploads` | | Command |
| `throttle.max_uploads.div` | `get_max_uploads_div` | | Variable |
| `throttle.max_uploads.div.set` | `max_uploads_div` | | Command |
| `throttle.max_uploads.div.set` | `set_max_uploads_div` | | Command |
| `throttle.max_uploads.global` | `get_max_uploads_global` | | Variable |
| `throttle.max_uploads.global.set` | `max_uploads_global` | | Command |
| `throttle.max_uploads.global.set` | `set_max_uploads_global` | | Command |
| `throttle.min_downloads.set` | `min_downloads` | | Variable |
| `throttle.min_peers.normal` | `get_min_peers` | | Variable |
| `throttle.min_peers.normal.set` | `min_peers` | | Command |
| `throttle.min_peers.normal.set` | `set_min_peers` | | Command |
| `throttle.min_peers.seed` | `get_min_peers_seed` | | Variable |
| `throttle.min_peers.seed.set` | `min_peers_seed` | | Command |
| `throttle.min_peers.seed.set` | `set_min_peers_seed` | | Command |
| `throttle.min_uploads.set` | `min_uploads` | | Variable |
| `throttle.up` | `throttle_up` | | Variable |
| `throttle.up.max` | `get_throttle_up_max` | | Variable |
| `throttle.up.rate` | `get_throttle_up_rate` | | Variable |

### DHT
| Command | Deprecated Commands | Description | Type |
| ------- | ------------------- | ----------- | ---- |
| `dht.add_node` | `dht_add_node` | | Command |
| `dht.mode.set` | `dht` | | Command |
| `dht.port` | `get_dht_port` | | Variable |
| `dht.port.set` | `dht_port` | | Command |
| `dht.port.set` | `set_dht_port` | | Command |
| `dht.throttle.name` | `get_dht_throttle` | | Variable |
| `dht.throttle.name.set` | `set_dht_throttle` | | Command |
| `dht.statistics` | `dht_statistics` | | Variable |

### Network
| Command | Deprecated Commands | Description | Type |
| ------- | ------------------- | ----------- | ---- |
| `network.bind_address` | `get_bind` | | Variable |
| `network.bind_address.set` | `bind` | | Command |
| `network.bind_address.set` | `set_bind` | | Command |
| `network.local_address` | `get_ip` | | Variable |
| `network.local_address.set` | `ip` | | Command |
| `network.local_address.set` | `set_ip` | | Command |
| `network.http.capath` | `get_http_capath` | | Variable |
| `network.http.capath.set` | `http_capath` | | Command |
| `network.http.capath.set` | `set_http_capath` | | Command |
| `network.http.cacert` | `get_http_cacert` | | Variable |
| `network.http.cacert.set` | `http_cacert` | | Command |
| `network.http.cacert.set` | `set_http_cacert` | | Command |
| `network.http.max_open` | `get_max_open_http` | | Variable |
| `network.http.max_open.set` | `set_max_open_http` | | Command |
| `network.http.proxy_address` | `get_http_proxy` | | Variable |
| `network.http.proxy_address.set` | `http_proxy` | | Command |
| `network.http.proxy_address.set` | `set_http_proxy` | | Command |
| `network.max_open_files` | `get_max_open_files` | | Variable |
| `network.max_open_files.set` | `set_max_open_files` | | Command |
| `network.max_open_sockets` | `get_max_open_sockets` | | Variable |
| `network.port_open` | `get_port_open` | | Variable |
| `network.port_open.set` | `port_open` | | Command |
| `network.port_open.set` | `set_port_open` | | Command |
| `network.port_random` | `get_port_random` | | Variable |
| `network.port_random.set` | `port_random` | | Command |
| `network.port_random.set` | `set_port_random` | | Command |
| `network.port_range` | `get_port_range` | | Variable |
| `network.port_range.set` | `port_range` | | Command |
| `network.port_range.set` | `set_port_range` | | Command |
| `network.proxy_address` | `get_proxy_address` | | Variable |
| `network.proxy_address.set` | `proxy_address` | | Command |
| `network.proxy_address.set` | `set_proxy_address` | | Command |
| `network.receive_buffer.size` | `get_receive_buffer_size` | | Variable |
| `network.receive_buffer.size.set` | `set_receive_buffer_size` | | Command |
| `network.scgi.dont_route.set` | `set_scgi_dont_route` | | Command |
| `network.scgi.dont_route` | `get_scgi_dont_route` | | Variable |
| `network.scgi.open_local` | `scgi_local` | | Variable | Command |
| `network.scgi.open_port` | `scgi_port` | | Variable | Command |
| `network.send_buffer.size` | `get_send_buffer_size` | | Variable |
| `network.send_buffer.size.set` | `set_send_buffer_size` | | Command |
| `network.xmlrpc.dialect.set` | `set_xmlrpc_dialect` | | Command |
| `network.xmlrpc.dialect.set` | `xmlrpc_dialect` | | Command |
| `network.xmlrpc.size_limit` | `get_xmlrpc_size_limit` | | Variable |
| `network.xmlrpc.size_limit.set` | `set_xmlrpc_size_limit` | | Command |
| `network.xmlrpc.size_limit.set` | `xmlrpc_size_limit` | | Command |

### Session
| Command | Deprecated Commands | Description | Type |
| ------- | ------------------- | ----------- | ---- |
| `session.path` | `get_session` | | Variable |
| `session.path.set` | `set_session` | | Command |
| `session.path.set` | `session` | | Command |
| `session.name` | `get_name` | | Variable |
| `session.name.set` | `set_name` | | Command |
| `session.on_completion` | `get_session_on_completion` | | Variable |
| `session.on_completion.set` | `set_session_on_completion` | | Command |
| `session.save` | `session_save` | | Variable |
| `session.use_lock` | `get_session_lock` | | Variable |
| `session.use_lock.set` | `set_session_lock` | | Command |

### Method
| Command | Deprecated Commands | Description | Type |
| ------- | ------------------- | ----------- | ---- |
| `method.insert` | `system.method.insert` | | Command |
| `method.erase` | `system.method.erase` | | Command |
| `method.get` | `system.method.get` | | Command |
| `method.set` | `system.method.set` | | Command |
| `method.list_keys` | `system.method.list_keys` | | Command |
| `method.has_key` | `system.method.has_key` | | Command |
| `method.set_key` | `system.method.set_key` | | Command |

### System
| Command | Deprecated Commands | Description | Type |
| ------- | ------------------- | ----------- | ---- |
| `system.file.allocate` | `system.file_allocate` | | Variable |
| `system.file.allocate.set` | `system.file_allocate.set` | | Command |
| `system.file.max_size` | `get_max_file_size` | | Variable |
| `system.file.max_size.set` | `set_max_file_size` | | Command |
| `system.file.split_size` | `get_split_file_size` | | Variable |
| `system.file.split_size.set` | `set_split_file_size` | | Command |
| `system.file.split_suffix` | `get_split_suffix` | | Variable |
| `system.file.split_suffix.set` | `set_split_suffix` | | Command |

### Load
| Command | Deprecated Commands | Description | Type |
| ------- | ------------------- | ----------- | ---- |
| `load.normal` | `load` | | Command |
| `load.start` | `load_start` | | Command |
| `load.start_verbose` | `load_start_verbose` | | Command |
| `load.raw` | `load_raw` | | Command |
| `load.raw_start` | `load_raw_start` | | Command |
| `load.raw_verbose` | `load_raw_verbose` | | Command |
| `load.verbose` | `load_verbose` | | Command |

### Tracker
| Command | Deprecated Commands | Description | Type |
| ------- | ------------------- | ----------- | ---- |
| `trackers.numwant` | `get_tracker_numwant` | | Variable |
| `trackers.numwant.set` | `set_tracker_numwant` | | Command |
| `trackers.numwant.set` | `tracker_numwant` | | Command |
| `trackers.use_udp` | `get_use_udp_trackers` | | Variable |
| `trackers.use_udp.set` | `set_use_udp_trackers` | | Command |
| `trackers.use_udp.set` | `use_udp_trackers` | | Command |

### Downloads
| Command | Deprecated Commands | Description | Type |
| ------- | ------------------- | ----------- | ---- |
| `d.base_filename` | `d.get_base_filename` | | Variable |
| `d.base_path` | `d.get_base_path` | | Variable |
| `d.bitfield` | `d.get_bitfield` | | Variable |
| `d.bytes_done` | `d.get_bytes_done` | | Variable |
| `d.chunk_size` | `d.get_chunk_size` | | Variable |
| `d.chunks_hashed` | `d.get_chunks_hashed` | | Variable |
| `d.complete` | `d.get_complete` | | Variable |
| `d.completed_bytes` | `d.get_completed_bytes` | | Variable |
| `d.completed_chunks` | `d.get_completed_chunks` | | Variable |
| `d.connection_current` | `d.get_connection_current` | | Variable |
| `d.connection_current.set` | `d.set_connection_current` | | Command |
| `d.connection_leech` | `d.get_connection_leech` | | Variable |
| `d.connection_seed` | `d.get_connection_seed` | | Variable |
| `d.custom` | `d.get_custom` | | Variable |
| `d.custom.set` | `d.set_custom` | | Command |
| `d.custom1` | `d.get_custom1` | | Variable |
| `d.custom1.set` | `d.set_custom1` | | Command |
| `d.custom2` | `d.get_custom2` | | Variable |
| `d.custom2.set` | `d.set_custom2` | | Command |
| `d.custom3` | `d.get_custom3` | | Variable |
| `d.custom3.set` | `d.set_custom3` | | Command |
| `d.custom4` | `d.get_custom4` | | Variable |
| `d.custom4.set` | `d.set_custom4` | | Command |
| `d.custom5` | `d.get_custom5` | | Variable |
| `d.custom5.set` | `d.set_custom5` | | Command |
| `d.custom_throw` | `d.get_custom_throw` | | Variable |
| `d.create_link` | `create_link` | | Variable |
| `d.creation_date` | `d.get_creation_date` | | Variable |
| `d.delete_link` | `delete_link` | | Variable |
| `d.delete_tied` | `delete_tied` | | Variable |
| `d.directory` | `d.get_directory` | | Variable |
| `d.directory.set` | `d.set_directory` | | Command |
| `d.directory_base` | `d.get_directory_base` | | Variable |
| `d.directory_base.set` | `d.set_directory_base` | | Command |
| `d.down.rate` | `d.get_down_rate` | | Variable |
| `d.down.total` | `d.get_down_total` | | Variable |
| `d.free_diskspace` | `d.get_free_diskspace` | | Variable |
| `d.hash` | `d.get_hash` | | Variable |
| `d.hashing` | `d.get_hashing` | | Variable |
| `d.hashing_failed` | `d.get_hashing_failed` | | Variable |
| `d.hashing_failed.set` | `d.set_hashing_failed` | | Command |
| `d.ignore_commands` | `d.get_ignore_commands` | | Variable |
| `d.ignore_commands.set` | `d.set_ignore_commands` | | Command |
| `d.left_bytes` | `d.get_left_bytes` | | Variable |
| `d.local_id` | `d.get_local_id` | | Variable |
| `d.local_id_html` | `d.get_local_id_html` | | Variable |
| `d.loaded_file` | `d.get_loaded_file` | | Variable |
| `d.max_file_size` | `d.get_max_file_size` | | Variable |
| `d.max_file_size.set` | `d.set_max_file_size` | | Command |
| `d.max_size_pex` | `d.get_max_size_pex` | | Variable |
| `d.message` | `d.get_message` | | Variable |
| `d.message.set` | `d.set_message` | | Command |
| `d.mode` | `d.get_mode` | | Variable |
| `d.multicall2` | `d.multicall` | | Variable |
| `d.name` | `d.get_name` | | Variable |
| `d.peer_exchange` | `d.get_peer_exchange` | | Variable |
| `d.peers_accounted` | `d.get_peers_accounted` | | Variable |
| `d.peers_complete` | `d.get_peers_complete` | | Variable |
| `d.peers_connected` | `d.get_peers_connected` | | Variable |
| `d.peers_max` | `d.get_peers_max` | | Variable |
| `d.peers_max.set` | `d.set_peers_max` | | Command |
| `d.peers_min` | `d.get_peers_min` | | Variable |
| `d.peers_min.set` | `d.set_peers_min` | | Command |
| `d.peers_not_connected` | `d.get_peers_not_connected` | | Variable |
| `d.priority` | `d.get_priority` | | Variable |
| `d.priority.set` | `d.set_priority` | | Command |
| `d.priority_str` | `d.get_priority_str` | | Variable |
| `d.ratio` | `d.get_ratio` | | Variable |
| `d.size_bytes` | `d.get_size_bytes` | | Variable |
| `d.size_chunks` | `d.get_size_chunks` | | Variable |
| `d.size_files` | `d.get_size_files` | | Variable |
| `d.size_pex` | `d.get_size_pex` | | Variable |
| `d.skip.rate` | `d.get_skip_rate` | | Variable |
| `d.skip.total` | `d.get_skip_total` | | Variable |
| `d.start` | - | | Command |
| `d.state` | `d.get_state` | | Variable |
| `d.state_changed` | `d.get_state_changed` | | Variable |
| `d.state_counter` | `d.get_state_counter` | | Variable |
| `d.stop` | - | | Command |
| `d.throttle_name` | `d.get_throttle_name` | | Variable |
| `d.throttle_name.set` | `d.set_throttle_name` | | Command |
| `d.tied_to_file` | `d.get_tied_to_file` | | Variable |
| `d.tied_to_file.set` | `d.set_tied_to_file` | | Command |
| `d.tracker_focus` | `d.get_tracker_focus` | | Variable |
| `d.tracker_numwant` | `d.get_tracker_numwant` | | Variable |
| `d.tracker_numwant.set` | `d.set_tracker_numwant` | | Command |
| `d.tracker_size` | `d.get_tracker_size` | | Variable |
| `d.up.rate` | `d.get_up_rate` | | Variable |
| `d.up.total` | `d.get_up_total` | | Variable |
| `d.uploads_max` | `d.get_uploads_max` | | Variable |
| `d.uploads_max.set` | `d.set_uploads_max` | | Command |

### Torrents
| Command | Deprecated Commands | Description | Type |
| ------- | ------------------- | ----------- | ---- |
| `t.group` | `t.get_group` | | Variable |
| `t.id` | `t.get_id` | | Variable |
| `t.is_enabled.set` | `t.set_enabled` | | Command |
| `t.min_interval` | `t.get_min_interval` | | Variable |
| `t.normal_interval` | `t.get_normal_interval` | | Variable |
| `t.scrape_complete` | `t.get_scrape_complete` | | Variable |
| `t.scrape_downloaded` | `t.get_scrape_downloaded` | | Variable |
| `t.scrape_incomplete` | `t.get_scrape_incomplete` | | Variable |
| `t.scrape_time_last` | `t.get_scrape_time_last` | | Variable |
| `t.type` | `t.get_type` | | Variable |
| `t.url` | `t.get_url` | | Variable |

### Files
| Command | Deprecated Commands | Description | Type |
| ------- | ------------------- | ----------- | ---- |
| `f.completed_chunks` | `f.get_completed_chunks` | | Variable |
| `f.frozen_path` | `f.get_frozen_path` | | Variable |
| `f.last_touched` | `f.get_last_touched` | | Variable |
| `f.match_depth_next` | `f.get_match_depth_next` | | Variable |
| `f.match_depth_prev` | `f.get_match_depth_prev` | | Variable |
| `f.offset` | `f.get_offset` | | Variable |
| `f.path` | `f.get_path` | | Variable |
| `f.path_components` | `f.get_path_components` | | Variable |
| `f.path_depth` | `f.get_path_depth` | | Variable |
| `f.priority` | `f.get_priority` | | Variable |
| `f.range_first` | `f.get_range_first` | | Variable |
| `f.range_second` | `f.get_range_second` | | Variable |
| `f.size_bytes` | `f.get_size_bytes` | | Variable |
| `f.size_chunks` | `f.get_size_chunks` | | Variable |
| `f.priority.set` | `f.set_priority` | | Variable |
| `fi.filename_last` | `fi.get_filename_last` | | Variable |

### Peers
| Command | Deprecated Commands | Description | Type |
| ------- | ------------------- | ----------- | ---- |
| `p.address` | `p.get_address` | | Variable |
| `p.client_version` | `p.get_client_version` | | Variable |
| `p.completed_percent` | `p.get_completed_percent` | | Variable |
| `p.down_rate` | `p.get_down_rate` | | Variable |
| `p.down_total` | `p.get_down_total` | | Variable |
| `p.id` | `p.get_id` | | Variable |
| `p.id_html` | `p.get_id_html` | | Variable |
| `p.options_str` | `p.get_options_str` | | Variable |
| `p.peer_rate` | `p.get_peer_rate` | | Variable |
| `p.peer_total` | `p.get_peer_total` | | Variable |
| `p.port` | `p.get_port` | | Variable |
| `p.up_rate` | `p.get_up_rate` | | Variable |
| `p.up_total` | `p.get_up_total` | | Variable |

### Views
| Command | Deprecated Commands | Description | Type |
| ------- | ------------------- | ----------- | ---- |
| `view.add` | `view_add` | | Command |
| `view.filter` | `view_filter` | | Command |
| `view.filter_on` | `view_filter_on` | | Command |
| `view.list` | `view_list` | | Command |
| `view.set` | `view_set` | | Command |
| `view.sort` | `view_sort` | | Command |
| `view.sort_current` | `view_sort_current` | | Command |
| `view.sort_new` | `view_sort_new` | | Command |
