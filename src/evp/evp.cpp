#include <sstream>
#include <algorithm>

#include <evp/io/imageio.hpp> // Include this first for debugging
#include <evp.hpp>
#include <evp/io.hpp>
#include <evp/util/tictoc.hpp>

using namespace std;
using namespace std::tr1;
using namespace evp;

void die(const string& msg) {
  cerr << "Error: " << msg << "." << endl;
  exit(1);
}

template<typename T>
void getArgument(int& argc, char**& argv, T* arg) {
  --argc; ++argv;
  if (!argc)
    die("Argument not supplied to final option");
  stringstream ss(*argv);
  ss >> *arg;
}

string listDevicesOpts[] = {"list-devices"};
string listDevicesDesc = "List available OpenCL devices.";
void listDevicesHandler(int&, char**&);

bool runCurveInit = false;
string curveInitOpts[] = {"curve-init"};
string curveInitDesc = "Run initial curve operators.";
void curveInitHandler(int&, char**&) {
  runCurveInit = true;
}

bool runCurveRelax = false;
string curveRelaxOpts[] = {"curve-relax"};
string curveRelaxDesc = "Run curve relaxation (implies curve-init).";
void curveRelaxHandler(int&, char**&) {
  runCurveInit = true;
  runCurveRelax = true;
}

bool runFlowInit = false;
string flowInitOpts[] = {"flow-init"};
string flowInitDesc = "Run initial flow operators.";
void flowInitHandler(int&, char**&) {
  runFlowInit = true;
}

bool runFlowRelax = false;
string flowRelaxOpts[] = {"flow-relax"};
string flowRelaxDesc = "Run flow relaxation operators (implies flow-init).";
void flowRelaxHandler(int&, char**&) {
  runFlowInit = true;
  runFlowRelax = true;
}

string helpOpts[] = {"-h", "--help"};
string helpDesc = "Show this help text.";
void helpHandler(int&, char**&);

i32 platformNum = 0;
string platformOpts[] = {"-p", "--platform"};
string platformArgs[] = {"id"};
string platformDesc = "Select platform <id>. Default: 0.";
void platformHandler(int& argc, char**& argv) {
  getArgument(argc, argv, &platformNum);
  platformNum--;
}

i32 deviceNum = -1;
string deviceOpts[] = {"-d", "--device"};
string deviceArgs[] = {"id"};
string deviceDesc = "Select device <id>. Default: Default platform device.";
void deviceHandler(int& argc, char**& argv) {
  getArgument(argc, argv, &deviceNum);
  deviceNum--;
}

ValueType valueType = Float32;
string valueTypeOpts[] = {"-b", "--bit-depth"};
string valueTypeArgs[] = {"n"};
string valueTypeDesc = "Bits to use for image storage (16 or 32). Default: 32.";
void valueTypeHandler(int& argc, char**& argv) {
  i32 bits;
  getArgument(argc, argv, &bits);
  
  switch (bits) {
    case 16:
      valueType = Float16;
      break;
    
    case 32:
      valueType = Float32;
      break;
      
    default:
      die("Invalid bit depth (should be 16 or 32)");
  }
}

i32 enqueuesPerFinish = 1000;
string epfOpts[] = {"--max-enqueues"};
string epfArgs[] = {"n"};
string epfDesc = "Let device catch up after <n> enqueues. Default: 1000.";
void epfHandler(int& argc, char**& argv) {
  getArgument(argc, argv, &enqueuesPerFinish);
  if (enqueuesPerFinish <= 0)
    die("Invalid number of enqueues per finish (must be > 0)");
}

i32 numOrientations = 8;
string numOrientationsOpts[] = {"-t", "--orientations"};
string numOrientationsArgs[] = {"n"};
string numOrientationsDesc = "Use <n> orientations. Default: 8.";
void numOrientationsHandler(int& argc, char**& argv) {
  getArgument(argc, argv, &numOrientations);
  if (numOrientations <= 0 || numOrientations%2 != 0)
    die("Invalid number of orientations (must be > 0 and even)");
}

i32 numCurvatures = 3;
string numCurvaturesOpts[] = {"-k", "--curvatures"};
string numCurvaturesArgs[] = {"n"};
string numCurvaturesDesc = "Use <n> curvatures. Default: 3.";
void numCurvaturesHandler(int& argc, char**& argv) {
  getArgument(argc, argv, &numCurvatures);
  if (numCurvatures <= 0 || numCurvatures > 5 || numCurvatures%2 == 0)
    die("Invalid number of curvatures (must > 0, <= 5, and odd)");
}

