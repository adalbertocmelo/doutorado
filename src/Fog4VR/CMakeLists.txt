build_lib(
  LIBNAME Fog4VR
  SOURCE_FILES model/festive.cc
        model/panda.cc
        model/tobasco2.cc
        model/tcp-stream-adaptation-algorithm.cc
        model/tcp-stream-client.cc
        model/tcp-stream-server.cc
        model/tcp-stream-controller.cc
        model/fog4vr-utils.cc
        helper/tcp-stream-helper.cc
  HEADER_FILES  model/festive.h
        model/panda.h
        model/tobasco2.h
        model/tcp-stream-adaptation-algorithm.h
        model/tcp-stream-client.h
        model/tcp-stream-server.h
        model/tcp-stream-interface.h
        model/tcp-stream-controller.h
        model/fog4vr-utils.h
        helper/tcp-stream-helper.h
  LIBRARIES_TO_LINK
      ${libcore}
      ${libinternet}
      ${libwave}
      ${libflow-monitor}
)
