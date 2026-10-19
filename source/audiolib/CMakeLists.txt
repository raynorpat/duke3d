# Jim Dose's audio library, SDL_mixer-backed. usrhooks.c is left out on
# purpose: sounds.c defines the same USRHOOKS_* functions. No include
# directory is exported: audiolib/assert.h would shadow <assert.h>.

add_library(audiolib STATIC
  al_midi.c
  dbopl.c
  dsl.c
  fx_man.c
  gmtimbre.c
  ll_man.c
  midi.c
  multivoc.c
  mv_mix.c
  mvreverb.c
  nodpmi.c
  oplemu.c
  pitch.c
  user.c)

target_link_libraries(audiolib PUBLIC duke3d_compile_options duke3d::SDL2_mixer)