f32 curveScale = 1.f;
string curveScaleOpts[] = {"--curve-scale"};
string curveScaleArgs[] = {"s"};
string curveScaleDesc = "Scale initial curve operators by <s>. Default: 1.";
void curveScaleHandler(int& argc, char**& argv) {
  getArgument(argc, argv, &curveScale);
  if (curveScale < 1)
    die("Invalid curve scaling (must >= 1)");
}

f32 rlxThresh = 0.f;
string rlxThreshOpts[] = {"--rlx-thresh"};
string rlxThreshArgs[] = {"t"};
string rlxThreshDesc = "Threshold relaxing curves with <t>. Default: 0.";
void rlxThreshHandler(int& argc, char**& argv) {
  getArgument(argc, argv, &rlxThresh);
  if (rlxThresh < 0 || rlxThresh >= 1)
    die("Invalid relaxation threshold (must be >= 0 and < 1)");
}

i32 curveIters = 5;
string curveItersOpts[] = {"--curve-iters"};
string curveItersArgs[] = {"n"};
string curveItersDesc = "Use <n> iterations for curve relaxation. Default: 5.";
void curveItersHandler(int& argc, char**& argv) {
  getArgument(argc, argv, &curveIters);
  if (curveIters <= 0)
    die("Invalid number of iterations (must be > 0)");
}

f32 curveDelta = 1.f;
string curveDeltaOpts[] = {"--curve-delta"};
string curveDeltaArgs[] = {"d"};
string curveDeltaDesc = "Use <d> for curve relaxation delta. Default: 1.";
void curveDeltaHandler(int& argc, char**& argv) {
  getArgument(argc, argv, &curveDelta);
}

i32 flowIters = 10;
string flowItersOpts[] = {"--flow-iters"};
string flowItersArgs[] = {"n"};
string flowItersDesc = "Use <n> iterations for flow relaxation. Default: 10.";
void flowItersHandler(int& argc, char**& argv) {
  getArgument(argc, argv, &flowIters);
  if (flowIters <= 0)
    die("Invalid number of iterations (must be > 0)");
}

f32 flowDelta = 1.f;
string flowDeltaOpts[] = {"--flow-delta"};
string flowDeltaArgs[] = {"d"};
string flowDeltaDesc = "Use <d> for flow relaxation delta. Default: 1.";
void flowDeltaHandler(int& argc, char**& argv) {
  getArgument(argc, argv, &curveDelta);
}

bool outputMatlab = true;
string noMatlabOpts[] = {"--no-mat"};
string noMatlabDesc = "Don't output in MATLAB format.";
void noMatlabHandler(int& argc, char**& argv) {
  outputMatlab = false;
}

bool outputPdf = false;
string pdfOpts[] = {"--pdf"};
string pdfDesc = "Output PDF files.";
void pdfHandler(int& argc, char**& argv) {
  outputPdf = true;
}

string outputDir = ".";
string outputDirOpts[] = {"-o", "--output-dir"};
string outputDirArgs[] = {"dir"};
string outputDirDesc = "Write output to <dir>. Default: Current directory.";
void outputDirHandler(int& argc, char**& argv) {
  getArgument(argc, argv, &outputDir);
}

typedef struct OptionEntry {
  string* opts; int nopts;
  string* args; int nargs;
  void (*handleOption)(int& argc, char**& argv);
  string description;
} OptionEntry;

