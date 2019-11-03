# Install script for directory: /Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/build-taglib-Desktop_Qt_5_12_5_clang_64bit-Release/libtag.a")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libtag.a" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libtag.a")
    execute_process(COMMAND "/usr/bin/ranlib" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libtag.a")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/taglib/tag.h;/taglib/fileref.h;/taglib/audioproperties.h;/taglib/taglib_export.h;/taglib/taglib.h;/taglib/tstring.h;/taglib/tlist.h;/taglib/tlist.tcc;/taglib/tstringlist.h;/taglib/tstringhandler.h;/taglib/tbytevector.h;/taglib/tbytevectorlist.h;/taglib/tbytevectorstream.h;/taglib/tiostream.h;/taglib/tfile.h;/taglib/tfilestream.h;/taglib/tmap.h;/taglib/tmap.tcc;/taglib/tpicture.h;/taglib/tpicturemap.h;/taglib/tpropertymap.h;/taglib/trefcounter.h;/taglib/tdebuglistener.h;/taglib/mpegfile.h;/taglib/mpegproperties.h;/taglib/mpegheader.h;/taglib/xingheader.h;/taglib/id3v1tag.h;/taglib/id3v1genres.h;/taglib/id3v2extendedheader.h;/taglib/id3v2frame.h;/taglib/id3v2header.h;/taglib/id3v2synchdata.h;/taglib/id3v2footer.h;/taglib/id3v2framefactory.h;/taglib/id3v2tag.h;/taglib/attachedpictureframe.h;/taglib/commentsframe.h;/taglib/eventtimingcodesframe.h;/taglib/generalencapsulatedobjectframe.h;/taglib/ownershipframe.h;/taglib/popularimeterframe.h;/taglib/privateframe.h;/taglib/relativevolumeframe.h;/taglib/synchronizedlyricsframe.h;/taglib/textidentificationframe.h;/taglib/uniquefileidentifierframe.h;/taglib/unknownframe.h;/taglib/unsynchronizedlyricsframe.h;/taglib/urllinkframe.h;/taglib/chapterframe.h;/taglib/tableofcontentsframe.h;/taglib/podcastframe.h;/taglib/oggfile.h;/taglib/oggpage.h;/taglib/oggpageheader.h;/taglib/xiphcomment.h;/taglib/vorbisfile.h;/taglib/vorbisproperties.h;/taglib/oggflacfile.h;/taglib/speexfile.h;/taglib/speexproperties.h;/taglib/opusfile.h;/taglib/opusproperties.h;/taglib/flacfile.h;/taglib/flacpicture.h;/taglib/flacproperties.h;/taglib/flacmetadatablock.h;/taglib/apefile.h;/taglib/apeproperties.h;/taglib/apetag.h;/taglib/apefooter.h;/taglib/apeitem.h;/taglib/mpcfile.h;/taglib/mpcproperties.h;/taglib/wavpackfile.h;/taglib/wavpackproperties.h;/taglib/trueaudiofile.h;/taglib/trueaudioproperties.h;/taglib/rifffile.h;/taglib/aifffile.h;/taglib/aiffproperties.h;/taglib/wavfile.h;/taglib/wavproperties.h;/taglib/infotag.h;/taglib/asffile.h;/taglib/asfproperties.h;/taglib/asftag.h;/taglib/asfattribute.h;/taglib/asfpicture.h;/taglib/mp4file.h;/taglib/mp4atom.h;/taglib/mp4tag.h;/taglib/mp4item.h;/taglib/mp4properties.h;/taglib/mp4coverart.h;/taglib/modfilebase.h;/taglib/modfile.h;/taglib/modtag.h;/taglib/modproperties.h;/taglib/itfile.h;/taglib/itproperties.h;/taglib/s3mfile.h;/taglib/s3mproperties.h;/taglib/xmfile.h;/taglib/xmproperties.h;/taglib/ebmlfile.h;/taglib/ebmlelement.h;/taglib/ebmlconstants.h;/taglib/ebmlmatroskafile.h;/taglib/ebmlmatroskaconstants.h;/taglib/ebmlmatroskaaudio.h;/taglib/dsffile.h;/taglib/dsfproperties.h;/taglib/dsdifffile.h;/taglib/dsdiffproperties.h;/taglib/dsdiffdiintag.h")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/taglib" TYPE FILE FILES
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/tag.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/fileref.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/audioproperties.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/taglib_export.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/toolkit/taglib.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/toolkit/tstring.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/toolkit/tlist.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/toolkit/tlist.tcc"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/toolkit/tstringlist.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/toolkit/tstringhandler.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/toolkit/tbytevector.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/toolkit/tbytevectorlist.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/toolkit/tbytevectorstream.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/toolkit/tiostream.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/toolkit/tfile.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/toolkit/tfilestream.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/toolkit/tmap.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/toolkit/tmap.tcc"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/toolkit/tpicture.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/toolkit/tpicturemap.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/toolkit/tpropertymap.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/toolkit/trefcounter.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/toolkit/tdebuglistener.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/mpeg/mpegfile.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/mpeg/mpegproperties.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/mpeg/mpegheader.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/mpeg/xingheader.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/mpeg/id3v1/id3v1tag.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/mpeg/id3v1/id3v1genres.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/mpeg/id3v2/id3v2extendedheader.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/mpeg/id3v2/id3v2frame.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/mpeg/id3v2/id3v2header.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/mpeg/id3v2/id3v2synchdata.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/mpeg/id3v2/id3v2footer.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/mpeg/id3v2/id3v2framefactory.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/mpeg/id3v2/id3v2tag.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/mpeg/id3v2/frames/attachedpictureframe.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/mpeg/id3v2/frames/commentsframe.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/mpeg/id3v2/frames/eventtimingcodesframe.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/mpeg/id3v2/frames/generalencapsulatedobjectframe.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/mpeg/id3v2/frames/ownershipframe.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/mpeg/id3v2/frames/popularimeterframe.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/mpeg/id3v2/frames/privateframe.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/mpeg/id3v2/frames/relativevolumeframe.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/mpeg/id3v2/frames/synchronizedlyricsframe.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/mpeg/id3v2/frames/textidentificationframe.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/mpeg/id3v2/frames/uniquefileidentifierframe.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/mpeg/id3v2/frames/unknownframe.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/mpeg/id3v2/frames/unsynchronizedlyricsframe.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/mpeg/id3v2/frames/urllinkframe.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/mpeg/id3v2/frames/chapterframe.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/mpeg/id3v2/frames/tableofcontentsframe.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/mpeg/id3v2/frames/podcastframe.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/ogg/oggfile.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/ogg/oggpage.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/ogg/oggpageheader.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/ogg/xiphcomment.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/ogg/vorbis/vorbisfile.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/ogg/vorbis/vorbisproperties.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/ogg/flac/oggflacfile.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/ogg/speex/speexfile.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/ogg/speex/speexproperties.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/ogg/opus/opusfile.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/ogg/opus/opusproperties.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/flac/flacfile.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/flac/flacpicture.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/flac/flacproperties.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/flac/flacmetadatablock.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/ape/apefile.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/ape/apeproperties.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/ape/apetag.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/ape/apefooter.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/ape/apeitem.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/mpc/mpcfile.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/mpc/mpcproperties.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/wavpack/wavpackfile.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/wavpack/wavpackproperties.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/trueaudio/trueaudiofile.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/trueaudio/trueaudioproperties.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/riff/rifffile.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/riff/aiff/aifffile.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/riff/aiff/aiffproperties.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/riff/wav/wavfile.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/riff/wav/wavproperties.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/riff/wav/infotag.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/asf/asffile.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/asf/asfproperties.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/asf/asftag.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/asf/asfattribute.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/asf/asfpicture.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/mp4/mp4file.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/mp4/mp4atom.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/mp4/mp4tag.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/mp4/mp4item.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/mp4/mp4properties.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/mp4/mp4coverart.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/mod/modfilebase.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/mod/modfile.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/mod/modtag.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/mod/modproperties.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/it/itfile.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/it/itproperties.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/s3m/s3mfile.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/s3m/s3mproperties.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/xm/xmfile.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/xm/xmproperties.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/ebml/ebmlfile.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/ebml/ebmlelement.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/ebml/ebmlconstants.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/ebml/matroska/ebmlmatroskafile.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/ebml/matroska/ebmlmatroskaconstants.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/ebml/matroska/ebmlmatroskaaudio.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/dsf/dsffile.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/dsf/dsfproperties.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/dsdiff/dsdifffile.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/dsdiff/dsdiffproperties.h"
    "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/taglib/dsdiff/dsdiffdiintag.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/Users/bill/Documents/xamp2/src/thirdparty/taglib2/build-taglib-Desktop_Qt_5_12_5_clang_64bit-Release/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
