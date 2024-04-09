get_filename_component( _PLUGIN ${CMAKE_CURRENT_LIST_DIR} NAME )

message( "Plugin - ${_PLUGIN}" )

file( GLOB PLUGIN_SRC
	"*.h"
	"*.cpp"
)

if ( WIN32 )
		set( _VER_DESCRIPTION "${PROJECT_NAME} Plugin" )
		set( _VER_FILENAME "${_PLUGIN}.dll" )
		configure_file(
			${CMAKE_SOURCE_DIR}/cmake/common/VersionInfo.rc.in
			${CMAKE_CURRENT_BINARY_DIR}/VersionInfo.rc
			@ONLY )
		list( APPEND PLUGIN_SRC ${CMAKE_CURRENT_BINARY_DIR}/VersionInfo.rc )

	add_library( ${_PLUGIN} SHARED ${PLUGIN_SRC} )
	target_link_libraries( ${_PLUGIN} bg PNDriver Qt5::Widgets Qt5::Concurrent )

	# translations
	file( GLOB TS_FILES
	    "*.ts"
	)
	add_translations( ${_PLUGIN} ${TS_FILES} )

	# install
	install( TARGETS ${_PLUGIN}
		RUNTIME DESTINATION . COMPONENT Runtime
		LIBRARY DESTINATION . COMPONENT Runtime )
	install( FILES $<TARGET_PDB_FILE:${_PLUGIN}> DESTINATION pdb COMPONENT Debug )
	
else ()
	# empty target for unsupported systems
	add_library( ${_PLUGIN} SHARED )
endif ( )