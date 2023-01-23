build_lib_example(
  NAME Fog4VR
  SOURCE_FILES Fog4VR.cc
  LIBRARIES_TO_LINK
    ${libFog4VR}
    ${libinternet}
    ${libwifi}
    ${libbuildings}
    ${libapplications}
    ${libpoint-to-point}
    ${libinternet-apps}
)

build_lib_example(
  NAME dash-migrationLTE-v2
  SOURCE_FILES dash-migrationLTE-v2.cc
  LIBRARIES_TO_LINK
    ${libFog4VR}
    ${libinternet}
    ${libwifi}
    ${libbuildings}
    ${libapplications}
    ${libpoint-to-point}
    ${libinternet-apps}
    ${liblte}
    ${libnetanim}
)
