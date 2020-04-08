#  -*- mode: cmake -*-

#
# Build TPL: SEACAS 
#    
# --- Define all the directories and common external project flags

# SEACAS does not call MPI directly, however HDF5 requires
# MPI and to resolve links we need MPI compile wrappers.
if (NOT ENABLE_XSDK)
    define_external_project_args(SEACAS
                                 TARGET seacas
                                 DEPENDS HDF5 NetCDF)
else()
    define_external_project_args(SEACAS
                                 TARGET seacas
                                 DEPENDS XSDK)
endif()

# add version version to the autogenerated tpl_versions.h file
amanzi_tpl_version_write(FILENAME ${TPL_VERSIONS_INCLUDE_FILE}
  PREFIX SEACAS
  VERSION ${SEACAS_VERSION_MAJOR} ${SEACAS_VERSION_MINOR} ${SEACAS_VERSION_PATCH})
  
# --- Define the configure parameters
# Compile flags
set(seacas_cflags_list -I${TPL_INSTALL_PREFIX}/include ${Amanzi_COMMON_CFLAGS})
build_whitespace_string(seacas_cflags ${seacas_cflags_list})

set(seacas_cxxflags_list -I${TPL_INSTALL_PREFIX}/include ${Amanzi_COMMON_CXXFLAGS})
build_whitespace_string(seacas_cflags ${seacas_cxxflags_list})

set(seacas_fcflags_list -I${TPL_INSTALL_PREFIX}/include ${Amanzi_COMMON_FCFLAGS})
build_whitespace_string(seacas_fcflags ${seacas_fcflags_list})

set(seacas_lflags_list)
build_whitespace_string(seacas_lflags ${seacas_lflags_list})

# determine library type
if (BUILD_SHARED_LIBS)
  set(SEACAS_LIBS_TYPE "SHARED")
else()
  set(SEACAS_LIBS_TYPE "STATIC")
endif()


# Build the NetCDF libraries string
include(BuildLibraryName)
build_library_name(netcdf seacas_netcdf_library ${SEACAS_LIBS_TYPE} APPEND_PATH ${NetCDF_DIR}/lib)
if ( ${CMAKE_BUILD_TYPE} STREQUAL "Debug" )
  build_library_name(hdf5_hl_debug seacas_hdf5_hl_library STATIC APPEND_PATH ${TPL_INSTALL_PREFIX}/lib)
  build_library_name(hdf5_debug seacas_hdf5_library STATIC APPEND_PATH ${TPL_INSTALL_PREFIX}/lib)
else()
  build_library_name(hdf5_hl seacas_hdf5_hl_library ${SEACAS_LIBS_TYPE} APPEND_PATH ${HDF5_DIR}/lib)
  build_library_name(hdf5 seacas_hdf5_library ${SEACAS_LIBS_TYPE} APPEND_PATH ${HDF5_DIR}/lib)
endif()
build_library_name(z seacas_z_library ${SEACAS_LIBS_TYPE} APPEND_PATH ${ZLIB_DIR}/lib)

set(seacas_netcdf_libraries
       ${seacas_netcdf_library}
       ${seacas_hdf5_hl_library}
       ${seacas_hdf5_library} -ldl
       ${seacas_z_library})
if ((NOT MPI_WRAPPERS_IN_USE) AND (MPI_C_LIBRARIES))
  list(APPEND seacas_netcdf_libraries ${MPI_C_LIBRARIES})
endif()

if (BUILD_SHARED_LIBS)
   set(seacas_install_rpath  "-DCMAKE_INSTALL_RPATH:PATH=${CMAKE_INSTALL_PREFIX}/SEACAS/lib"
                             "-DCMAKE_INSTALL_NAME_DIR:PATH=${CMAKE_INSTALL_PREFIX}/SEACAS/lib")
else()
   set(seacas_install_rpath "-DCMAKE_INSTALL_NAME_DIR:PATH=${CMAKE_INSTALL_PREFIX}/SEACAS/lib"
                            "-DCMAKE_SKIP_INSTALL_RPATH:BOOL=ON"
                            "-DCMAKE_SKIP_RPATH:BOOL=ON")
endif()

#
# --- Define the SEACAS patch step - mainly for nem_slice to be able
# --- to handle columns
#
set(ENABLE_SEACAS_Patch ON)
if (ENABLE_SEACAS_Patch)
  set(SEACAS_patch_file seacas-nemslice.patch)
  configure_file(${SuperBuild_TEMPLATE_FILES_DIR}/seacas-patch-step.sh.in
                 ${SEACAS_prefix_dir}/seacas-patch-step.sh
                 @ONLY)
  set(SEACAS_PATCH_COMMAND bash ${SEACAS_prefix_dir}/seacas-patch-step.sh)
  message(STATUS "Applying SEACAS patches")
else (ENABLE_SEACAS_Patch)
  set(SEACAS_PATCH_COMMAND)
  message(STATUS "Patch NOT APPLIED for SEACAS")
endif (ENABLE_SEACAS_Patch)

