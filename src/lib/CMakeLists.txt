add_library(libraid6
			libraid6.cpp
			string_utils.cpp)
			
find_path(LIBGFCOMPLETE_INCLUDE_DIR NAMES gf_complete.h PATHS ${PROJECT_SOURCE_DIR}/external/gf-complete/include NO_DEFAULT_PATH)
find_path(LIBJERASURE_INCLUDE_DIR NAMES jerasure.h PATHS ${PROJECT_SOURCE_DIR}/external/jerasure/include NO_DEFAULT_PATH)
find_library(LIBGFCOMPLETE_LIBRARY libgf_complete PATHS ${PROJECT_SOURCE_DIR}/external/gf-complete/src/.libs/ REQUIRED)
find_path(JERASURE_LIBDIR NAMES libJerasure.lib PATHS ${PROJECT_SOURCE_DIR}/external/jerasure/src/.libs)
find_library(LIBJERASURE_LIBRARY libJerasure  PATHS ${JERASURE_LIBDIR} REQUIRED)
find_library(LIBMINGWEX_LIBRARY libmingwex PATHS ${JERASURE_LIBDIR} REQUIRED)
find_library(LIBGCC_LIBRARY libgcc PATHS ${JERASURE_LIBDIR} REQUIRED)

target_link_libraries(libraid6 INTERFACE 
							${LIBGFCOMPLETE_LIBRARY}
							${LIBJERASURE_LIBRARY}
							${LIBMINGWEX_LIBRARY}
							${LIBGCC_LIBRARY})

target_include_directories(libraid6 PUBLIC include/libraid6)
target_include_directories(libraid6 PRIVATE 
									${CMAKE_CURRENT_LIST_DIR}/include 
									${LIBGFCOMPLETE_INCLUDE_DIR}
									${LIBJERASURE_INCLUDE_DIR})
