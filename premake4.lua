solution "EVP"
  -- General settings

  location "build"

  flags {"ExtraWarnings", "FatalWarnings"}
    
  configurations {"Debug", "Release"}

  configuration "Debug"
    targetdir "bin"
    flags {"Symbols"}
  
  configuration "Release"
    targetdir "bin"
    flags {"OptimizeSpeed"}
  
  configuration "not macosx"
    links {"OpenCL"}
  
  configuration {"macosx", "gmake"}
    linkoptions {"-framework OpenCL"}
  
  configuration "xcode*"
    links {"OpenCL.framework"}

  defines {"NO_GL_INTEROP"}
    
  files {"**.hpp", "**.cl", "premake4.lua"}

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

  if _OPTIONS["no-jpeg"] then
    defines {"EVP_NO_JPEG"}
  end

  if _OPTIONS["no-png"] then
    defines {"EVP_NO_PNG"}
  end

  newoption {
    trigger = "no-jpeg",
    description = "Don't enable jpeg I/O (only affects EVP command line tool)"
  }

  newoption {
    trigger = "no-png",
    description = "Don't enable png I/O (only affects EVP command line tool)"
  }

  -- The EVP command line tool

  newoption {
    trigger = "evp",
    description = "Generate build files for the EVP command line tool"
  }

  if _OPTIONS["evp"] or _OPTIONS["all"] then
    project "evp"
      kind "ConsoleApp"
      language "C++"

      targetname "evp"

      includedirs {"deps/clip/include", "deps/evp/include"}

      files {"src/evp/evp.cpp"}

      if not _OPTIONS["no-jpeg"] then
        links {"jpeg"}
      end

      if not _OPTIONS["no-png"] then
        links {"png"}

        configuration "macosx"
          includedirs {"/usr/X11/include"}
          libdirs {"/usr/X11/lib"}
      end
  end

  -- Building MATLAB mex files

  newoption {
    trigger = "matlabroot",
    value = "path",
    description = "Path to MATLAB root directory ('matlabroot' in MATLAB)"
  }

  if _OPTIONS["matlabroot"] or _OPTIONS["all"] then
    project "evpmex"
      kind "SharedLib"
      language "C++"
      
      local archname = "???"

      if os.get() == "macosx" then
        archname = iif(is64bit, "maci64", "maci")
      elseif os.get() == "linux" then
        archname = iif(is64bit, "a64", "glx")
      elseif os.get() == "windows" then
        archname = iif(is64bit, "w64", "w32")
      end

      targetprefix ""
      targetname "evp"
      targetdir "bin/matlab"
      targetextension (".mex" .. archname)

      files {"src/matlab/evp.cpp"}

      includedirs {"deps/clip/include", "deps/evp/include"}
      includedirs {_OPTIONS["matlabroot"] .. "/extern/include"}
      libdirs {_OPTIONS["matlabroot"] .. "/bin/" .. archname}

      links {"mx", "mex", "mat"}

      configuration "macosx"
        buildoptions {
          "-fno-common",
          "-no-cpp-precomp",
          "-DMATLAB_MEX_FILE",
          "-DMX_COMPAT_32",
          "-DNDEBUG",
          "-arch " .. iif(is64bit, "x86_64", "i386")
        }

        linkoptions {
          "-undefined error",
          "-Wl,-twolevel_namespace",
          "-Wl,-exported_symbols_list," ..
            _OPTIONS["matlabroot"] .. "/extern/lib/" .. archname ..
            "/mexFunction.map",
          "-arch " .. iif(is64bit, "x86_64", "i386")
        }
  end

  -- An ugly test-harness for tweaking and debugging

  newoption {
    trigger = "test",
    description = "Generate build files to build the test-harness code"
  }

  if _OPTIONS["test"] or _OPTIONS["all"] then
    project "test"
      kind "ConsoleApp"
      language "C++"
      targetname "test"

      files {"src/test/test.cpp"}

      includedirs {"deps/clip/include", "deps/evp/include"}

      if not _OPTIONS["no-jpeg"] then
        links {"jpeg"}
      end

      if not _OPTIONS["no-png"] then
        links {"png"}

        configuration "macosx"
          includedirs {"/usr/X11/include"}
          libdirs {"/usr/X11/lib"}
      end
  end

  if not _OPTIONS["help"] and
     -- not _OPTIONS["all"] and
     not _OPTIONS["test-harness"] and
     not _OPTIONS["matlabroot"] and
     not _OPTIONS["evp"] then
    print "No build files were produced because no relevant options were given."
    print "Use '--matlabroot=<path>' to generate build files for the MATLAB bindings."
    print "Use '--test' to generate build files for the test harness."
    print "Use '--evp' to generate build files for the EVP command line tool."
    -- print "Use '--all' to generate build files for all of the above."
    os.exit()
  end
