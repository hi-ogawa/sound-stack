Just follow sound stack in Linux system.

- SDL: https://wiki.libsdl.org/CategoryAudio
- PulseAudio: https://freedesktop.org/software/pulseaudio/doxygen/async.html
- ALSA:
  - library: http://www.alsa-project.org/alsa-doc/alsa-lib/pcm.html
  - kernel: https://www.kernel.org/doc/html/v4.10/sound/index.html

---

- PulseAudio

```
# Data structure

pa_core
'-' pa_mainloop_api
'-' pa_sink
'-' pa_mempool
'-* pa_hook (PA_CORE_HOOK_MAX many types of pre-defined callbacks)
'-* pa_module
'-* pa_card
'-* ? client

pa_mainloop
'-' pa_mainloop_api
'-* struct pollfd
'-* pa_io_event
    '-' pa_io_event_cb_t (callback)
'-* pa_time_event
    '-' pa_time_event_cb_t (callback)
'-* pa_defer_event
    '-' pa_defer_event_cb_t (callback)

pa_module
'-' userdata
'-* argument


# Daemon

- main =>
  - pa_mainloop_new
  - pa_core_new(pa_mainloop_api) =>
    - pa_mempool_new
    - pa_msgobject_new(pa_core) ?
    - pa_hook_init (PA_CORE_HOOK_MAX times)
  - pa_cli_command_execute_file_stream =>
    - ... (for "load-module" input) =>
      - pa_cli_command_load => pa_module_load =>
        - initialize pa_module
        - execute "pa__init" function from module
  - assert at least one module exists
  - pa_mainloop_run =>
    - loop pa_mainloop_iterate until it fails =>
      - pa_mainloop_prepare(pa_mainloop)
      - pa_mainloop_poll(pa_mainloop)
      - pa_mainloop_dispatch(pa_mainloop)

 | shm_overview(7)
 | Q. how do they notify each other about shm status ?


# Modules

- module-udev-detect (see 78-sound-card.rules and 90-pulseaudio.rules too)
  - pa__init =>
    - (some udev client routine? <libudev.h>)
      - udev_new, udev_monitor_new_from_netlink ...
      - register monitor_cb to core mainloop with poll fd from udev_monitor_get_fd
    - setup_inotify =>
      - register inotify_cb to core mainloop with poll fd from inotify_add_watch("/dev/snd")
    - udev_list_entry_foreach
      - process_path => process_device(udev_device) =>
        - card_changed =>
          - initialize struct device
          - device.card_name = ("alsa_card.%s", n);
          - construct device.args (for "module-alsa-card" loading later)
          - pa_hashmap_put(device) to userdata.devices
          - verify_access(device) =>
            - check access(cd, R_OK|W_OK) for "/dev/snd/controlC%s"
            - pa_module_load("module-alsa-card", device.args)

  - monitor_cb =>
    - process_device (same as above)

  - inotify_cb =>
    - run verify_access for possibly changed device

 | userdata (per module struct)
 | '-* device (as hash)
 | '-' udev
 | '-' udev_monitor
 | '-' inotify_fd

 | libudev(3)
 | inotify(7)


- module-alsa-card
  - pa__init =>
    - userdata.alsa_card_index = snd_card_get_index(device_id)
    - snd_config_update_free_global
    - pa_alsa_ucm_query_profiles (assume ucm is true)=>
      - snd_use_case_mgr_open
    - pa_module_hook_connect PA_CORE_HOOK_SINK_INPUT_PUT and PA_CORE_HOOK_SOURCE_OUTPUT_PUT
    - pa_alsa_profile_set_probe ...
    - pa_alsa_init_proplist_card ...
    - pa_card_new

  - (on pa_hook_fire PA_CORE_HOOK_SOURCE_OUTPUT_PUT) =>
    - source_output_put_hook_callback ...

 | userdata
 | '-' device_id
 | '-' alsa_card_index
 | '-' pa_alsa_profile_set
 | '-' pa_card ?
 |     '-* sinks
 |     '-* sources
 |     '-' pa_proplist
 |     '-* pa_profile

 | <asoundlib.h>
 | Q. follow how pulseaudio calls snd_pcm_xxx interfaces ?
 | http://www.alsa-project.org/alsa-doc/alsa-lib/pcm.html


# Client

- pa_context_connect =>
  - pa_context_set_state(c, PA_CONTEXT_CONNECTING)
  - try_next_connection =>
    - pa_socket_client_new_string => pa_socket_client_new_unix
    - pa_socket_client_set_callback(... on_connection)

  | - (callback) on_connection => setup_context =>
  |   - pa_pstream_new
  |   - pa_pdispatch_new
  |   - pa_pstream_send_tagstruct PA_COMMAND_AUTH
  |   - pa_pdispatch_register_reply(...  setup_complete_callback)
  |   - pa_context_set_state(c, PA_CONTEXT_AUTHORIZING)
  |
  | - (callback) setup_complete_callback
  |   - pa_pstream_send_tagstruct PA_COMMAND_SET_CLIENT_NAME
  |   - pa_pdispatch_register_reply(... setup_complete_callback)
  |   - pa_context_set_state(c, PA_CONTEXT_SETTING_NAME)
  |
  | - (callback) setup_complete_callback
  |   - pa_context_set_state(c, PA_CONTEXT_READY);

- pa_stream_connect_playback =>
  - create_stream(PA_STREAM_PLAYBACK) =>
    - pa_pstream_send_tagstruct PA_COMMAND_CREATE_PLAYBACK_STREAM
    - pa_pdispatch_register_reply(... pa_create_stream_callback)
    - pa_stream_set_state(s, PA_STREAM_CREATING)

  | - (callback) pa_create_stream_callback =>
  |   - create_stream_complete =>
  |     - pa_stream_set_state(s, PA_STREAM_READY)

- pa_stream_write =>
  - pa_stream_write_ext_free =>
    - pa_pstream_send_memblock =>
      - pa_mainloop.defer_enable(pa_pstream.defer_event, 1)

  | - (callback) pa_pstream.defer_event =>
  |   - defer_callback => do_pstream_read_write
  |     - pa_mainloop.defer_enable(pa_pstream.defer_event, 0)
  |     - do_write => pa_iochannel_write => pa_write

Q. follow server side of handlers for these client pdispatch.

- pa_native_protocol_connect =>
  - pa_pstream_new
  - pa_pdispatch_new with command_table (e.g. PA_COMMAND_CREATE_PLAYBACK_STREAM to command_create_playback_stream)

- command_create_playback_stream
  - playback_stream_new
    - pa_sink_input_new
    - pa_msgobject_new(playback_stream)
    - pa_idxset_put(c->output_streams, s, &s->index)
    - pa_sink_input_put(s->sink_input)

  | playback_stream
  | '-' output_stream
  | '-' pa_sink_input

```

