add_rules("mode.debug", "mode.release")

target("frelayConsole")
    set_rules("qt.console")
    add_files("../Res/ico.rc")
    add_files("Console.cpp", "Console.h", "main.cpp")
    add_deps("ClientCore")
    add_deps("Protocol")
    add_deps("Util")
    add_frameworks("QtNetwork")
    
    if is_plat("mingw") then
    else
        add_deps("crashelper")
    end