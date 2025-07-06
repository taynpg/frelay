add_rules("mode.debug", "mode.release")

target("frelayServer")
    set_rules("qt.console")
    add_files("Server.cpp", "Server.h", "main.cpp")
    add_deps("ClientCore")
    add_deps("Protocol")
    add_deps("Util")
    add_frameworks("QtNetwork")
    
    if is_plat("windows") then
        add_files("../Res/server.rc")
    end
    
    if is_plat("mingw") then
        add_files("../Res/server.rc")
    else
        add_deps("crashelper")
    end

    if is_plat("linux") then
        add_links("dl")
    end