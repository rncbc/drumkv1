# drumkv1_jack.pro
#
NAME = drumkv1

TARGET = $${NAME}_jack
TEMPLATE = app

include(src_jack.pri)

HEADERS = \
	config.h \
	drumkv1.h \
	drumkv1_ui.h \
	drumkv1_jack.h \
	drumkv1_config.h \
	drumkv1_sample.h \
	drumkv1_wave.h \
	drumkv1_ramp.h \
	drumkv1_list.h \
	drumkv1_fx.h \
	drumkv1_nsm.h \
	drumkv1_reverb.h \
	drumkv1_param.h \
	drumkv1_sched.h \
	drumkv1_programs.h \
	drumkv1_controls.h \
	drumkv1widget.h \
	drumkv1widget_env.h \
	drumkv1widget_filt.h \
	drumkv1widget_sample.h \
	drumkv1widget_wave.h \
	drumkv1widget_knob.h \
	drumkv1widget_preset.h \
	drumkv1widget_status.h \
	drumkv1widget_elements.h \
	drumkv1widget_programs.h \
	drumkv1widget_controls.h \
	drumkv1widget_config.h \
	drumkv1widget_jack.h

SOURCES = \
	drumkv1.cpp \
	drumkv1_ui.cpp \
	drumkv1_jack.cpp \
	drumkv1_config.cpp \
	drumkv1_sample.cpp \
	drumkv1_wave.cpp \
	drumkv1_nsm.cpp \
	drumkv1_param.cpp \
	drumkv1_sched.cpp \
	drumkv1_programs.cpp \
	drumkv1_controls.cpp \
	drumkv1widget.cpp \
	drumkv1widget_env.cpp \
	drumkv1widget_filt.cpp \
	drumkv1widget_sample.cpp \
	drumkv1widget_wave.cpp \
	drumkv1widget_knob.cpp \
	drumkv1widget_preset.cpp \
	drumkv1widget_status.cpp \
	drumkv1widget_elements.cpp \
	drumkv1widget_programs.cpp \
	drumkv1widget_controls.cpp \
	drumkv1widget_config.cpp \
	drumkv1widget_jack.cpp

FORMS = \
	drumkv1widget.ui \
	drumkv1widget_config.ui

RESOURCES += drumkv1.qrc


unix {

	OBJECTS_DIR = .obj_jack
	MOC_DIR     = .moc_jack
	UI_DIR      = .ui_jack

	isEmpty(PREFIX) {
		PREFIX = /usr/local
	}

	BINDIR = $${PREFIX}/bin
	DATADIR = $${PREFIX}/share

	DEFINES += DATADIR=\"$${DATADIR}\"

	INSTALLS += target desktop icon appdata \
		icon_scalable mimeinfo mimetypes mimetypes_scalable

	target.path = $${BINDIR}

	desktop.path = $${DATADIR}/applications
	desktop.files += $${NAME}.desktop

	icon.path = $${DATADIR}/icons/hicolor/32x32/apps
	icon.files += images/$${NAME}.png 

	icon_scalable.path = $${DATADIR}/icons/hicolor/scalable/apps
	icon_scalable.files += images/$${NAME}.svg

	appdata.path = $${DATADIR}/appdata
	appdata.files += appdata/$${NAME}.appdata.xml

	mimeinfo.path = $${DATADIR}/mime/packages
	mimeinfo.files += mimetypes/$${NAME}.xml

	mimetypes.path = $${DATADIR}/icons/hicolor/32x32/mimetypes
	mimetypes.files += mimetypes/application-x-$${NAME}-preset.png

	mimetypes_scalable.path = $${DATADIR}/icons/hicolor/scalable/mimetypes
	mimetypes_scalable.files += mimetypes/application-x-$${NAME}-preset.svg
}

QT += xml


# QT5 support
!lessThan(QT_MAJOR_VERSION, 5) {
	QT += widgets
}
