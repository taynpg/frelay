add_rules("mode.debug", "mode.release")

target("frelayConsole")
    set_rules("qt.console")
    add_files("Console.cpp", "Console.h", "main.cpp")
    add_deps("ClientCore")
    add_deps("Protocol")
    add_deps("Util")
    add_frameworks("QtNetwork")

    if is_plat("windows") then
        add_files("../Res/ico.rc")
    end
    
    if is_plat("mingw") then
        add_files("../Res/ico.rc")
    else
        add_deps("crashelper")
    end

    if is_plat("linux") then
        add_links("dl")
    end