# --- Configure the package
set(SEACAS_CMAKE_CACHE_ARGS
                    -DCMAKE_INSTALL_PREFIX:FILEPATH=${TPL_INSTALL_PREFIX}/SEACAS
                    -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
                    -DCMAKE_EXE_LINKER_FLAGS:STRING=${seacas_lflags}
                    -DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}
                    -DSEACASProj_ENABLE_ALL_PACKAGES:BOOL=FALSE
                    -DSEACASProj_ENABLE_SEACASExodus:BOOL=TRUE
                    -DSEACASProj_ENABLE_SEACASNemslice:STRING=:BOOL=TRUE
                    -DSEACASProj_ENABLE_SEACASNemspread:STRING=:BOOL=TRUE
                    -DSEACASProj_ENABLE_SEACASExodiff:STRING=:BOOL=TRUE
                    -DSEACASProj_ENABLE_SEACASExotxt:STRING=:BOOL=TRUE
                    -DSEACASProj_ENABLE_SEACASExoformat:STRING=:BOOL=TRUE
                    -DSEACASProj_ENABLE_SEACASDecomp:STRING=:BOOL=TRUE
                    -DSEACASProj_ENABLE_ALL_OPTIONAL_PACKAGES:BOOL=FALSE
                    -DSEACASProj_ENABLE_SECONDARY_TESTED_CODE:BOOL=FALSE
                    -DSEACASProj_ENABLE_TESTS:BOOL=FALSE
                    -DSEACASProj_SKIP_FORTRANCINTERFACE_VERIFY_TEST:BOOL=ON 
                    -DSEACASProj_HIDE_DEPRECATED_CODE:STRING="NO"
                    -DTPL_ENABLE_Netcdf:BOOL=TRUE
                    -DTPL_Netcdf_LIBRARIES:STRING=${seacas_netcdf_libraries}
                    -DNetcdf_INCLUDE_DIRS:STRING=${NetCDF_INCLUDE_DIRS}
                    -DTPL_Netcdf_PARALLEL:BOOL=TRUE
                    -DTPL_ENABLE_Matio:BOOL=FALSE
                    -DTPL_ENABLE_X11:BOOL=FALSE
                    -DTPL_ENABLE_CGNS:BOOL=FALSE
                    -DTPL_ENABLE_MPI:BOOL=ON
                    -DTPL_ENABLE_Pamgen:BOOL=FALSE
                    -DTPL_ENABLE_Pthread:BOOL=FALSE
                    -DSEACASExodus_ENABLE_THREADSAFE:BOOL=OFF
                    -DSEACASIoss_ENABLE_THREADSAFE:BOOL=OFF
                    ${seacas_install_rpath})

# --- Add external project build and tie to the SEACAS build target
ExternalProject_Add(${SEACAS_BUILD_TARGET}
                    DEPENDS   ${SEACAS_PACKAGE_DEPENDS}             # Package dependency target
                    TMP_DIR   ${SEACAS_tmp_dir}                     # Temporary files directory
                    STAMP_DIR ${SEACAS_stamp_dir}                   # Timestamp and log directory
                    # -- Download and URL definitions
                    DOWNLOAD_DIR ${TPL_DOWNLOAD_DIR}                # Download directory
                    URL          ${SEACAS_URL}                      # URL may be a web site OR a local file
                    URL_MD5      ${SEACAS_MD5_SUM}                  # md5sum of the archive file
                    # -- Patch
                    PATCH_COMMAND ${SEACAS_PATCH_COMMAND}
                    # -- Configure
                    SOURCE_DIR       ${SEACAS_source_dir}           # Source directory
                    CMAKE_CACHE_ARGS ${AMANZI_CMAKE_CACHE_ARGS}     # Ensure uniform build
                                     ${SEACAS_CMAKE_CACHE_ARGS}
                                     -DCMAKE_C_FLAGS:STRING=${Amanzi_COMMON_CFLAGS}  # Ensure uniform build
                                     -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
                                     -DCMAKE_CXX_FLAGS:STRING=${Amanzi_COMMON_CXXFLAGS}
                                     -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
                                     -DCMAKE_Fortran_FLAGS:STRING=${Amanzi_COMMON_FCFLAGS}
                                     -DCMAKE_Fortran_COMPILER:FILEPATH=${CMAKE_Fortran_COMPILER}
                    # -- Build
                    BINARY_DIR       ${SEACAS_build_dir}           # Build directory 
                    BUILD_COMMAND    $(MAKE)                       # $(MAKE) enables parallel builds through make
                    BUILD_IN_SOURCE  ${SEACAS_BUILD_IN_SOURCE}     # Flag for in source builds
                    # -- Install
                    INSTALL_DIR      ${TPL_INSTALL_PREFIX}/SEACAS  # Install directory, NOT in the usual place!
                    # -- Output control
                    ${SEACAS_logging_args})

# --- Useful variables for other packages that depend on SEACAS
set(SEACAS_DIR ${TPL_INSTALL_PREFIX}/SEACAS)
