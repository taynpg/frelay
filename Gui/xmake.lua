add_rules("mode.debug", "mode.release")
target("frelayGUI")
    add_rules("qt.widgetapp")
    add_defines("QAPPLICATION_CLASS=QApplication")
    add_includedirs(".", {public = true})
    add_files("Control/*.h")
    add_files("Form/*.h")
    add_files("GuiUtil/*.h")
    add_files("Control/*.cpp")
    add_files("Control/*.ui")
    add_files("Form/*.cpp")
    add_files("Form/*.ui")
    add_files("GuiUtil/*.cpp")
    add_files("*.cpp")
    add_files("*.ui")
    add_files("*.h")
    add_files("../Res/frelay.qrc")

    if is_plat("windows") then
        add_files("../Res/ico.rc")
        add_ldflags("-subsystem:windows")
    end
    add_frameworks("QtCore")
    add_frameworks("QtGui")
    add_frameworks("QtNetwork")
    add_deps("Struct")
    add_deps("Protocol")
    add_deps("Util")
    add_deps("ClientCore")
    add_deps("SingleApplication")
    if is_plat("mingw") then
        add_files("../Res/ico.rc")
        add_ldflags("-mwindows")
    else
        add_deps("crashelper")
    end

    if is_plat("linux") then
        add_links("dl")
    end