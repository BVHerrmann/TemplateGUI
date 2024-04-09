message( "Inspector - ${PROJECT_NAME}" )

add_definitions( -DTARGET="${PROJECT_NAME}" -DVERSION="${PROJECT_VERSION}" -DYEAR=${YEAR} )

find_package( Qt5 COMPONENTS Core Widgets Network PrintSupport Qml Quick QuickWidgets SerialPort Sql VirtualKeyboard REQUIRED )
find_package( QtSingleApplication )

# pull and push scripts
add_subtree_actions( "inspector" "inspector" "git@github.com:BertramBildverarbeitung/app-inspector.git" )

file(GLOB INSPECTOR_SRC
  "*.h"
  "*.cpp"
  "*.css"
  "*.qrc"
)

if ( APPLE )
  set_source_files_properties( "icon.icns" PROPERTIES MACOSX_PACKAGE_LOCATION Resources )
  
  list(REMOVE_ITEM INSPECTOR_SRC "${CMAKE_CURRENT_SOURCE_DIR}/crashhandler.h")
  list(REMOVE_ITEM INSPECTOR_SRC "${CMAKE_CURRENT_SOURCE_DIR}/crashhandler.cpp")
  
  add_executable (Inspector MACOSX_BUNDLE
      ${INSPECTOR_SRC}
      icon.icns
  )
  
  set_target_properties( Inspector PROPERTIES
      MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/MacOSXBundleInfo.plist.in"
      MACOSX_BUNDLE_BUNDLE_NAME "${PROJECT_NAME}"
      MACOSX_BUNDLE_GUI_IDENTIFIER "de.bertram-bildverarbeitung.${PROJECT_NAME}"
      MACOSX_BUNDLE_ICON_FILE "icon.icns"
      MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
      MACOSX_BUNDLE_INFO_STRING "${PROJECT_NAME} ${PROJECT_VERSION}, © 2000–${YEAR} Bertram Bildverarbeitung GmbH. All rights reserved."
      MACOSX_BUNDLE_COPYRIGHT "${MACOSX_BUNDLE_INFO_STRING}"
      MACOSX_BUNDLE_LONG_VERSION_STRING "${MACOSX_BUNDLE_BUNDLE_VERSION}"
      MACOSX_BUNDLE_SHORT_VERSION_STRING "${MACOSX_BUNDLE_BUNDLE_VERSION}"
  )
