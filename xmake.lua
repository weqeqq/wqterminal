set_project("weqeqq.terminal")
set_version("0.1.0")
set_languages("c++23")
set_warnings("all")
add_rules("mode.debug", "mode.release", "plugin.compile_commands.autoupdate")

set_policy("build.c++.modules", true)
set_policy("build.c++.modules.std", false)

add_repositories("weqeqq.repo https://github.com/weqeqq/xmake-repo.git")
add_requires("weqeqq.test 0.2.0")

-- stylua: ignore start

target("weqeqq.terminal")
    set_kind("static")
    add_files("sources/**.cppm", {public = true})
    add_includedirs("sources", {public = true})
    set_policy("build.c++.modules", true)

target("tests")
    set_kind("binary")
    set_default(false)
    add_deps("weqeqq.terminal")
    add_files("tests/**.cpp")
    set_policy("build.c++.modules", true)
    add_packages("weqeqq.test", { components = {"core", "main"} })
    add_tests("default")
