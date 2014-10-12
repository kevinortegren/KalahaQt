solution "JoKeBotKalaha"
	configurations { "Debug", "Release" }
	platforms { "x32" }
	
	location "build"
	targetdir "bin"
	debugdir "bin"
	
	configuration "Debug"
		flags { "Symbols" }
		targetsuffix "_d"
	configuration "Release"
		flags { "Optimize" }
	
	project "JoKeBotKalaha"
		kind "WindowedApp"
		language "C++"
		files { "src/**.h", "src/**.cpp", "src/**.ui" }
		links { "lib/Qt5Core", "lib/Qt5Gui", "lib/Qt5Network", "lib/Qt5Widgets" }
		includedirs { "include/**", "include/" }