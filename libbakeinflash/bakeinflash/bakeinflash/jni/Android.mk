# Copyright (C) 2010 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := freetype
LOCAL_SRC_FILES := libfreetype.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := avcodec
LOCAL_SRC_FILES := libavcodec.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := avformat
LOCAL_SRC_FILES := libavformat.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := avutil
LOCAL_SRC_FILES := libavutil.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := swscale
LOCAL_SRC_FILES := libswscale.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := swresamplescale
LOCAL_SRC_FILES := libswresample.a
include $(PREBUILT_STATIC_LIBRARY)

# include $(CLEAR_VARS)
# LOCAL_MODULE := ngtserver
# LOCAL_SRC_FILES := libngtserver.a
# include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE    := bakeinflash

LOCAL_SRC_FILES := main.cpp
# LOCAL_SRC_FILES += FutureDashApp.cpp

LOCAL_SRC_FILES += bakeinflash/bakeinflash_dlist.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_render_handler_ogles.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_tesselate.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_action.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_environment.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_listener.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_root.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_text.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_as_sprite.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_filters.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_log.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_shape.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_tools.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_font.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_morph2.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_sound.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_types.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_fontlib.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_movie_def.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_sound_handler_openal.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_value.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_button.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_freetype.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_mutex.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_sprite.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_video_impl.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_canvas.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_function.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_object.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_sprite_def.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_character.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_impl.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_player.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_stream.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_disasm.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_avm2_jit.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_render.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_styles.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_tcp.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_string_object.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_json.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_abc.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_avm2.cpp

LOCAL_SRC_FILES += bakeinflash/bakeinflash_as_classes/as_array.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_as_classes/as_color_transform.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_as_classes/as_global.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_as_classes/as_mcloader.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_as_classes/as_number.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_as_classes/as_string.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_as_classes/as_boolean.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_as_classes/as_date.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_as_classes/as_key.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_as_classes/as_mouse.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_as_classes/as_point.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_as_classes/as_textformat.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_as_classes/as_broadcaster.cpp
# LOCAL_SRC_FILES += bakeinflash/bakeinflash_as_classes/as_loadvars.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_as_classes/as_selection.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_as_classes/as_transform.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_as_classes/as_class.cpp
# LOCAL_SRC_FILES += bakeinflash/bakeinflash_as_classes/as_flash.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_as_classes/as_event.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_as_classes/as_event_dispatcher.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_as_classes/as_urlvariables.cpp

LOCAL_SRC_FILES += bakeinflash/bakeinflash_as_classes/as_math.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_as_classes/as_netconnection.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_as_classes/as_sharedobject.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_as_classes/as_xml.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_as_classes/as_color.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_as_classes/as_geom.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_as_classes/as_matrix.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_as_classes/as_netstream.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_as_classes/as_sound.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_as_classes/as_xmlsocket.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_as_classes/as_microphone.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_as_classes/as_iOS.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_as_classes/as_imap.cpp
LOCAL_SRC_FILES += bakeinflash/bakeinflash_as_classes/as_iaccel.cpp
#LOCAL_SRC_FILES += bakeinflash/bakeinflash_as_classes/as_camera.cpp
LOCAL_SRC_FILES += bakeinflash/plugins/sqlite/sqlite_db.cpp
LOCAL_SRC_FILES += bakeinflash/plugins/sqlite/sqlite_table.cpp
LOCAL_SRC_FILES += bakeinflash/plugins/file/file.cpp
LOCAL_SRC_FILES += base/container.cpp
LOCAL_SRC_FILES += base/image.cpp
LOCAL_SRC_FILES += base/membuf.cpp
LOCAL_SRC_FILES += base/tu_file.cpp
LOCAL_SRC_FILES += base/tu_timer.cpp
LOCAL_SRC_FILES += base/zlib_adapter.cpp
LOCAL_SRC_FILES += base/ear_clip_triangulate_float.cpp
LOCAL_SRC_FILES += base/image_filters.cpp
# LOCAL_SRC_FILES += base/tu_gc_singlethreaded_marksweep.cpp
LOCAL_SRC_FILES += base/tu_types.cpp
LOCAL_SRC_FILES += base/ear_clip_triangulate_sint16.cpp
LOCAL_SRC_FILES += base/jpeg.cpp
LOCAL_SRC_FILES += base/triangulate_float.cpp
LOCAL_SRC_FILES += base/tu_loadlib.cpp
LOCAL_SRC_FILES += base/utf8.cpp
LOCAL_SRC_FILES += base/file_util.cpp
LOCAL_SRC_FILES += base/logger.cpp
LOCAL_SRC_FILES += base/triangulate_sint32.cpp
LOCAL_SRC_FILES += base/tu_random.cpp 
LOCAL_SRC_FILES += base/utility.cpp
LOCAL_SRC_FILES += base/md5.cpp

