bin_PROGRAMS = shaaft

shaaft_SOURCES = \
	main.cpp \
	ActionCallbacks.cpp \
	BlockController.cpp \
	BlockModel.cpp \
	BlockView.cpp \
	Callback.cpp \
	CallbackManager.cpp \
	Game.cpp \
	GameState.cpp \
	Input.cpp \
	Keys.cpp \
	MenuManager.cpp \
	ScoreKeeper.cpp \
	Selectable.cpp \
	SelectableFactory.cpp \
	TextInput.cpp \
	VideoBase.cpp \
	XMLHelper.cpp

INCLUDES = -I../utils -I../utilssdl -I../utilsgl -I../tinyxml -I../license
LDADD = ../utils/libutils.a ../utilsgl/libutilsgl.a ../utilssdl/libutilssdl.a ../tinyxml/libtinyxml.a ../license/liblicense.a

if WIN32
LDADD += shaaft.res
else
endif

depend:
	makedepend -I/usr/include/g++-3 $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS) $(shaaft_SOURCES)

if WIN32
else
pkgdata_DATA = resource.dat

resource.dat:
	../tools/Packer ../data/description_file resource.dat

DISTCLEANFILES = resource.dat
endif

small:
	strip $(bin_PROGRAMS)
	upx -9 $(bin_PROGRAMS)

MAINTAINERCLEANFILES = Makefile.bak Makefile.in
