# Verifies that every release-facing version surface matches EXPECTED_VERSION.
# This file is included during normal configuration and can also run via:
#   cmake -DROOT_DIR=<root> -DEXPECTED_VERSION=<x.y.z> -P cmake/VerifyVersionConsistency.cmake

function(mw_verify_version_consistency root_dir expected_version)
    if(NOT expected_version MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
        message(FATAL_ERROR "Invalid expected semantic version: '${expected_version}'")
    endif()

    string(REPLACE "." ";" version_parts "${expected_version}")
    list(GET version_parts 0 version_major)
    list(GET version_parts 1 version_minor)
    list(GET version_parts 2 version_patch)

    set(expected_rc_numeric "${version_major},${version_minor},${version_patch},0")
    set(expected_manifest "${expected_version}.0")

    set(version_files
        "${root_dir}/CMakeLists.txt"
        "${root_dir}/src/App/resources.rc"
        "${root_dir}/src/App/app.manifest"
        "${root_dir}/scripts/build-release.ps1"
        "${root_dir}/installer/MandelbrotWallpaper.iss"
    )
    foreach(version_file IN LISTS version_files)
        if(NOT EXISTS "${version_file}")
            message(FATAL_ERROR "Version consistency check requires missing file: ${version_file}")
        endif()
    endforeach()

    file(READ "${root_dir}/CMakeLists.txt" cmake_text)
    file(READ "${root_dir}/src/App/resources.rc" rc_text)
    file(READ "${root_dir}/src/App/app.manifest" manifest_text)
    file(READ "${root_dir}/scripts/build-release.ps1" release_script_text)
    file(READ "${root_dir}/installer/MandelbrotWallpaper.iss" installer_text)

    set(failures "")

    string(FIND "${cmake_text}" "project(MandelbrotLiveWallpaper VERSION ${expected_version}" cmake_version_index)
    if(cmake_version_index EQUAL -1)
        list(APPEND failures "CMakeLists.txt must declare MandelbrotLiveWallpaper VERSION ${expected_version}")
    endif()

    foreach(required_rc_token IN ITEMS
        "FILEVERSION ${expected_rc_numeric}"
        "PRODUCTVERSION ${expected_rc_numeric}"
        "VALUE \"FileVersion\", \"${expected_version}\\0\""
        "VALUE \"ProductVersion\", \"${expected_version}\\0\""
    )
        string(FIND "${rc_text}" "${required_rc_token}" token_index)
        if(token_index EQUAL -1)
            list(APPEND failures "src/App/resources.rc missing: ${required_rc_token}")
        endif()
    endforeach()

    string(FIND "${manifest_text}" "version=\"${expected_manifest}\"" manifest_index)
    if(manifest_index EQUAL -1)
        list(APPEND failures "src/App/app.manifest must contain assemblyIdentity version=\"${expected_manifest}\"")
    endif()

    string(FIND "${release_script_text}" "\$Version = '${expected_version}'" release_index)
    if(release_index EQUAL -1)
        list(APPEND failures "scripts/build-release.ps1 must set \$Version = '${expected_version}'")
    endif()

    string(FIND "${installer_text}" "#define AppVersion \"${expected_version}\"" installer_index)
    if(installer_index EQUAL -1)
        list(APPEND failures "installer/MandelbrotWallpaper.iss must define AppVersion \"${expected_version}\"")
    endif()

    if(failures)
        list(JOIN failures "\n- " failure_text)
        message(FATAL_ERROR
            "Release version mismatch. Expected ${expected_version} on every version surface:\n- ${failure_text}")
    endif()

    message(STATUS "Release version surfaces are consistent at ${expected_version}.")
endfunction()

if(CMAKE_SCRIPT_MODE_FILE)
    if(NOT DEFINED ROOT_DIR OR ROOT_DIR STREQUAL "")
        message(FATAL_ERROR "ROOT_DIR is required in script mode.")
    endif()
    if(NOT DEFINED EXPECTED_VERSION OR EXPECTED_VERSION STREQUAL "")
        message(FATAL_ERROR "EXPECTED_VERSION is required in script mode.")
    endif()
    mw_verify_version_consistency("${ROOT_DIR}" "${EXPECTED_VERSION}")
endif()