LOCAL_SRC_FILES += jpeglib/cdjpeg.c
LOCAL_SRC_FILES += jpeglib/jcdctmgr.c
LOCAL_SRC_FILES += jpeglib/jcmaster.c
LOCAL_SRC_FILES += jpeglib/jcsample.c
LOCAL_SRC_FILES += jpeglib/jdatasrc.c
LOCAL_SRC_FILES += jpeglib/jdinput.c
LOCAL_SRC_FILES += jpeglib/jdphuff.c
LOCAL_SRC_FILES += jpeglib/jfdctflt.c
LOCAL_SRC_FILES += jpeglib/jidctint.c
LOCAL_SRC_FILES += jpeglib/jquant2.c
LOCAL_SRC_FILES += jpeglib/rdppm.c
LOCAL_SRC_FILES += jpeglib/wrbmp.c
LOCAL_SRC_FILES += jpeglib/jcapimin.c
LOCAL_SRC_FILES += jpeglib/jchuff.c
LOCAL_SRC_FILES += jpeglib/jcomapi.c
LOCAL_SRC_FILES += jpeglib/jctrans.c
LOCAL_SRC_FILES += jpeglib/jdcoefct.c
LOCAL_SRC_FILES += jpeglib/jdmainct.c
LOCAL_SRC_FILES += jpeglib/jdpostct.c
LOCAL_SRC_FILES += jpeglib/jfdctfst.c
LOCAL_SRC_FILES += jpeglib/jidctred.c
LOCAL_SRC_FILES += jpeglib/jutils.c
LOCAL_SRC_FILES += jpeglib/rdrle.c
LOCAL_SRC_FILES += jpeglib/wrgif.c
LOCAL_SRC_FILES += jpeglib/jcapistd.c
LOCAL_SRC_FILES += jpeglib/jcinit.c
LOCAL_SRC_FILES += jpeglib/jcparam.c
LOCAL_SRC_FILES += jpeglib/jdapimin.c
LOCAL_SRC_FILES += jpeglib/jdcolor.c
LOCAL_SRC_FILES += jpeglib/jdmarker.c
LOCAL_SRC_FILES += jpeglib/jdsample.c
LOCAL_SRC_FILES += jpeglib/jfdctint.c
LOCAL_SRC_FILES += jpeglib/jmemansi.c
LOCAL_SRC_FILES += jpeglib/rdbmp.c
LOCAL_SRC_FILES += jpeglib/rdswitch.c
LOCAL_SRC_FILES += jpeglib/wrppm.c
LOCAL_SRC_FILES += jpeglib/jccoefct.c
LOCAL_SRC_FILES += jpeglib/jcmainct.c \
LOCAL_SRC_FILES += jpeglib/jcphuff.c
LOCAL_SRC_FILES += jpeglib/jdapistd.c
LOCAL_SRC_FILES += jpeglib/jddctmgr.c
LOCAL_SRC_FILES += jpeglib/jdmaster.c
LOCAL_SRC_FILES += jpeglib/jdtrans.c
LOCAL_SRC_FILES += jpeglib/jidctflt.c
LOCAL_SRC_FILES += jpeglib/jmemmgr.c
LOCAL_SRC_FILES += jpeglib/rdcolmap.c
LOCAL_SRC_FILES += jpeglib/rdtarga.c
LOCAL_SRC_FILES += jpeglib/wrrle.c
LOCAL_SRC_FILES += jpeglib/jccolor.c
LOCAL_SRC_FILES += jpeglib/jcmarker.c
LOCAL_SRC_FILES += jpeglib/jcprepct.c
LOCAL_SRC_FILES += jpeglib/jdatadst.c
LOCAL_SRC_FILES += jpeglib/jdhuff.c
LOCAL_SRC_FILES += jpeglib/jdmerge.c
LOCAL_SRC_FILES += jpeglib/jerror.c
LOCAL_SRC_FILES += jpeglib/jidctfst.c
LOCAL_SRC_FILES += jpeglib/jquant1.c
LOCAL_SRC_FILES += jpeglib/rdgif.c
LOCAL_SRC_FILES += jpeglib/transupp.c
LOCAL_SRC_FILES += jpeglib/wrtarga.c

# LOCAL_SRC_FILES += net/http_client.cpp
# LOCAL_SRC_FILES += net/http_helper.cpp
# LOCAL_SRC_FILES += net/net_interface_tcp.cpp
# LOCAL_SRC_FILES += net/tu_net_file.cpp

LOCAL_SRC_FILES += pugixml/pugixml.cpp

LOCAL_SRC_FILES += sqlite3/sqlite3.c


LOCAL_SRC_FILES  += OpenAL/OpenAL32/alAuxEffectSlot.c \
                    OpenAL/OpenAL32/alBuffer.c        \
                    OpenAL/OpenAL32/alDatabuffer.c    \
                    OpenAL/OpenAL32/alEffect.c        \
                    OpenAL/OpenAL32/alError.c         \
                    OpenAL/OpenAL32/alExtension.c     \
                    OpenAL/OpenAL32/alFilter.c        \
                    OpenAL/OpenAL32/alListener.c      \
                    OpenAL/OpenAL32/alSource.c        \
                    OpenAL/OpenAL32/alState.c         \
                    OpenAL/OpenAL32/alThunk.c         \
                    OpenAL/Alc/ALc.c                  \
                    OpenAL/Alc/alcConfig.c            \
                    OpenAL/Alc/alcEcho.c              \
                    OpenAL/Alc/alcModulator.c         \
                    OpenAL/Alc/alcReverb.c            \
                    OpenAL/Alc/alcRing.c              \
                    OpenAL/Alc/alcThread.c            \
                    OpenAL/Alc/ALu.c                  \
                    OpenAL/Alc/android.c              \
                    OpenAL/Alc/bs2b.c                 \
                    OpenAL/Alc/null.c                 \

#LOCAL_CFLAGS     := -DAL_BUILD_LIBRARY -DAL_ALEXT_PROTOTYPES -O3 -g0
LOCAL_CFLAGS     := -DAL_BUILD_LIBRARY -DAL_ALEXT_PROTOTYPES -O3


LOCAL_STATIC_LIBRARIES := avformat avcodec swscale  avutil freetype ngtserver 
# LOCAL_STATIC_LIBRARIES := freetype 
# -lavdevice -lavformat -lavcodec -lswscale -lavutil -lz -lbz2 -lm 

LOCAL_LDLIBS    := -lz -llog -lGLESv1_CM

include $(BUILD_SHARED_LIBRARY)


