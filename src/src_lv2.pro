# drumkv1_lv2.pro
#
NAME = drumkv1

TARGET = $${NAME}_lv2
TEMPLATE = lib
CONFIG += shared plugin

include(src_lv2.pri)

HEADERS = \
	config.h \
	drumkv1.h \
	drumkv1_lv2.h \
	drumkv1_config.h \
	drumkv1_sample.h \
	drumkv1_wave.h \
	drumkv1_ramp.h \
	drumkv1_list.h \
	drumkv1_fx.h \
	drumkv1widget.h \
	drumkv1widget_env.h \
	drumkv1widget_filt.h \
	drumkv1widget_sample.h \
	drumkv1widget_wave.h \
	drumkv1widget_knob.h \
	drumkv1widget_preset.h \
	drumkv1widget_status.h \
	drumkv1widget_config.h \
	drumkv1widget_elements.h \
	drumkv1widget_lv2.h

SOURCES = \
	drumkv1.cpp \
	drumkv1_lv2.cpp \
	drumkv1widget.cpp \
	drumkv1widget_env.cpp \
	drumkv1widget_filt.cpp \
	drumkv1widget_sample.cpp \
	drumkv1widget_wave.cpp \
	drumkv1widget_knob.cpp \
	drumkv1widget_preset.cpp \
	drumkv1widget_status.cpp \
	drumkv1widget_config.cpp \
	drumkv1widget_elements.cpp \
	drumkv1widget_lv2.cpp

FORMS = \
	drumkv1widget.ui

RESOURCES += drumkv1.qrc


unix {

	OBJECTS_DIR = .obj_lv2
	MOC_DIR     = .moc_lv2
	UI_DIR      = .ui_lv2

	isEmpty(PREFIX) {
		PREFIX = /usr/local
	}

	contains(PREFIX, $$system(echo $HOME)) {
		LV2DIR = $${PREFIX}/.lv2
	} else {
		ARCH = $$system(uname -m)
		contains(ARCH, x86_64) {
			LV2DIR = $${PREFIX}/lib64/lv2
		} else {
			LV2DIR = $${PREFIX}/lib/lv2
		}
	}

	isEmpty(QMAKE_EXTENSION_SHLIB) {
		QMAKE_EXTENSION_SHLIB = so
	}

	TARGET_LV2 = $${NAME}.lv2/$${NAME}.$${QMAKE_EXTENSION_SHLIB}

	!exists($${TARGET_LV2}) {
		system(touch $${TARGET_LV2})
	}

	QMAKE_POST_LINK += $${QMAKE_COPY} -vp $(TARGET) $${TARGET_LV2}

	INSTALLS += target

	target.path  = $${LV2DIR}/$${NAME}.lv2
	target.files = $${TARGET_LV2} \
		$${NAME}.lv2/$${NAME}.ttl \
		$${NAME}.lv2/manifest.ttl

	QMAKE_CLEAN += $${TARGET_LV2}
}

QT += xml

# QT5 support
!lessThan(QT_MAJOR_VERSION, 5) {
	QT += widgets
}
