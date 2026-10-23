# Jim Dose's audio library, played through SDL audio. Only the parts the
# game uses are kept; the DOS sound card drivers are gone. sounds.c supplies
# the USRHOOKS_* functions. Includes are written as "audiolib/...".

add_library(audiolib STATIC
  al_midi.c
  dbopl.c
  dsl.c
  fx_man.c
  gmtimbre.c
  ll_man.c
  midi.c
  multivoc.c
  music.c
  mv_mix.c
  mvreverb.c
  oplemu.c
  pitch.c
  user.c)

target_link_libraries(audiolib PUBLIC duke3d_compile_options duke3d::SDL2)
if(NOT WIN32 AND NOT APPLE)
  target_link_libraries(audiolib PUBLIC m)
endif()
