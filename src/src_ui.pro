# drumkv1_ui.pro
#
NAME = drumkv1

TARGET = $${NAME}_ui
TEMPLATE = lib
CONFIG += shared
LIBS += -L.

include(src_ui.pri)

HEADERS = \
	config.h \
	drumkv1_ui.h \
	drumkv1widget.h \
	drumkv1widget_env.h \
	drumkv1widget_filt.h \
	drumkv1widget_sample.h \
	drumkv1widget_wave.h \
	drumkv1widget_param.h \
	drumkv1widget_preset.h \
	drumkv1widget_status.h \
	drumkv1widget_spinbox.h \
	drumkv1widget_elements.h \
	drumkv1widget_programs.h \
	drumkv1widget_controls.h \
	drumkv1widget_control.h \
	drumkv1widget_config.h

SOURCES = \
	drumkv1_ui.cpp \
	drumkv1widget.cpp \
	drumkv1widget_env.cpp \
	drumkv1widget_filt.cpp \
	drumkv1widget_sample.cpp \
	drumkv1widget_wave.cpp \
	drumkv1widget_param.cpp \
	drumkv1widget_preset.cpp \
	drumkv1widget_status.cpp \
	drumkv1widget_spinbox.cpp \
	drumkv1widget_elements.cpp \
	drumkv1widget_programs.cpp \
	drumkv1widget_controls.cpp \
	drumkv1widget_control.cpp \
	drumkv1widget_config.cpp

FORMS = \
	drumkv1widget.ui \
	drumkv1widget_control.ui \
	drumkv1widget_config.ui

RESOURCES += drumkv1.qrc


unix {

	OBJECTS_DIR = .obj_ui
	MOC_DIR     = .moc_ui
	UI_DIR      = .ui_ui

	isEmpty(PREFIX) {
		PREFIX = /usr/local
	}

	isEmpty(LIBDIR) {
		TARGET_ARCH = $$system(uname -m)
		contains(TARGET_ARCH, x86_64) {
			LIBDIR = $${PREFIX}/lib64
		} else {
			LIBDIR = $${PREFIX}/lib
		}
	}

	INSTALLS += target

	target.path = $${LIBDIR}

	LIBS += -l$${NAME} -Wl,-rpath,$${LIBDIR}
}

QT += widgets xml
