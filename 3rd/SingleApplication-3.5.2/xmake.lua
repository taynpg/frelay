add_rules("mode.debug", "mode.release")

target("SingleApplication")
    add_rules("qt.static")
    if has_config("single5") then
        add_defines("QT_DEFAULT_MAJOR_VERSION=5")
    else
        add_defines("QT_DEFAULT_MAJOR_VERSION=6")
    end
    -- QApplication/QGuiApplication/QCoreApplication
    add_defines("QAPPLICATION_CLASS=QApplication")
    add_defines("QT_NO_CAST_TO_ASCII")
    add_defines("QT_NO_CAST_FROM_ASCII")
    add_defines("QT_NO_URL_CAST_FROM_STRING")
    add_defines("QT_NO_CAST_FROM_BYTEARRAY")
    add_defines("QT_USE_QSTRINGBUILDER")
    add_defines("QT_NO_NARROWING_CONVERSIONS_IN_CONNECT")
    add_defines("QT_NO_KEYWORDS")
    add_defines("QT_NO_FOREACH")
    add_includedirs(".", {public = true})
    add_files("*.h")
    add_files("*.cpp")
    add_frameworks("QtCore")
    add_frameworks("QtGui")
    add_frameworks("QtWidgets")
    add_frameworks("QtNetwork")

option("single5")
    set_default(false)
    set_showmenu(true)
    set_description("SingleApplication Build with Qt5.")