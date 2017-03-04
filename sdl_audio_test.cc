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
  - /dev/shm/pulse-shm-
  - find kernel doc
- kernel interface
  - /dev/snd/
  - find kernel doc

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

void _main(char *filename) {
  SDL_AudioSpec want, have;
  SDL_AudioDeviceID dev;

  SDL_memset(&want, 0, sizeof(want));
  want.freq = 11025;
  want.format = AUDIO_S16LSB;
  want.channels = 1;
  want.samples = 2048;
  want.callback = NULL;

  dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
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
      SDL_PauseAudioDevice(dev, 0);
      SDL_Delay(2000);
      SDL_CloseAudioDevice(dev);
      SDL_FreeWAV(wav_buffer);
    }

  }
}

int main(int argc, char **argv) {
  SDL_Init(SDL_INIT_AUDIO);
  _main(argv[1]);
  SDL_Quit();
  return 0;
}