else ( WIN32 )
  set( _VER_DESCRIPTION "${PROJECT_NAME}" )
  set( _VER_FILENAME "${PROJECT_NAME}.exe" )
  configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/common/VersionInfo.rc.in
    ${CMAKE_CURRENT_BINARY_DIR}/VersionInfo.rc
    @ONLY )

  add_executable (Inspector WIN32
    inspector.rc
    ${CMAKE_CURRENT_BINARY_DIR}/VersionInfo.rc
	${INSPECTOR_SRC}
  )
  set_property( DIRECTORY "${PROJECT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT Inspector )
  
  target_compile_definitions( Inspector PRIVATE
    _UNICODE
    UNICODE
  )
endif ( )
set_target_properties( Inspector PROPERTIES OUTPUT_NAME ${PROJECT_NAME} VERSION ${PROJECT_VERSION} )

# translations
file( GLOB TS_FILES
    "*.ts"
)
add_translations( Inspector ${TS_FILES} )

if( PLUGINS )
    add_dependencies(Inspector ${PLUGINS} )
    foreach(plugin ${PLUGINS} )
        get_target_property( _linked_libraries ${plugin} LINK_LIBRARIES )

        list( FIND _linked_libraries CVL _cvl_pos )
        if ( _cvl_pos GREATER -1 )
            add_definitions( -DCVL_USED )
            target_link_libraries( Inspector CVL )
        endif()
    endforeach()
endif( )
target_link_libraries( Inspector QtSingleApplication bg Qt5::Network Qt5::Widgets Qt5::Qml Qt5::Quick Qt5::QuickWidgets Qt5::SerialPort Qt5::Sql Qt5::VirtualKeyboard )
if ( WIN32 )
	target_link_libraries( Inspector Breakpad )
endif()
if ( VLD_FOUND )
	target_link_libraries( Inspector debug VLD )
endif ()

# copy Qt libraries
if ( APPLE )
    set_target_properties( Inspector PROPERTIES XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@loader_path/../Frameworks ${Qt5_DIR}/../../../lib ${CMAKE_BINARY_DIR}/$(CONFIGURATION) ${CMAKE_BINARY_DIR}/$(CONFIGURATION)/lib" )
elseif ( WIN32 )
    # libs and dependencies
    foreach( _QT_LIB ${Qt5_COMPONENTS})
        set( _QT_COMPONENT "Qt5${_QT_LIB}" )
	
        get_target_property( _loc "Qt5::${_QT_LIB}" LOCATION)
        get_filename_component( _QT_LIB_NAME ${_loc} NAME_WE )
        get_filename_component( _QT_LIB_EXT ${_loc} EXT )
        get_filename_component( _QT_LIB_DIRECTORY ${_loc} DIRECTORY )
        
        add_custom_command(	TARGET Inspector
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_QT_LIB_DIRECTORY}/${_QT_LIB_NAME}$<$<CONFIG:Debug>:d>${_QT_LIB_EXT}"
                "${CMAKE_BINARY_DIR}/$<CONFIGURATION>/" )
        install( FILES "${_QT_LIB_DIRECTORY}/${_QT_LIB_NAME}$<$<CONFIG:Debug>:d>${_QT_LIB_EXT}"
            DESTINATION . COMPONENT Runtime )
        
        foreach( plugin ${${_QT_COMPONENT}_PLUGINS} )
            get_target_property(_loc ${plugin} LOCATION)
            get_filename_component( QT_PLUGIN ${_loc} NAME_WE )
            get_filename_component( QT_PLUGIN_DIRECTORY ${_loc} DIRECTORY )
            get_filename_component( QT_PLUGIN_DIRECTORY ${QT_PLUGIN_DIRECTORY} NAME_WE )
            
            add_custom_command(	TARGET Inspector
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                    "${CMAKE_BINARY_DIR}/$<CONFIGURATION>/${QT_PLUGIN_DIRECTORY}/" )
            add_custom_command(	TARGET Inspector
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${Qt5_DIR}/../../../plugins/${QT_PLUGIN_DIRECTORY}/${QT_PLUGIN}$<$<CONFIG:Debug>:d>.dll"
                    "${CMAKE_BINARY_DIR}/$<CONFIGURATION>/${QT_PLUGIN_DIRECTORY}/" )
                install (FILES "${Qt5_DIR}/../../../plugins/${QT_PLUGIN_DIRECTORY}/${QT_PLUGIN}$<$<CONFIG:Debug>:d>.dll"
                    DESTINATION ${QT_PLUGIN_DIRECTORY} COMPONENT Runtime )
        endforeach()
    endforeach()
    
    # qml
    file ( GLOB _QT_QML_DIRS "${Qt5_DIR}/../../../qml/*"
        LIST_DIRECTORIES true )
        
    foreach( _QT_QML_DIR ${_QT_QML_DIRS} )
        if( IS_DIRECTORY ${_QT_QML_DIR} )
            get_filename_component( _DIR ${_QT_QML_DIR} NAME )
		  
            add_custom_command(	TARGET Inspector
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                    "${CMAKE_BINARY_DIR}/$<CONFIGURATION>/${_DIR}/" )
									
            file ( GLOB_RECURSE _QT_QML_FILES
                LIST_DIRECTORIES true
                RELATIVE "${_QT_QML_DIR}"
                "${_QT_QML_DIR}/*" )
			
            # remove all debug dlls
            foreach( _QT_QML_FILE ${_QT_QML_FILES})
                get_filename_component( _EXT ${_QT_QML_FILE} EXT )
                if ( _EXT STREQUAL ".dll" )
                    get_filename_component( _DIRECTORY ${_QT_QML_FILE} DIRECTORY )
					get_filename_component( _NAME ${_QT_QML_FILE} NAME_WE )
					if ( _DIRECTORY )
						list( REMOVE_ITEM _QT_QML_FILES "${_DIRECTORY}/${_NAME}d${_EXT}" )
					else()
						list( REMOVE_ITEM _QT_QML_FILES "${_NAME}d${_EXT}" )
					endif()
				endif()
			endforeach()
			
		  	foreach( _QT_QML_FILE ${_QT_QML_FILES})
				if( IS_DIRECTORY "${_QT_QML_DIR}/${_QT_QML_FILE}" )
					add_custom_command(	TARGET Inspector
                        POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E make_directory
                            "${CMAKE_BINARY_DIR}/$<CONFIGURATION>/${_DIR}/${_QT_QML_FILE}/" )
				else()
					get_filename_component( _EXT ${_QT_QML_FILE} EXT )
					if ( _EXT STREQUAL ".dll" )
						get_filename_component( _DIRECTORY ${_QT_QML_FILE} DIRECTORY )
						get_filename_component( _NAME ${_QT_QML_FILE} NAME_WE )
			  			add_custom_command(	TARGET Inspector
                            POST_BUILD
                            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                                "${_QT_QML_DIR}/${_DIRECTORY}/${_NAME}$<$<CONFIG:Debug>:d>.dll"
                                "${CMAKE_BINARY_DIR}/$<CONFIGURATION>/${_DIR}/${_DIRECTORY}/${_NAME}$<$<CONFIG:Debug>:d>.dll" )
						install (FILES "${_QT_QML_DIR}/${_DIRECTORY}/${_NAME}$<$<CONFIG:Debug>:d>.dll"
                            DESTINATION "${_DIR}/${_DIRECTORY}" COMPONENT Runtime)
					else()
						get_filename_component( _DIRECTORY ${_QT_QML_FILE} DIRECTORY )
						
			  			add_custom_command(	TARGET Inspector
                            POST_BUILD
                            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                                "${_QT_QML_DIR}/${_QT_QML_FILE}"
                                "${CMAKE_BINARY_DIR}/$<CONFIGURATION>/${_DIR}/${_DIRECTORY}" )
						install( FILES "${_QT_QML_DIR}/${_QT_QML_FILE}"
                            DESTINATION "${_DIR}/${_DIRECTORY}"
                            COMPONENT Runtime )
					endif()
				endif()
			endforeach()
	  endif()
  endforeach()
endif ( )

install(TARGETS Inspector
	RUNTIME DESTINATION . COMPONENT Runtime
	BUNDLE DESTINATION . COMPONENT Runtime
)
if ( WIN32 )
	install( FILES $<TARGET_PDB_FILE:Inspector> DESTINATION pdb COMPONENT Debug )
endif ( )

# directories to look for dependencies
if ( WIN32 )
	SET( APPS "\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.exe" )
	SET( DIRS ${QT_DIR}/bin ${CMAKE_BINARY_DIR}/\${CMAKE_INSTALL_CONFIG_NAME} "C:/Program Files (x86)/Windows Kits/10/Redist/ucrt/DLLs/${VS_ARCH}" "C:/Program Files (x86)/Microsoft Visual Studio ${VS_VERSION}.0/VC/redist/${VS_ARCH}/Microsoft.VC${VS_VERSION}0.CRT" )

	# Now the work of copying dependencies into the bundle/package
	INSTALL(CODE "
		foreach(PLUGIN ${PLUGINS})
			get_filename_component(PLUGIN_PATH \"\${CMAKE_INSTALL_PREFIX}/\${PLUGIN}${CMAKE_SHARED_LIBRARY_SUFFIX}\" ABSOLUTE)
			list ( APPEND LIBS \${PLUGIN_PATH} )
		endforeach()
		include(BundleUtilities)
		fixup_bundle(\"${APPS}\" \"\${LIBS}\" \"${DIRS}\")
		verify_app( \"${APPS}\" )
    " COMPONENT Runtime)
endif ( )
