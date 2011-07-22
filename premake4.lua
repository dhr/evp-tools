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

  -- A simple test harness for running images and debugging

  newoption {
    trigger = "test-harness",
    description = "Produce build files to build the test-harness code"
  }

  if _OPTIONS["test-harness"] then
    project "test"
      kind "ConsoleApp"
      language "C++"
      targetname "test"

      files {"src/test/test.cpp"}

      includedirs {"deps/clip/include", "deps/evp/include"}
      links {"jpeg"}
  end

  -- Building MATLAB mex files

  newoption {
    trigger = "matlabroot",
    value = "path",
    description = "Path to MATLAB root directory ('matlabroot' in MATLAB)"
  }

  if _OPTIONS["matlabroot"] then
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

  if not _OPTIONS["help"] and
     not _OPTIONS["test-harness"] and
     not _OPTIONS["matlabroot"] then
    print "No build files were produced because no relevant options were given."
    print "Use '--matlabroot=<path>' to generate build files for the MATLAB bindings."
    print "Use '--test-harness' to generate build files for the test harness."
    os.exit()
  end

