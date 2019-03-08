project(org_mitk_nvidia_aiaa)

mitk_create_plugin(
  EXPORT_DIRECTIVE NVIDIA_AIAA_EXPORT
  EXPORTED_INCLUDE_SUFFIXES src
  MODULE_DEPENDS MitkNvidiaAIAAModule
  PACKAGE_DEPENDS Poco|Net
)
