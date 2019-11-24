
#-----------------------------------------------------------------------------
set(MODULE_NAME SegmentEditorNvidiaAIAA)

# We create a python module from client_api.py file
CONFIGURE_FILE(../../py-client/client_api.py
  ${CMAKE_CURRENT_SOURCE_DIR}/NvidiaAIAAClientAPI/client_api.py 
  COPYONLY
  )

#-----------------------------------------------------------------------------
set(MODULE_PYTHON_SCRIPTS
  ${MODULE_NAME}.py
  ${MODULE_NAME}Lib/__init__.py
  ${MODULE_NAME}Lib/SegmentEditorEffect.py
  NvidiaAIAAClientAPI/__init__.py
  NvidiaAIAAClientAPI/client_api.py
  )

set(MODULE_PYTHON_RESOURCES
  ${MODULE_NAME}Lib/SegmentEditorEffect.png
  )

#-----------------------------------------------------------------------------
slicerMacroBuildScriptedModule(
  NAME ${MODULE_NAME}
  SCRIPTS ${MODULE_PYTHON_SCRIPTS}
  RESOURCES ${MODULE_PYTHON_RESOURCES}
  WITH_GENERIC_TESTS
  )

#-----------------------------------------------------------------------------
if(BUILD_TESTING)
  # Register the unittest subclass in the main script as a ctest.
  # Note that the test will also be available at runtime.
  #slicer_add_python_unittest(SCRIPT ${MODULE_NAME}.py)

  # Additional build-time testing
  #add_subdirectory(Testing)
endif()
