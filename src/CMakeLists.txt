add_executable(${PROJECT_NAME} main.cpp matrix.h matrix.cpp commonUtils.h camera.h camera.cpp EMP_Logo.h EMP_Logo_Alpha.h light.h light.cpp terrain.h terrain.cpp terrainTextures.h memory.h memory.cpp texture.h texture.cpp benchmark.h benchmark.cpp)
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".elf")

set(CMAKE_VERBOSE_MAKEFILE ON)

set(TITLE_ID "NTVRNDR02")

# Option to use our vita-cg-compiler instead of psp2cgc
option(USE_VITA_CG_COMPILER "Use vita-cg-compiler instead of psp2cgc" OFF)
set(VITA_CG_COMPILER_PATH "" CACHE PATH "Path to vita-cg-compiler executable")

# Validate vita-cg-compiler path if option is enabled
if(USE_VITA_CG_COMPILER)
    if(NOT VITA_CG_COMPILER_PATH)
        message(FATAL_ERROR "USE_VITA_CG_COMPILER is ON but VITA_CG_COMPILER_PATH is not set. "
                           "Please set -DVITA_CG_COMPILER_PATH=/path/to/vita-cg-compiler.exe")
    endif()
    if(NOT EXISTS "${VITA_CG_COMPILER_PATH}")
        message(FATAL_ERROR "vita-cg-compiler not found at: ${VITA_CG_COMPILER_PATH}")
    endif()
    message(STATUS "Using vita-cg-compiler: ${VITA_CG_COMPILER_PATH}")
endif()

if(SAFE_HOMEBREW)
    set(VITA_FSELF_SAFE_FLAG -s)
    # Clear ATTRIBUTE fields in param.sfo so the package doesn't request extended permissions
    set(VITA_MKSFOEX_SAFE_FLAGS -d ATTRIBUTE=0x0 -d ATTRIBUTE2=0x0 -s)
endif()

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(${PROJECT_NAME} PUBLIC ${VITASDK}/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${VITASDK}/arm-vita-eabi/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${VITASDK}/arm-vita-eabi/include/c++/10.3.0)

link_directories(${PSVITA_LIBS_PATH})

# generate shader targets
file(GLOB SHADER_SRC "${CMAKE_SOURCE_DIR}/Shaders/*_v.cg" "${CMAKE_SOURCE_DIR}/Shaders/*_f.cg")
foreach(shader ${SHADER_SRC})
    string(REPLACE ".cg" ".gxp" shader_gxp ${shader})
    string(REPLACE "Shaders/" "out/shaders/" shader_o ${shader_gxp})
    string(REPLACE ".gxp" "_gxp.o" shader_o ${shader_o})

    get_filename_component(SHADER_FILENAME_WITH_EXTENSION ${shader} NAME)
    get_filename_component(SHADER_GXP_FILENAME ${shader_gxp} NAME)

    if(BUILD_SHADERS)
        # determine if vertex or fragment shader
        if(shader MATCHES "_v\\.cg$")
            set(SHADER_PROFILE "sce_vp_psp2")
        else()
            set(SHADER_PROFILE "sce_fp_psp2")
        endif()

        if(USE_VITA_CG_COMPILER)
            # Compile using our vita-cg-compiler
            add_custom_command(
                OUTPUT ${shader_gxp}
                COMMAND ${CMAKE_COMMAND} -E echo ">> ${VITA_CG_COMPILER_PATH} -profile ${SHADER_PROFILE} -O3 -o ${shader_gxp} ${SHADER_FILENAME_WITH_EXTENSION}"
                COMMAND "${VITA_CG_COMPILER_PATH}" -profile ${SHADER_PROFILE} -O3 -o ${shader_gxp} ${SHADER_FILENAME_WITH_EXTENSION}
                DEPENDS ${shader}
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/Shaders
                COMMENT "[vita-cg-compiler] Compiling ${SHADER_FILENAME_WITH_EXTENSION}"
                VERBATIM
            )
        else()
            # Compile using psp2cgc (reference compiler)
            add_custom_command(
                OUTPUT ${shader_gxp}
                COMMAND ${CMAKE_COMMAND} -E echo ">> psp2cgc -profile ${SHADER_PROFILE} ${SHADER_FILENAME_WITH_EXTENSION} -O3 -o ${shader_gxp}"
                COMMAND psp2cgc -profile ${SHADER_PROFILE} ${SHADER_FILENAME_WITH_EXTENSION} -O3 -o ${shader_gxp}
                DEPENDS ${shader}
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/Shaders
                COMMENT "[psp2cgc] Compiling ${SHADER_FILENAME_WITH_EXTENSION}"
                VERBATIM
            )
        endif()
    endif()

    # Objcopy gxp to object file (uses precompiled gxp when BUILD_SHADERS is OFF)
    add_custom_command(
        OUTPUT ${shader_o}
        COMMAND arm-vita-eabi-objcopy --input-target binary --output-target elf32-littlearm --binary-architecture arm --set-section-alignment .data=4 ${SHADER_GXP_FILENAME} ${shader_o}
        DEPENDS ${shader_gxp}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/Shaders
        COMMENT "Objcopying ${shader_gxp} to ${shader_o}"
    )

    list(APPEND SHADER_OBJS ${shader_o})
endforeach()

add_custom_target(Shaders DEPENDS ${SHADER_OBJS})
add_dependencies(${PROJECT_NAME} Shaders)
target_link_libraries(${PROJECT_NAME} PRIVATE -Wl,-q -lm ${SHADER_OBJS} SceDisplay_stub SceGxm_stub SceCtrl_stub SceRtc_stub SceIofilemgr_stub)

add_custom_command(OUTPUT EBOOT.BIN
	COMMAND vita-elf-create $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.velf
	COMMAND vita-make-fself ${VITA_FSELF_SAFE_FLAG} ${PROJECT_NAME}.velf EBOOT.BIN
	DEPENDS ${PROJECT_NAME}
	COMMENT "Creating EBOOT.BIN"
)

add_custom_command(OUTPUT "${PROJECT_NAME}.vpk"
	COMMAND vita-mksfoex ${VITA_MKSFOEX_SAFE_FLAGS} TITLE_ID=${TITLE_ID} "${PROJECT_NAME}" param.sfo
	COMMAND vita-pack-vpk ${VITA_FSELF_SAFE_FLAG} param.sfo -b EBOOT.BIN ${PROJECT_NAME}.vpk
	DEPENDS EBOOT.BIN
	COMMENT "Creating ${PROJECT_NAME}.vpk"
)

# Custom targets
add_custom_target(
    CreateEBOOT ALL
    DEPENDS EBOOT.BIN
)

add_custom_target(
    CreateVPK ALL
    DEPENDS ${PROJECT_NAME}.vpk
)
