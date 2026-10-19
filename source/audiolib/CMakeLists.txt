# Jim Dose's audio library, SDL_mixer-backed. usrhooks.c is left out on
# purpose: sounds.c defines the same USRHOOKS_* functions. No include
# directory is exported: audiolib/assert.h would shadow <assert.h>.

add_library(audiolib STATIC
  dsl.c
  fx_man.c
  ll_man.c
  multivoc.c
  mv_mix.c
  mvreverb.c
  nodpmi.c
  pitch.c
  user.c)

target_link_libraries(audiolib PUBLIC duke3d_compile_options duke3d::SDL2_mixer)
