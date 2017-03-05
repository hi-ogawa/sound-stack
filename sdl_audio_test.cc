/*

# Audio file format

$ wget http://www.wavsource.com/snds_2017-02-05_1692732105491832/movies/12_angry_men/answer.wav
$ ffprobe answer.wav
Input #0, wav, from 'answer.wav':
  Duration: 00:00:01.45, bitrate: 176 kb/s
    Stream #0:0: Audio: pcm_s16le ([1][0][0][0] / 0x0001), 11025 Hz, 1 channels, s16, 176 kb/s

# Interfaces

- SDL interface
  - https://wiki.libsdl.org/SDL_AudioSpec
  - https://wiki.libsdl.org/SDL_OpenAudioDevice
- pulseaudio interface
  - https://www.freedesktop.org/wiki/Software/PulseAudio/Documentation/Developer/
- kernel interface (alsa)
  - https://www.kernel.org/doc/html/v4.10/sound/index.html

# Example

$ make
$ ./sdl_audio_test answer.wav
INFO: AudioSpec freq     = 11025
INFO:           channels = 1
INFO:           samples  = 1024
INFO:           size     = 2048

*/

#include <SDL.h>
#include <stdio.h>


/*

# data structure

SDL_AudioDriver (singleton as current_audio)
'-' SDL_AudioDriverImpl (implements OpenDevice, PlayDevice, etc...)
'-* SDL_AudioDeviceItem (outputDevices managed via DetectDevices (see PULSEAUDIO_DetectDevices for example))
'-* SDL_AudioDeviceItem (inputDevices)

SDL_AudioDevice (as open_devices[16])
'-' SDL_AudioSpec
'-' SDL_AudioBufferQueue
'-' SDL_PrivateAudioData
    '-' pa_mainloop
    '-' pa_context
    '-' pa_stream

Q. when does pulseaudio check there's at least one current_audio.outputDevices ?

*/

void _main(char *filename) {
  SDL_AudioSpec want, have;
  SDL_AudioDeviceID dev;

  SDL_memset(&want, 0, sizeof(want));
  want.freq = 11025;
  want.format = AUDIO_S16LSB;
  want.channels = 1;
  want.samples = 2048;
  // --> .size = 2048 samples/buffer * 16 bits/sample * 1 channels
  //           = 4096 * 8 bits/buffer
  //           = 4096 bytes/buffer
  want.callback = NULL;

  dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
  // => open_audio_device =>
  //  - allocate SDL_AudioDevice with paused and enabled on
  //  - PULSEAUDIO_OpenDevice (as current_audio.impl.OpenDevice) =>
  //    - prepare pa_sample_spec, pa_buffer_attr, pa_channel_map, pa_stream_flags_t
  //    - allocate SDL_PrivateAudioData.mixbuf
  //    - ConnectToPulseServer => xxx_Internal =>
  //      - pa_mainloop_new ?
  //      - pa_mainloop_get_api ?
  //      - pa_context_new ?
  //      - pa_context_connect ?
  //      - pa_mainloop_iterate until pa_context_get_state == PA_CONTEXT_READY
  //    - pa_channel_map_init_auto ?
  //    - pa_stream_new ?
  //    - pa_stream_connect_playback ?
  //    - pa_mainloop_iterate until pa_context_get_state == PA_CONTEXT_READY
  //  - allocate AudioDevice.buffer_queue_pool
  //    - AudioDevice.spec.callback = SDL_BufferQueueDrainCallback
  //  - push AudioDevice to global open_devices
  //  - SDL_CreateThreadInternal(SDL_RunAudio) (see below)

  // - SDL_RunAudio =>
  //   - loop below until AudioDevice.shutdown
  //     - stream = SDL_PrivateAudioData.mixbuf
  //     - if AudioDevice.paused fill stream with silence
  //     - otherwise fill stream by executing AudioDevice.spec.callback (e.g. SDL_BufferQueueDrainCallback) =>
  //       - dequeue_audio_from_device
  //         - [copy data from AudioDevice.buffer_queue_head/tail/pool to mixbuf]
  //     - PULSEAUDIO_PlayDevice => pa_stream_write ?
  //     - PULSEAUDIO_WaitDevice =>

  if (dev == 0) {
    SDL_Log("Failed to open audio: %s", SDL_GetError());
  } else if (have.format != want.format) {
    SDL_Log("Format didn't match.");
  } else {

    SDL_Log("AudioSpec freq     = %d", have.freq);
    SDL_Log("          channels = %d", have.channels);
    SDL_Log("          samples  = %d", have.samples);
    SDL_Log("          size     = %d", have.size);

    Uint32 wav_length;
    Uint8 *wav_buffer;

    if (SDL_LoadWAV(filename, &have, &wav_buffer, &wav_length) == NULL) {
      SDL_Log("SDL_LoadWAV: %s\n", SDL_GetError());
    } else {
      SDL_QueueAudio(dev, wav_buffer, wav_length);
      // => queue_audio_to_device
      //  - [copy given data to AudioDevice.buffer_queue_head/tail/pool]
      SDL_PauseAudioDevice(dev, 0);
      // AudioDevice.paused = 0
      SDL_Delay(2000);
      SDL_CloseAudioDevice(dev);
      SDL_FreeWAV(wav_buffer);
    }

  }
}

int main(int argc, char **argv) {
  SDL_Init(SDL_INIT_AUDIO);
  // SDL_Init => SDL_AudioInit =>
  //   - setup current_audio
  //   - PULSEAUDIO_Init =>
  //     - ConnectToPulseServer (for hotplug_mainloop and hotplug_context)
  //       - [see SDL_OpenAudioDevice path for what this does]
  //     - setup current_audio.impl
  //   - PULSEAUDIO_DetectDevices (as current_audio.impl.DetectDevices) =>
  //     - initial AudioDevice setup (blocking pa operation)
  //     - SDL_CreateThreadInternal(HotplugThread)
  //       [ spawn a dedicated thread only for pushing audio
  //         source/sink hotplug event via SDL_AddAudioDevice, SDL_RemoveAudioDevice ]
  _main(argv[1]);
  SDL_Quit();
  return 0;
}