#define OPTION_FLAG_ENTRY(name) \
  {name##Opts, sizeof(name##Opts)/sizeof(string), NULL, 0, \
   &name##Handler, name##Desc}

#define OPTION_ARGS_ENTRY(name) \
  {name##Opts, sizeof(name##Opts)/sizeof(string), \
   name##Args, sizeof(name##Args)/sizeof(string), \
   &name##Handler, name##Desc}

OptionEntry options[] = {
  OPTION_FLAG_ENTRY(listDevices),
  OPTION_FLAG_ENTRY(curveInit),
  OPTION_FLAG_ENTRY(curveRelax),
  OPTION_FLAG_ENTRY(flowInit),
  OPTION_FLAG_ENTRY(flowRelax),
  OPTION_FLAG_ENTRY(help),
  OPTION_ARGS_ENTRY(platform),
  OPTION_ARGS_ENTRY(device),
  OPTION_ARGS_ENTRY(valueType),
  OPTION_ARGS_ENTRY(epf),
  OPTION_ARGS_ENTRY(numOrientations),
  OPTION_ARGS_ENTRY(numCurvatures),
  OPTION_ARGS_ENTRY(curveScale),
  OPTION_ARGS_ENTRY(rlxThresh),
  OPTION_ARGS_ENTRY(curveIters),
  OPTION_ARGS_ENTRY(curveDelta),
  OPTION_ARGS_ENTRY(flowIters),
  OPTION_ARGS_ENTRY(flowDelta),
  OPTION_FLAG_ENTRY(noMatlab),
  OPTION_FLAG_ENTRY(pdf),
  OPTION_ARGS_ENTRY(outputDir)
};
i32 numOptions = sizeof(options)/sizeof(OptionEntry);

void helpHandler(int&, char**&) {
  cout << "Usage: evp command[s] [options] images\n";
  
  i32 maxOptionLength = 0;
  for (i32 i = 0; i < numOptions; ++i) {
    i32 length = -1;
    for (i32 j = 0; j < options[i].nopts; ++j)
      length += options[i].opts[j].size() + 1;
    for (i32 j = 0; j < options[i].nargs; ++j)
      length += options[i].args[j].size() + 3;
    maxOptionLength = max(maxOptionLength, length);
  }
  
  for (i32 i = 0; i < numOptions; ++i) {
    stringstream ss;
    ss << options[i].opts[0];
    for (i32 j = 1; j < options[i].nopts; ++j)
      ss << "/" << options[i].opts[j];
    for (i32 j = 0; j < options[i].nargs; ++j)
      ss << " <" << options[i].args[j] << ">";
    i32 size = i32(ss.tellp());
    cout << "  " << ss.str();
    for (i32 j = 0; j < maxOptionLength + 2 - size; ++j)
      cout << ' ';
    cout << options[i].description << "\n";
  }
  
  cout.flush();
  
  exit(0);
}

void listDevicesHandler(int&, char**&) {
  vector<cl::Platform> platforms;
  cl::Platform::get(&platforms);
  
  vector<cl::Device> devices;
  for (i32 i = 0; i < i32(platforms.size()); ++i) {
    cl::Platform p = platforms[i];
    stringstream ss;
    ss << "Platform #" << i + 1 << ": "
       << p.getInfo<CL_PLATFORM_NAME>();
    cout << ss.str() << endl;

    i32 lineLen = ss.str().length();
    for (i32 n = 0; n < lineLen; ++n)
      cout.put('-');
    cout << endl;
    
    p.getDevices(CL_DEVICE_TYPE_ALL, &devices);
    for (i32 j = 0; j < i32(devices.size()); ++j) {
      cl::Device d = devices[j];
      cout << "  Device #" << j + 1 << ": ";
      
      switch (d.getInfo<CL_DEVICE_TYPE>()) {
        case CL_DEVICE_TYPE_CPU:
          cout << "(CPU)";
          break;
          
        case CL_DEVICE_TYPE_GPU:
          cout << "(GPU)";
          break;
          
        case CL_DEVICE_TYPE_ACCELERATOR:
          cout << "(Accelerator)";
          break;
          
        default:
          cout << "(Unknown)";
          break;
      }
      
      cout << " " << d.getInfo<CL_DEVICE_NAME>().c_str() << endl;
    }
  }
  
  exit(0);
}

void processOptions(int& argc, char**& argv) {
  while (argc) {
    string opt(*argv);
    bool recognized = false;
    
    for (i32 i = 0; i < numOptions; ++i) {
      for (i32 j = 0; j < options[i].nopts; ++j) {
        if (opt == options[i].opts[j]) {
          options[i].handleOption(argc, argv);
          recognized = true;
          goto loopend;
        }
      }
    }
loopend:

    if (**argv != '-' && !recognized) {
      return; // Assume we've hit filenames
    }
    else if (!recognized) {
      die("Unrecognized option " + opt);
      exit(1);
    }
    
    --argc; ++argv;
  }
}

void progMon(f32 progress) {
  static f32 last = 0;
  i32 tot = 35;
  
  if (last == 0) {
    for (i32 i = 0; i < tot; ++i)
      cout << "_";
    cout << "\n";
  }
  
  i32 n = i32(progress*tot) - i32(last*tot);
  for (i32 i = 0; i < n; ++i)
    cout << '^';
  
  if (progress == 1) {
    last = 0;
    cout << "\n";
  }
  else
    last = progress;
  
  cout.flush();
}

void processImages(int& argc, char**& argv) {
  if (deviceNum < 0) {
    vector<cl::Platform> platforms;
    vector<cl::Device> devices;
    cl::Platform::get(&platforms);
    platforms[0].getDevices(CL_DEVICE_TYPE_DEFAULT, &devices);
    ClipInit(devices, valueType);
  }
  else
    ClipInit(platformNum, deviceNum, valueType);
  
  SetEnqueuesPerFinish(enqueuesPerFinish);
  
  LLInitOpParams initOpParams(Edges, numOrientations, numCurvatures,
                              curveScale);
  shared_ptr<LLInitOps> initOps;
  
  RelaxCurveOpParams rlxCurveParams(Edges, numOrientations, numCurvatures);
  shared_ptr<RelaxCurveOp> rlxCurve;
  
  i32 total = argc;
  i32 soFar = 0;
  while (argc > 0) {
    string imageFile(*argv);
    size_t lastSlash = imageFile.find_last_of('/');
    
    string dirName = ".";
    string imageName = imageFile;
    if (lastSlash != string::npos) {
      if (lastSlash >= imageFile.length() - 1)
        die("Image name can't end in a slash");
      
      dirName = imageFile.substr(0, lastSlash);
      imageName = imageFile.substr(lastSlash + 1);
    }
    
    size_t lastDot = imageName.find_last_of('.');
    if (lastDot == string::npos || lastDot >= imageName.length() - 1)
      die("No filename extension found; unable to determine type");
    
    string baseName = imageName.substr(0, lastDot);
    string extension = imageName.substr(lastDot + 1);
    
    ImageData imageData;
    try {
      ReadImage(imageFile, imageData);
    }
    catch (const exception& err) {
      die(err.what());
    }
    
    ImageBuffer imageBuffer(imageData);
    CurveBuffersPtr edges;
    CurveDataPtr edgesData;
    
    cout << "Image " << ++soFar << "/" << total << ": " << baseName << endl;
    
    std::string outputBaseName;
    
    if (runCurveInit) {
      if (!initOps.get()) {
        initOps = shared_ptr<LLInitOps>(new LLInitOps(initOpParams));
        initOps->addProgressListener(&progMon);
      }
      
      cout << "Calculating initial estimates..." << endl;
      tic();
      edges = initOps->apply(imageBuffer);
      cout << "Done in " << toc()/1000000.f << " seconds." << endl;
      
      edgesData = ReadImageDataFromBufferArray(*edges);
      outputBaseName = outputDir + "/" + baseName + "-curve-initial";
      
      if (outputMatlab)
        WriteMatlabArray(outputBaseName + ".mat", *edgesData);
      
      if (outputPdf)
        WriteLLColumnsToPDF(outputBaseName + ".pdf", *edgesData, 0.01);
    }
    if (runCurveRelax) {
      if (!rlxCurve.get()) {
        RelaxCurveOp* temp =
          new RelaxCurveOp(rlxCurveParams, curveIters, curveDelta, rlxThresh);
        rlxCurve = shared_ptr<RelaxCurveOp>(temp);
        rlxCurve->addProgressListener(&progMon);
      }
      
      cout << "Relaxing..." << endl;
      tic();
      edges = rlxCurve->apply(*edges);
      cout << "Done in " << toc()/1000000.f << " seconds." << endl;
      
      edgesData = ReadImageDataFromBufferArray(*edges);
      outputBaseName = outputDir + "/" + baseName + "-curve-relaxed";
      
      if (outputMatlab)
        WriteMatlabArray(outputBaseName + ".mat", *edgesData);
      
      if (outputPdf)
        WriteLLColumnsToPDF(outputBaseName + ".pdf", *edgesData, 0.01);
    }
    
    if (argc > 1)
      cout << endl;
    
    --argc; ++argv;
  }
}

int main(int argc, char** argv) {
  processOptions(--argc, ++argv);
  
  if (!runCurveInit && !runCurveRelax && !runFlowInit && !runFlowRelax)
    die("No commands specified; use --help to see commands");
  
  if (!argc)
    die("No image files specified; use --help for help");
  
  try {
    processImages(argc, argv);
  }
  catch (const exception& err) {
    die(err.what());
  }
}
