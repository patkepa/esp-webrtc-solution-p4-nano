idf_component_register(SRCS "webrtc.c"  "main.c" "board.c" "media_sys.c"
                       EMBED_TXTFILES "join.aac"
                       INCLUDE_DIRS "."
                       REQUIRES esp_webrtc
                                common
                                console
                                codec_board
                                av_render
                                media_lib_sal
                                esp_peer
                                espressif__esp_capture
                                espressif__esp_video_codec
                                espressif__esp_codec_dev)
