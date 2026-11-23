# The two binaries of the single-player game (docs/design.md, section 4.2;
# docs/decisions/0010-project-and-binary-names.md).
#
# The source lists are those of the release's Visual Studio projects, starwars.vcproj
# for the engine and game/game.vcproj for the game module, as they stand after M0.
# Sources are listed explicitly; a new file is added here by hand.
#
# The Win32 platform code in win32/ is not built; sys/ replaces it
# (docs/decisions/0005-platform-layer-sdl3.md).
add_executable(jasp
  client/cl_cgame.cpp
  client/cl_cin.cpp
  client/cl_console.cpp
  client/cl_input.cpp
  client/cl_keys.cpp
  client/cl_main.cpp
  client/cl_mp3.cpp
  client/cl_parse.cpp
  client/cl_scrn.cpp
  client/cl_ui.cpp
  client/snd_ambient.cpp
  client/snd_dma.cpp
  client/snd_mem.cpp
  client/snd_mix.cpp
  client/snd_music.cpp
  game/genericparser2.cpp
  game/q_math.cpp
  game/q_shared.cpp
  ghoul2/G2_API.cpp
  ghoul2/G2_bolts.cpp
  ghoul2/G2_bones.cpp
  ghoul2/G2_misc.cpp
  ghoul2/G2_savetables.cpp
  ghoul2/G2_surfaces.cpp
  mp3code/cdct.c
  mp3code/csbt.c
  mp3code/csbtb.c
  mp3code/csbtl3.c
  mp3code/cup.c
  mp3code/cupini.c
  mp3code/cupl1.c
  mp3code/cupl3.c
  mp3code/cwin.c
  mp3code/cwinb.c
  mp3code/cwinm.c
  mp3code/hwin.c
  mp3code/l3dq.c
  mp3code/l3init.c
  mp3code/mdct.c
  mp3code/mhead.c
  mp3code/msis.c
  mp3code/towave.c
  mp3code/uph.c
  mp3code/upsf.c
  qcommon/cm_draw.cpp
  qcommon/cm_load.cpp
  qcommon/cm_patch.cpp
  qcommon/cm_polylib.cpp
  qcommon/cm_randomterrain.cpp
  qcommon/cm_shader.cpp
  qcommon/cm_terrain.cpp
  qcommon/cm_terrainmap.cpp
  qcommon/cm_test.cpp
  qcommon/cm_trace.cpp
  qcommon/cmd.cpp
  qcommon/common.cpp
  qcommon/cvar.cpp
  qcommon/files_common.cpp
  qcommon/files_pc.cpp
  qcommon/md4.cpp
  qcommon/msg.cpp
  qcommon/net_chan.cpp
  qcommon/stringed_ingame.cpp
  qcommon/stringed_interface.cpp
  qcommon/sg_fields.cpp
  qcommon/unzip.cpp
  qcommon/z_memman_pc.cpp
  renderer/matcomp.c
  renderer/tr_WorldEffects.cpp
  renderer/tr_arioche.cpp
  renderer/tr_backend.cpp
  renderer/tr_bsp.cpp
  renderer/tr_cmds.cpp
  renderer/tr_curve.cpp
  renderer/tr_draw.cpp
  renderer/tr_font.cpp
  renderer/tr_gamma.cpp
  renderer/tr_ghoul2.cpp
  renderer/tr_glconfig.cpp
  renderer/tr_image.cpp
  renderer/tr_init.cpp
  renderer/tr_light.cpp
  renderer/tr_main.cpp
  renderer/tr_marks.cpp
  renderer/tr_mesh.cpp
  renderer/tr_model.cpp
  renderer/tr_noise.cpp
  renderer/tr_quicksprite.cpp
  renderer/tr_scene.cpp
  renderer/tr_shade.cpp
  renderer/tr_shade_calc.cpp
  renderer/tr_shader.cpp
  renderer/tr_shadows.cpp
  renderer/tr_sky.cpp
  renderer/tr_stb.cpp
  renderer/tr_stl.cpp
  renderer/tr_surface.cpp
  renderer/tr_surfacesprites.cpp
  renderer/tr_terrain.cpp
  renderer/tr_world.cpp
  rmg/RM_Area.cpp
  rmg/RM_Instance.cpp
  rmg/RM_InstanceFile.cpp
  rmg/RM_Instance_BSP.cpp
  rmg/RM_Instance_Group.cpp
  rmg/RM_Instance_Random.cpp
  rmg/RM_Instance_Void.cpp
  rmg/RM_Manager.cpp
  rmg/RM_Mission.cpp
  rmg/RM_Objective.cpp
  rmg/RM_Path.cpp
  rmg/RM_Terrain.cpp
  server/exe_headers.cpp
  server/sv_ccmds.cpp
  server/sv_client.cpp
  server/sv_game.cpp
  server/sv_init.cpp
  server/sv_main.cpp
  server/sv_savegame.cpp
  server/sv_snapshot.cpp
  server/sv_world.cpp
  sys/sys_console.cpp
  sys/sys_files.cpp
  sys/sys_gamepad.cpp
  sys/sys_input.cpp
  sys/sys_main.cpp
  sys/sys_module.cpp
  sys/sys_null_sound.cpp
  sys/sys_null_window.cpp
  sys/sys_qgl.cpp
  sys/sys_sound.cpp
  sys/sys_window.cpp
  ui/ui_atoms.cpp
  ui/ui_connect.cpp
  ui/ui_main.cpp
  ui/ui_saber.cpp
  ui/ui_shared.cpp
  ui/ui_syscalls.cpp
)
target_compile_definitions(jasp PRIVATE
  _JK2EXE
  # miniz is used under its own names. Its zlib aliases are macros (crc32, compress,
  # ...) and would rename unrelated identifiers wherever its header is included.
  MINIZ_NO_ZLIB_COMPATIBLE_NAMES
)
target_link_libraries(jasp PRIVATE miniz SDL3::SDL3)
jasp_legacy_target(jasp)

