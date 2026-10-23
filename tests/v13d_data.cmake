# Helpers for tests that read 1.3D, 1.1, 1.0 or 0.99 data from outside the tree.
# Include it from a -P script.

# Edition fingerprints: DUKE3D.GRP MD5 (docs/v13d/reference.md, "Checksums";
# sw11 and sw10 are the 1.1 and 1.0 shareware).
set(_v13d_grp_md5_full 981125cb9237c19aa0237109958d2b50)
set(_v13d_grp_md5_sw c03558e3a78d1c5356dc69b6134c5b55)
set(_v13d_grp_md5_sw11 9b0683a74c8bf36bf85631616385bec8)
set(_v13d_grp_md5_sw10 1e57cf6272e8be0e746666700cc0ee96)
set(_v13d_grp_md5_sw099 56b35e575eba7f16c0e19628bd6bd934)

# Stops the script unless <dir> holds the given edition (full, sw, sw11 or
# sw10 or sw099).
function(v13d_check_edition dir edition)
  if(NOT IS_DIRECTORY "${dir}")
    message(FATAL_ERROR "data folder not found: ${dir}")
  endif()
  set(_grp "")
  foreach(_name DUKE3D.GRP duke3d.grp)
    if(EXISTS "${dir}/${_name}")
      set(_grp "${dir}/${_name}")
      break()
    endif()
  endforeach()
  if(NOT _grp)
    message(FATAL_ERROR "wrong edition: expected ${edition}, but ${dir} has no DUKE3D.GRP")
  endif()
  file(MD5 "${_grp}" _md5)
  if(NOT _md5 STREQUAL "${_v13d_grp_md5_${edition}}")
    message(FATAL_ERROR "wrong edition: expected ${edition}, ${_grp} has MD5 ${_md5}")
  endif()
endfunction()

function(v13d_game_profile edition out)
  if(edition STREQUAL "full")
    set(_profile 1.3d)
  elseif(edition STREQUAL "sw")
    set(_profile 1.3d-shareware)
  elseif(edition STREQUAL "sw11")
    set(_profile 1.1)
  elseif(edition STREQUAL "sw10")
    set(_profile 1.0)
  elseif(edition STREQUAL "sw099")
    set(_profile 0.99)
  else()
    message(FATAL_ERROR "unknown test edition: ${edition}")
  endif()
  set(${out} "${_profile}" PARENT_SCOPE)
endfunction()