---

- Kernel and Driver

```
# Data structure

# Initialization

# Playback

# Virtual files

- /dev/snd/control, /dev/snd/pcm
- /proc/asound/
  - https://www.kernel.org/doc/html/v4.10/sound/designs/procfile.html
  - https://www.kernel.org/doc/html/v4.10/sound/alsa-configuration.html#proc-interfaces-proc-asound

Q. follow how sound/pci/hda/hda_intel.c defines alsa pcm interface ?
```

---

- Environment

```
$ pactl info
Server String: unix:/run/user/1000/pulse/native
Library Protocol Version: 30
Server Protocol Version: 30
Is Local: yes
Client Index: 878
Tile Size: 65472
User Name: hiogawa
Host Name: hiogawa-xenial-tp13
Server Name: pulseaudio
Server Version: 8.0
Default Sample Specification: s16le 2ch 44100Hz
Default Channel Map: front-left,front-right
Default Sink: alsa_output.pci-0000_00_1f.3.analog-stereo
Default Source: alsa_input.pci-0000_00_1f.3.analog-stereo
Cookie: dc94:bc46

$ pactl list modules | grep 'Name:'
	Name: module-device-restore
	Name: module-stream-restore
	Name: module-card-restore
	Name: module-augment-properties
	Name: module-switch-on-port-available
	Name: module-udev-detect
	Name: module-alsa-card
	Name: module-jackdbus-detect
	Name: module-bluetooth-policy
	Name: module-bluetooth-discover
	Name: module-bluez5-discover
	Name: module-native-protocol-unix
	Name: module-default-device-restore
	Name: module-rescue-streams
	Name: module-always-sink
	Name: module-intended-roles
	Name: module-suspend-on-idle
	Name: module-systemd-login
	Name: module-position-event-sounds
	Name: module-filter-heuristics
	Name: module-filter-apply
	Name: module-x11-publish
	Name: module-x11-bell
	Name: module-x11-cork-request
	Name: module-x11-xsmp

$ lspci | grep -i audio
00:1f.3 Audio device: Intel Corporation Sunrise Point-LP HD Audio (rev 21)

$ sudo lspci -v -s 00:1f.3
00:1f.3 Audio device: Intel Corporation Sunrise Point-LP HD Audio (rev 21)
	Subsystem: Lenovo Sunrise Point-LP HD Audio
	Flags: bus master, fast devsel, latency 64, IRQ 280
	Memory at f1140000 (64-bit, non-prefetchable) [size=16K]
	Memory at f1130000 (64-bit, non-prefetchable) [size=64K]
	Capabilities: [50] Power Management version 3
	Capabilities: [60] MSI: Enable+ Count=1/1 Maskable- 64bit+
	Kernel driver in use: snd_hda_intel
	Kernel modules: snd_hda_intel, snd_soc_skl
```