# The platform layer is new code and is held to the warnings of new code. They are not
# errors here, because its files include headers of the legacy code.
set(jasp_sys_sources
  sys/sys_console.cpp
  sys/sys_files.cpp
  sys/sys_gamepad.cpp
  sys/sys_input.cpp
  sys/sys_main.cpp
  sys/sys_module.cpp
  sys/sys_null_sound.cpp
  sys/sys_null_window.cpp
  sys/sys_qgl.cpp
  sys/sys_sound.cpp
  sys/sys_window.cpp
)
if(MSVC)
  set_source_files_properties(${jasp_sys_sources} PROPERTIES COMPILE_OPTIONS "/W4")
else()
  set_source_files_properties(${jasp_sys_sources} PROPERTIES
    COMPILE_OPTIONS "-Wall;-Wextra;-Wpedantic;-Wshadow;-Wconversion")
endif()

# game, cgame and ICARUS. The file name states the architecture: jagamex86_64, jagamearm64.
add_library(jagame MODULE
  cgame/FX_ATSTMain.cpp
  cgame/FX_Blaster.cpp
  cgame/FX_Bowcaster.cpp
  cgame/FX_BryarPistol.cpp
  cgame/FX_Concussion.cpp
  cgame/FX_DEMP2.cpp
  cgame/FX_Disruptor.cpp
  cgame/FX_Emplaced.cpp
  cgame/FX_Flechette.cpp
  cgame/FX_HeavyRepeater.cpp
  cgame/FX_NoghriShot.cpp
  cgame/FX_RocketLauncher.cpp
  cgame/FX_TuskenShot.cpp
  cgame/FxPrimitives.cpp
  cgame/FxScheduler.cpp
  cgame/FxSystem.cpp
  cgame/FxTemplate.cpp
  cgame/FxUtil.cpp
  cgame/cg_camera.cpp
  cgame/cg_consolecmds.cpp
  cgame/cg_credits.cpp
  cgame/cg_draw.cpp
  cgame/cg_drawtools.cpp
  cgame/cg_effects.cpp
  cgame/cg_ents.cpp
  cgame/cg_event.cpp
  cgame/cg_headers.cpp
  cgame/cg_info.cpp
  cgame/cg_lights.cpp
  cgame/cg_localents.cpp
  cgame/cg_main.cpp
  cgame/cg_marks.cpp
  cgame/cg_players.cpp
  cgame/cg_playerstate.cpp
  cgame/cg_predict.cpp
  cgame/cg_scoreboard.cpp
  cgame/cg_servercmds.cpp
  cgame/cg_snapshot.cpp
  cgame/cg_syscalls.cpp
  cgame/cg_text.cpp
  cgame/cg_view.cpp
  cgame/cg_weapons.cpp
  game/AI_Animal.cpp
  game/AI_AssassinDroid.cpp
  game/AI_Atst.cpp
  game/AI_BobaFett.cpp
  game/AI_Civilian.cpp
  game/AI_Default.cpp
  game/AI_Droid.cpp
  game/AI_GalakMech.cpp
  game/AI_Glider.cpp
  game/AI_Grenadier.cpp
  game/AI_HazardTrooper.cpp
  game/AI_Howler.cpp
  game/AI_ImperialProbe.cpp
  game/AI_Interrogator.cpp
  game/AI_Jedi.cpp
  game/AI_Mark1.cpp
  game/AI_Mark2.cpp
  game/AI_MineMonster.cpp
  game/AI_Rancor.cpp
  game/AI_Remote.cpp
  game/AI_RocketTrooper.cpp
  game/AI_SaberDroid.cpp
  game/AI_SandCreature.cpp
  game/AI_Seeker.cpp
  game/AI_Sentry.cpp
  game/AI_Sniper.cpp
  game/AI_Stormtrooper.cpp
  game/AI_Tusken.cpp
  game/AI_Utils.cpp
  game/AI_Wampa.cpp
  game/AnimalNPC.c
  game/FighterNPC.c
  game/G_Timer.cpp
  game/NPC.cpp
  game/NPC_behavior.cpp
  game/NPC_combat.cpp
  game/NPC_goal.cpp
  game/NPC_misc.cpp
  game/NPC_move.cpp
  game/NPC_reactions.cpp
  game/NPC_senses.cpp
  game/NPC_sounds.cpp
  game/NPC_spawn.cpp
  game/NPC_stats.cpp
  game/NPC_utils.cpp
  game/Q3_Interface.cpp
  game/SpeederNPC.c
  game/WalkerNPC.c
  game/bg_misc.cpp
  game/bg_pangles.cpp
  game/bg_panimate.cpp
  game/bg_pmove.cpp
  game/bg_slidemove.cpp
  game/bg_vehicleload.c
  game/g_active.cpp
  game/g_breakable.cpp
  game/g_camera.cpp
  game/g_client.cpp
  game/g_cmds.cpp
  game/g_combat.cpp
  game/g_emplaced.cpp
  game/g_functions.cpp
  game/g_fx.cpp
  game/g_headers.cpp
  game/g_inventory.cpp
  game/g_itemLoad.cpp
  game/g_items.cpp
  game/g_main.cpp
  game/g_mem.cpp
  game/g_misc.cpp
  game/g_misc_model.cpp
  game/g_missile.cpp
  game/g_mover.cpp
  game/g_nav.cpp
  game/g_navigator.cpp
  game/g_navnew.cpp
  game/g_object.cpp
  game/g_objectives.cpp
  game/g_rail.cpp
  game/g_ref.cpp
  game/g_roff.cpp
  game/g_savegame.cpp
  game/g_savetables.cpp
  game/g_session.cpp
  game/g_spawn.cpp
  game/g_svcmds.cpp
  game/g_target.cpp
  game/g_trigger.cpp
  game/g_turret.cpp
  game/g_usable.cpp
  game/g_utils.cpp
  game/g_vehicles.c
  game/g_weapon.cpp
  game/g_weaponLoad.cpp
  game/genericparser2.cpp
  game/q_math.cpp
  game/q_shared.cpp
  game/wp_saber.cpp
  game/wp_saberLoad.cpp
  icarus/BlockStream.cpp
  icarus/IcarusImplementation.cpp
  icarus/Sequence.cpp
  icarus/Sequencer.cpp
  icarus/TaskManager.cpp
  qcommon/sg_fields.cpp
  qcommon/tri_coll_test.cpp
  ratl/ratl.cpp
  ravl/CBounds.cpp
  ravl/CVec.cpp
  rufl/hfile.cpp
  rufl/hstring.cpp
  rufl/random.cpp
  ui/gameinfo.cpp
)
set_target_properties(jagame PROPERTIES
  PREFIX ""
  OUTPUT_NAME "jagame${JASP_ARCHITECTURE}"
  # The engine and the module define many functions of the same name. Only the two
  # entry points, marked JAGAME_EXPORT, are visible outside the module.
  C_VISIBILITY_PRESET hidden
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON
)
# <jagame_export.h>, which defines JAGAME_EXPORT for the compiler in use.
include(GenerateExportHeader)
generate_export_header(jagame)
target_include_directories(jagame PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
jasp_legacy_target(jagame)

# The engine loads the module from its own directory, under the name it was built with.
target_compile_definitions(jasp PRIVATE "JASP_GAME_MODULE_FILE=\"$<TARGET_FILE_NAME:jagame>\"")
add_dependencies(jasp jagame)

# C++ in files named .c: the vehicle code, which the release shared with the
# multiplayer game module.
set_source_files_properties(
  game/AnimalNPC.c
  game/FighterNPC.c
  game/SpeederNPC.c
  game/WalkerNPC.c
  game/bg_vehicleload.c
  game/g_vehicles.c
  PROPERTIES LANGUAGE CXX
)

if(BUILD_TESTING)
  # Both need the retail game data and are skipped without it (docs/design.md,
  # section 7). The second one also needs an OpenGL context without a display.
  foreach(variant IN ITEMS "smoke-load-map none" "smoke-render-map offscreen")
    separate_arguments(variant)
    list(GET variant 0 name)
    list(GET variant 1 window)
    add_test(
      NAME ${name}
      COMMAND "${CMAKE_COMMAND}"
        "-DJASP=$<TARGET_FILE:jasp>"
        "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/${name}"
        -DMAP=yavin1b
        -DFRAMES=120
        -DWINDOW=${window}
        -P "${PROJECT_SOURCE_DIR}/cmake/SmokeTest.cmake"
    )
    set_tests_properties(${name} PROPERTIES
      SKIP_REGULAR_EXPRESSION "^SKIPPED"
      TIMEOUT 900
    )
  endforeach()
endif()
