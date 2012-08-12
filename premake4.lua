solution "EVP"
  -- Options

  newoption {
    trigger = "arch",
    description = "Architecture to target (32- or 64-bit)",
    allowed = {
      {"32", "32-bit"},
      {"64", "64-bit"}
    }
  }

  local is64bit = true

  if _OPTIONS["arch"] == "64" then
    is64bit = true
  elseif _OPTIONS["arch"] == "32" then
    is64bit = false
  end

  platforms {iif(is64bit, "x64", "x32")}

  newoption {
    trigger = "no-jpeg",
    description = "Don't enable jpeg I/O"
  }

  newoption {
    trigger = "no-png",
    description = "Don't enable png I/O"
  }
  
  newoption {
    trigger = "no-matio",
    description = "Don't enable MAT file I/O"
  }
  
  -- Configurations
  
  flags {"ExtraWarnings", "FatalWarnings"}
    
  configurations {"Debug", "Release"}

  configuration "Debug"
    targetdir "bin"
    flags {"Symbols"}
  
  configuration "Release"
    targetdir "bin"
    flags {"OptimizeSpeed"}
  
  -- Libraries/Defines
  
  configuration "not macosx"
    links {"OpenCL"}
  
  configuration {"macosx", "gmake"}
    linkoptions {"-framework OpenCL"}
  
  configuration "xcode*"
    links {"OpenCL.framework"}

  defines {"NO_GL_INTEROP"}

  if not _OPTIONS["no-jpeg"] then
    links {"jpeg"}
  else
    defines {"EVP_NO_JPEG"}
  end

  if not _OPTIONS["no-png"] then
    links {"png"}

    configuration "macosx"
      includedirs {"/usr/X11/include"}
      libdirs {"/usr/X11/lib"}
  else
    defines {"EVP_NO_PNG"}
  end
    
  if not _OPTIONS["no-matio"] then
    links {"matio"}
  else
    defines {"EVP_NO_MATIO"}
  end
  
  -- Prebuild

  location "build"
  
  dofile("embed.lua")
  newaction {
    trigger = "embed",
    description = "Stringify OpenCL code",
    execute = doembed
  }
  
  prebuildcommands {"cd ..; premake4 embed"}
  
  -- Build
  
  files {"**.hpp", "**.cl", "premake4.lua"}
  
  project "evp"
    kind "ConsoleApp"
    language "C++"

    targetname "evp"

    includedirs {"deps/clip/include", "deps/evp/include"}

    files {"src/evp/evp.cpp"}