# drumkv1_lv2.pro
#
NAME = drumkv1

TARGET = $${NAME}
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
	drumkv1_reverb.h \
	drumkv1_param.h \
	drumkv1_sched.h

SOURCES = \
	drumkv1.cpp \
	drumkv1_lv2.cpp \
	drumkv1_wave.cpp \
	drumkv1_param.cpp \
	drumkv1_sched.cpp


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
		isEmpty(LIBDIR) {
			LV2DIR = $${PREFIX}/lib/lv2
		} else {
			LV2DIR = $${LIBDIR}/lv2
		}
	}

	TARGET_LV2 = $${NAME}.lv2/$${TARGET}.so

	!exists($${TARGET_LV2}) {
		system(touch $${TARGET_LV2})
	}

	TARGET_LIB = $${NAME}.lv2/lib$${TARGET}.a

	!exists($${TARGET_LIB}) {
		system(touch $${TARGET_LIB})
	}

	QMAKE_POST_LINK += $${QMAKE_COPY} -vp $(TARGET) $${TARGET_LV2};\
		rm -vf $${TARGET_LIB}; ar -r $${TARGET_LIB} $${TARGET_LV2}

	INSTALLS += target

	target.path  = $${LV2DIR}/$${NAME}.lv2
	target.files = $${TARGET_LV2} \
		$${NAME}.lv2/$${NAME}.ttl \
		$${NAME}.lv2/manifest.ttl

	QMAKE_CLEAN += $${TARGET_LV2} $${TARGET_LIB}
}

QT -= gui
QT += xml
