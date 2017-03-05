Just follow sound stack in Linux system.

- SDL: https://wiki.libsdl.org/CategoryAudio
- PulseAudio: https://www.freedesktop.org/wiki/Software/PulseAudio/Documentation/Developer/
- ALSA: https://www.kernel.org/doc/html/v4.10/sound/index.html

---

$ lspci | grep -i audio
00:1f.3 Audio device: Intel Corporation Sunrise Point-LP HD Audio (rev 21)
$ sudo lspci -s 00:1f.3 -v
00:1f.3 Audio device: Intel Corporation Sunrise Point-LP HD Audio (rev 21)
	Subsystem: Lenovo Sunrise Point-LP HD Audio
	Flags: bus master, fast devsel, latency 64, IRQ 280
	Memory at f1140000 (64-bit, non-prefetchable) [size=16K]
	Memory at f1130000 (64-bit, non-prefetchable) [size=64K]
	Capabilities: [50] Power Management version 3
	Capabilities: [60] MSI: Enable+ Count=1/1 Maskable- 64bit+
	Kernel driver in use: snd_hda_intel
	Kernel modules: snd_hda_intel, snd_soc_skl
