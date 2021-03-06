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

bool runEdgeInit = false;
string edgeInitOpts[] = {"edge-init"};
string edgeInitDesc = "Run initial edge operators.";
void edgeInitHandler(int&, char**&) {
  runEdgeInit = true;
}

bool runEdgeRelax = false;
string edgeRelaxOpts[] = {"edge-relax"};
string edgeRelaxDesc = "Run edge relaxation (implies edge-init).";
void edgeRelaxHandler(int&, char**&) {
  runEdgeRelax = true;
}

bool runLineInit = false;
string lineInitOpts[] = {"line-init"};
string lineInitDesc = "Run initial line operators.";
void lineInitHandler(int&, char**&) {
  runLineInit = true;
}

bool runLineRelax = false;
string lineRelaxOpts[] = {"line-relax"};
string lineRelaxDesc = "Run line relaxation (implies line-init).";
void lineRelaxHandler(int&, char**&) {
  runLineRelax = true;
}

bool runEdgeSuppress = false;
string edgeSuppressOpts[] = {"edge-suppress"};
string edgeSuppressDesc = "Run edge suppression (implies edge- and line-init).";
void edgeSuppressHandler(int&, char**&) {
  runEdgeSuppress = true;
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
  runFlowRelax = true;
}

bool curvePdf = false;
string curvePdfOpts[] = {"curve-pdf"};
string curvePdfDesc = "Render a MAT file containing curve data as a PDF.";
void curvePdfHandler(int&, char**&) {
  curvePdf = true;
}

bool flowPdf = false;
string flowPdfOpts[] = {"flow-pdf"};
string flowPdfDesc = "Render a MAT file containing flow data as a PDF.";
void flowPdfHandler(int&, char**&) {
  flowPdf = true;
}

string helpOpts[] = {"-h", "--help"};
string helpDesc = "Show this help text and exit immediately.";
void helpHandler(int&, char**&);

i32 platformNum = 0;
string platformOpts[] = {"-p", "--platform"};
string platformArgs[] = {"id"};
string platformDesc = "Select OpenCL platform <id>.";
void platformHandler(int& argc, char**& argv) {
  getArgument(argc, argv, &platformNum);
  platformNum--;
}

i32 deviceNum = -1;
string deviceOpts[] = {"-d", "--device"};
string deviceArgs[] = {"id"};
string deviceDesc = "Select OpenCL device <id>.";
void deviceHandler(int& argc, char**& argv) {
  getArgument(argc, argv, &deviceNum);
  deviceNum--;
}

ValueType valueType = Float32;
string valueTypeOpts[] = {"-b", "--bit-depth"};
string valueTypeArgs[] = {"n"};
string valueTypeDesc = "Use <n> bits for image storage (16 or default of 32).";
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

ImageBufferType bufferType = Texture;
string bufferTypeOpts[] = {"--buf-type"};
string bufferTypeArgs[] = {"n"};
string bufferTypeDesc = "Buffer type ('Global' or default of 'Texture').";
void bufferTypeHandler(int& argc, char**& argv) {
  string name, name0;
  getArgument(argc, argv, &name0);
  name.resize(name0.length());
  transform(name0.begin(), name0.end(), name.begin(), ::tolower);
  
  if (name == "global")
    bufferType = Global;
  else if (name == "texture")
    bufferType = Texture;
  else
    die("Invalid storage type " + name0 + ", should be 'Global' or 'Texture'");
}

i32 enqueuesPerFinish = 5000;
string epfOpts[] = {"--max-enqueues"};
string epfArgs[] = {"n"};
string epfDesc = "Let device catch up after <n> (=5000) enqueues.";
void epfHandler(int& argc, char**& argv) {
  getArgument(argc, argv, &enqueuesPerFinish);
  if (enqueuesPerFinish <= 0)
    die("Invalid number of enqueues per finish (must be > 0)");
}

i32 numOrientations = 8;
string numOrientationsOpts[] = {"-t", "--orientations"};
string numOrientationsArgs[] = {"n"};
string numOrientationsDesc = "Use <n> (=8) orientations.";
void numOrientationsHandler(int& argc, char**& argv) {
  getArgument(argc, argv, &numOrientations);
  if (numOrientations <= 0 || numOrientations%2 != 0)
    die("Invalid number of orientations (must be > 0 and even)");
}

i32 numCurvatures = 3;
string numCurvaturesOpts[] = {"-k", "--curvatures"};
string numCurvaturesArgs[] = {"n"};
string numCurvaturesDesc = "Use <n> (=3) curvatures.";
void numCurvaturesHandler(int& argc, char**& argv) {
  getArgument(argc, argv, &numCurvatures);
  if (numCurvatures <= 0 || numCurvatures > 5 || numCurvatures%2 == 0)
    die("Invalid number of curvatures (must > 0, <= 5, and odd)");
}

f32 curveScale = 1.f;
string curveScaleOpts[] = {"--curve-scale"};
string curveScaleArgs[] = {"s"};
string curveScaleDesc = "Scale initial curve operators by <s> (=1.0).";
void curveScaleHandler(int& argc, char**& argv) {
  getArgument(argc, argv, &curveScale);
  if (curveScale < 1)
    die("Invalid curve scaling (must be >= 1.0)");
}

f32 rlxThresh = 0.f;
string rlxThreshOpts[] = {"--rlx-thresh"};
string rlxThreshArgs[] = {"t"};
string rlxThreshDesc = "Threshold relaxing curves with <t> (=0.0).";
void rlxThreshHandler(int& argc, char**& argv) {
  getArgument(argc, argv, &rlxThresh);
  if (rlxThresh < 0 || rlxThresh >= 1)
    die("Invalid relaxation threshold (must be >= 0 and < 1)");
}

i32 curveIters = 5;
string curveItersOpts[] = {"--curve-iters"};
string curveItersArgs[] = {"n"};
string curveItersDesc = "Use <n> (=5) iterations for curve relaxation.";
void curveItersHandler(int& argc, char**& argv) {
  getArgument(argc, argv, &curveIters);
  if (curveIters <= 0)
    die("Invalid number of iterations (must be > 0)");
}

f32 curveDelta = 1.f;
string curveDeltaOpts[] = {"--curve-delta"};
string curveDeltaArgs[] = {"d"};
string curveDeltaDesc = "Use <d> (=1.0) for curve relaxation delta.";
void curveDeltaHandler(int& argc, char**& argv) {
  getArgument(argc, argv, &curveDelta);
}

i32 flowIters = 30;
string flowItersOpts[] = {"--flow-iters"};
string flowItersArgs[] = {"n"};
string flowItersDesc = "Use <n> (=30) iterations for flow relaxation.";
void flowItersHandler(int& argc, char**& argv) {
  getArgument(argc, argv, &flowIters);
  if (flowIters <= 0)
    die("Invalid number of iterations (must be > 0)");
}

f32 flowDelta = 0.25f;
string flowDeltaOpts[] = {"--flow-delta"};
string flowDeltaArgs[] = {"d"};
string flowDeltaDesc = "Use <d> (=0.25) for flow relaxation delta.";
void flowDeltaHandler(int& argc, char**& argv) {
  getArgument(argc, argv, &flowDelta);
}

f32 flowMinSupport = 0.25f;
string flowMinSupportOpts[] = {"--flow-min-support"};
string flowMinSupportArgs[] = {"s"};
string flowMinSupportDesc = "Require <s> (=0.25) as minimal flow support.";
void flowMinSupportHandler(int& argc, char**& argv) {
  getArgument(argc, argv, &flowMinSupport);
}

f32 flowInitSize = 2.f;
string flowInitSizeOpts[] = {"--flow-init-size"};
string flowInitSizeArgs[] = {"s"};
string flowInitSizeDesc = "Use <s> (=2.0) for initial flow operator scale.";
void flowInitSizeHandler(int& argc, char**& argv) {
  getArgument(argc, argv, &flowInitSize);
}

i32 flowThetaJitters = 1;
string flowThetaJittersOpts[] = {"--orientation-jitters"};
string flowThetaJittersArgs[] = {"n"};
string flowThetaJittersDesc = "Use <n> (=1) for number of orientation jitters.";
void flowThetaJittersHandler(int& argc, char**& argv) {
  getArgument(argc, argv, &flowThetaJitters);
}

i32 flowNumScaleJitters = 1;
string flowNumScaleJittersOpts[] = {"--scale-jitters"};
string flowNumScaleJittersArgs[] = {"n"};
string flowNumScaleJittersDesc = "Use <n> (=1) for number of scale jitters.";
void flowNumScaleJittersHandler(int& argc, char**& argv) {
  getArgument(argc, argv, &flowNumScaleJitters);
}

f32 flowMinConf = 0.7f;
string flowMinConfOpts[] = {"--flow-min-conf"};
string flowMinConfArgs[] = {"c"};
string flowMinConfDesc = "Use <c> (=0.7) for minimum initial flow confidences.";
void flowMinConfHandler(int& argc, char**& argv) {
  getArgument(argc, argv, &flowMinConf);
}

f32 flowInitThresh = 0.01f;
string flowInitThreshOpts[] = {"--flow-init-thresh"};
string flowInitThreshArgs[] = {"t"};
string flowInitThreshDesc = "Use <t> (=0.01) for initial flow threshold.";
void flowInitThreshHandler(int& argc, char**& argv) {
  getArgument(argc, argv, &flowInitThresh);
}

enum FlowInitOpType { GradientInit, GaborInit, PushPullInit };
FlowInitOpType flowInitType = GaborInit;
string flowInitTypeOpts[] = {"--flow-init-op"};
string flowInitTypeArgs[] = {"t"};
string flowInitTypeDesc = "Initial flow op ('Gradient', 'PushPull' or default 'Gabor').";
void flowInitTypeHandler(int& argc, char**& argv) {
  string name, name0;
  getArgument(argc, argv, &name0);
  name.resize(name0.length());
  transform(name0.begin(), name0.end(), name.begin(), ::tolower);
  
  if (name == "gradient")
    flowInitType = GradientInit;
  else if (name == "gabor")
    flowInitType = GaborInit;
  else if (name == "pushpull")
    flowInitType = PushPullInit;
  else {
    die("Invalid initial flow op type " + name0 +
        ", should be 'Gradient', 'PushPull', or 'Gabor'");
  }
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

f32 pdfDarken = 1; // This confusingly is the value for no darkening...
string pdfDarkenOpts[] = {"--pdf-darken"};
string pdfDarkenArgs[] = {"d"};
string pdfDarkenDesc = "Darken PDF by amount <d> (=0.0).";
void pdfDarkenHandler(int& argc, char**& argv) {
  getArgument(argc, argv, &pdfDarken);
  if (pdfDarken < 0 || pdfDarken > 1)
    die("Darkening parameter must be between in range [0, 1]."); 
  pdfDarken = 1 - pdfDarken;
}

string outputDir = ".";
string outputDirOpts[] = {"-o", "--output-dir"};
string outputDirArgs[] = {"dir"};
string outputDirDesc = "Write output to <dir>. Defaults to current directory.";
void outputDirHandler(int& argc, char**& argv) {
  getArgument(argc, argv, &outputDir);
}

f32 pdfThresh = 0.01f;
string pdfThreshOpts[] = {"--pdf-thresh"};
string pdfThreshArgs[] = {"t"};
string pdfThreshDesc = "Use <t> (=0.01) for thresholding PDF output.";
void pdfThreshHandler(int& argc, char**& argv) {
  getArgument(argc, argv, &pdfThresh);
}

typedef struct OptionEntry {
  string* opts; int nopts;
  string* args; int nargs;
  void (*handleOption)(int& argc, char**& argv);
  string description;
  bool isCommand;
} OptionEntry;

#define OPTION_COMMAND_ENTRY(name) \
  {name##Opts, sizeof(name##Opts)/sizeof(string), NULL, 0, \
   &name##Handler, name##Desc, true}

#define OPTION_FLAG_ENTRY(name) \
  {name##Opts, sizeof(name##Opts)/sizeof(string), NULL, 0, \
   &name##Handler, name##Desc, false}

#define OPTION_ARGS_ENTRY(name) \
  {name##Opts, sizeof(name##Opts)/sizeof(string), \
   name##Args, sizeof(name##Args)/sizeof(string), \
   &name##Handler, name##Desc, false}

OptionEntry options[] = {
  OPTION_COMMAND_ENTRY(listDevices),
  OPTION_COMMAND_ENTRY(edgeInit),
  OPTION_COMMAND_ENTRY(edgeRelax),
  OPTION_COMMAND_ENTRY(lineInit),
  OPTION_COMMAND_ENTRY(lineRelax),
  OPTION_COMMAND_ENTRY(edgeSuppress),
  OPTION_COMMAND_ENTRY(flowInit),
  OPTION_COMMAND_ENTRY(flowRelax),
  OPTION_COMMAND_ENTRY(curvePdf),
  OPTION_COMMAND_ENTRY(flowPdf),
  OPTION_COMMAND_ENTRY(help),
  OPTION_ARGS_ENTRY(platform),
  OPTION_ARGS_ENTRY(device),
  OPTION_ARGS_ENTRY(valueType),
  OPTION_ARGS_ENTRY(bufferType),
  OPTION_ARGS_ENTRY(epf),
  OPTION_ARGS_ENTRY(numOrientations),
  OPTION_ARGS_ENTRY(numCurvatures),
  OPTION_ARGS_ENTRY(curveScale),
  OPTION_ARGS_ENTRY(rlxThresh),
  OPTION_ARGS_ENTRY(curveIters),
  OPTION_ARGS_ENTRY(curveDelta),
  OPTION_ARGS_ENTRY(flowInitSize),
  OPTION_ARGS_ENTRY(flowMinConf),
  OPTION_ARGS_ENTRY(flowThetaJitters),
  OPTION_ARGS_ENTRY(flowNumScaleJitters),
  OPTION_ARGS_ENTRY(flowInitType),
  OPTION_ARGS_ENTRY(flowInitThresh),
  OPTION_ARGS_ENTRY(flowIters),
  OPTION_ARGS_ENTRY(flowDelta),
  OPTION_ARGS_ENTRY(flowMinSupport),
  OPTION_FLAG_ENTRY(noMatlab),
  OPTION_FLAG_ENTRY(pdf),
  OPTION_ARGS_ENTRY(pdfThresh),
  OPTION_ARGS_ENTRY(pdfDarken),
  OPTION_ARGS_ENTRY(outputDir)
};
i32 numOptions = sizeof(options)/sizeof(OptionEntry);

void helpHandler(int&, char**&) {
  cout << "Usage: evp command[s] [options] inputs\n";
  
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

bool processOptions(int& argc, char**& argv) {
  bool commandSpecified = false;
  while (argc) {
    string opt(*argv);
    bool recognized = false;
    
    for (i32 i = 0; i < numOptions; ++i) {
      for (i32 j = 0; j < options[i].nopts; ++j) {
        if (opt == options[i].opts[j]) {
          options[i].handleOption(argc, argv);
          recognized = true;
          commandSpecified = commandSpecified || options[i].isCommand;
          goto loopend;
        }
      }
    }
loopend:

    if (**argv != '-' && !recognized) {
      return commandSpecified; // Assume we've hit filenames
    }
    else if (!recognized) {
      die("Unrecognized option " + opt);
    }
    
    --argc; ++argv;
  }
  
  return commandSpecified;
}

void processImages(int& argc, char**& argv) {
  ProgramSettings settings = CLIP_DEFAULT_PROGRAM_SETTINGS;
  settings.memoryValueType = valueType;
  settings.bufferType = bufferType;
  
  if (deviceNum < 0) {
    vector<cl::Platform> platforms;
    vector<cl::Device> devices;
    cl::Platform::get(&platforms);
    platforms[0].getDevices(CL_DEVICE_TYPE_DEFAULT, &devices);
    ClipInit(devices, settings);
  }
  else
    ClipInit(platformNum, deviceNum, settings);
  
  SetEnqueuesPerFinish(enqueuesPerFinish);
  
  LLInitOpParams edgeInitOpParams(Edges, numOrientations, numCurvatures,
                                  curveScale);
  shared_ptr<LLInitOps> edgeInitOps;
  
  LLInitOpParams lineInitOpParams(Lines, numOrientations, numCurvatures,
                                  curveScale);
  shared_ptr<LLInitOps> lineInitOps;
  
  RelaxCurveOpParams edgeRlxCurveParams(Edges, numOrientations, numCurvatures);
  shared_ptr<RelaxCurveOp> edgeRlxCurve;
  
  RelaxCurveOpParams lineRlxCurveParams(Lines, numOrientations, numCurvatures);
  shared_ptr<RelaxCurveOp> lineRlxCurve;
  
  SuppressLineEdgesOpParams
    suppressLineEdgesOpParams(numOrientations, numCurvatures);
  shared_ptr<SuppressLineEdgesOp> edgeSuppressOps;
  
  FlowInitOpParams flowInitOpParams(numOrientations, numCurvatures);
  flowInitOpParams.size = flowInitSize;
  flowInitOpParams.minConf = flowMinConf;
  flowInitOpParams.threshold = flowInitThresh;
  shared_ptr<FlowInitOps> flowInitOps;
  
  RelaxFlowOpParams rlxFlowParams(numOrientations, numCurvatures);
  rlxFlowParams.minSupport = flowMinSupport;
  shared_ptr<RelaxFlowOp> rlxFlowOp;
  
  i32 total = argc;
  i32 soFar = 0;
  while (argc > 0) {
    string imageFile(*argv);
    size_t lastSlash = imageFile.find_last_of('/');
    
    string dirName = ".";
    string imageName = imageFile;
    if (lastSlash != string::npos) {
      if (lastSlash >= imageFile.length() - 1)
        die("Input name can't end in a slash");
      
      dirName = imageFile.substr(0, lastSlash);
      imageName = imageFile.substr(lastSlash + 1);
    }
    
    size_t lastDot = imageName.find_last_of('.');
    if (lastDot == string::npos || lastDot >= imageName.length() - 1)
      die("No filename extension found; unable to determine type");
    
    string baseName = imageName.substr(0, lastDot);
    string extension = imageName.substr(lastDot + 1);
    
    ImageData imageData;
    ImageBuffer imageBuffer;
    
    bool isMatFile = extension == "mat";
    if (!isMatFile) {
      try {
        ReadImage(imageFile, imageData);
        imageBuffer = ImageBuffer(imageData);
      }
      catch (const exception& err) {
        die(err.what());
      }
    }
    
    CurveBuffersPtr edges, lines;
    CurveDataPtr edgesData, linesData;
    FlowBuffersPtr flow;
    FlowDataPtr flowData;
    
    cout << "Input " << ++soFar << "/" << total << ": " << baseName << endl;
    
    string outputBaseName = outputDir + "/" + baseName;
    
    if (curvePdf || flowPdf) {
      if (isMatFile) {
        if (curvePdf) {
          CurveDataPtr localCurveData = ReadMatlabArray<2>(imageFile);
          if (!localCurveData.get())
            die("Failed to read curve data");
          cout << "Writing curve data to PDF... " << flush;
          WriteLLColumnsToPDF(outputBaseName + ".pdf", *localCurveData,
                              pdfThresh, pdfDarken);
          cout << "done." << endl;
        }
        else {
          FlowDataPtr localFlowData = ReadMatlabArray<3>(imageFile);
          if (!localFlowData.get())
            die("Failed to read flow data");
          cout << "Writing flow data to PDF... " << flush;
          WriteFlowToPDF(outputBaseName + ".pdf", *localFlowData,
                         pdfThresh, pdfDarken);
          cout << "done." << endl;
        }
      }
      else {
        cerr << "Didn't write PDF for " << imageName
             << "... not a MAT file" << endl;
      }
    }
    
    if (runEdgeInit || (runEdgeRelax && !isMatFile)) {
      if (!edgeInitOps.get()) {
        edgeInitOps = shared_ptr<LLInitOps>(new LLInitOps(edgeInitOpParams));
        edgeInitOps->addProgressListener(&TextualProgressMonitor);
      }
      
      cout << "Calculating initial edge estimates..." << endl;
      tic();
      edges = edgeInitOps->apply(imageBuffer);
      cout << "Done in " << toc()/1000000.f << " seconds." << endl;
      
      edgesData = BufferArrayToDataArray(*edges);
      
      string outputName = outputBaseName + "-edge-initial";
      
      if (outputMatlab)
        WriteMatlabArray(outputName + ".mat", *edgesData);
      
      if (outputPdf) {
        WriteLLColumnsToPDF(outputName + ".pdf", *edgesData,
                            pdfThresh, pdfDarken);
      }
    }
    if (runEdgeRelax) {
      if (isMatFile) {
        if (!(edgesData = ReadMatlabArray<2>(imageFile)).get())
          die("Failed to read edge data");
        edges = DataArrayToBufferArray(*edgesData);
      }
      
      if (!edgeRlxCurve.get()) {
        RelaxCurveOp* temp =
          new RelaxCurveOp(edgeRlxCurveParams, curveIters,
                           curveDelta, rlxThresh);
        edgeRlxCurve = shared_ptr<RelaxCurveOp>(temp);
        edgeRlxCurve->addProgressListener(&TextualProgressMonitor);
      }
      
      cout << "Relaxing edges..." << endl;
      tic();
      edges = edgeRlxCurve->apply(*edges);
      cout << "Done in " << toc()/1000000.f << " seconds." << endl;
      
      edgesData = BufferArrayToDataArray(*edges);
      string outputName = outputBaseName + "-edge-relaxed";
      
      if (outputMatlab)
        WriteMatlabArray(outputName + ".mat", *edgesData);
      
      if (outputPdf) {
        WriteLLColumnsToPDF(outputName + ".pdf", *edgesData,
                            pdfThresh, pdfDarken);
      }
    }
    
    if (runLineInit || (runLineRelax && !isMatFile)) {
      if (!lineInitOps.get()) {
        lineInitOps = shared_ptr<LLInitOps>(new LLInitOps(lineInitOpParams));
        lineInitOps->addProgressListener(&TextualProgressMonitor);
      }
      
      cout << "Calculating initial line estimates..." << endl;
      tic();
      lines = lineInitOps->apply(imageBuffer);
      cout << "Done in " << toc()/1000000.f << " seconds." << endl;
      
      linesData = BufferArrayToDataArray(*lines);
      string outputName = outputBaseName + "-line-initial";
      
      if (outputMatlab)
        WriteMatlabArray(outputName + ".mat", *linesData);
      
      if (outputPdf) {
        WriteLLColumnsToPDF(outputName + ".pdf", *linesData,
                            pdfThresh, pdfDarken);
      }
    }
    if (runLineRelax) {
      if (isMatFile) {
        if (!(linesData = ReadMatlabArray<2>(imageFile)))
          die("Failed to read line data");
        lines = DataArrayToBufferArray(*linesData);
      }
      
      if (!lineRlxCurve.get()) {
        RelaxCurveOp* temp =
          new RelaxCurveOp(lineRlxCurveParams, curveIters,
                           curveDelta, rlxThresh);
        lineRlxCurve = shared_ptr<RelaxCurveOp>(temp);
        lineRlxCurve->addProgressListener(&TextualProgressMonitor);
      }
      
      cout << "Relaxing lines..." << endl;
      tic();
      lines = lineRlxCurve->apply(*lines);
      cout << "Done in " << toc()/1000000.f << " seconds." << endl;
      
      linesData = BufferArrayToDataArray(*lines);
      string outputName = outputBaseName + "-line-relaxed";
      
      if (outputMatlab)
        WriteMatlabArray(outputName + ".mat", *linesData);
      
      if (outputPdf)
        WriteLLColumnsToPDF(outputName + ".pdf", *linesData,
                            pdfThresh, pdfDarken);
    }
    
    if (runEdgeSuppress) {
      if (!edgeSuppressOps.get()) {
        edgeSuppressOps = shared_ptr<SuppressLineEdgesOp>
          (new SuppressLineEdgesOp(suppressLineEdgesOpParams));
        edgeSuppressOps->addProgressListener(&TextualProgressMonitor);
      }
      
      cout << "Suppressing edges around lines..." << endl;
      tic();
      edges = edgeSuppressOps->apply(*edges, *lines);
      cout << "Done in " << toc()/1000000.f << " seconds." << endl;
      
      edgesData = BufferArrayToDataArray(*edges);
      string outputName = outputBaseName + "-edge-suppressed";
      
      if (outputMatlab)
        WriteMatlabArray(outputName + ".mat", *edgesData);
      
      if (outputPdf) {
        WriteLLColumnsToPDF(outputName + ".pdf", *edgesData,
                            pdfThresh, pdfDarken);
      }
    }
    
    if (runFlowInit || (runFlowRelax && !isMatFile)) {
      if (!flowInitOps.get()) {
        switch (flowInitType) {
          case GradientInit:
            flowInitOps = shared_ptr<FlowInitOps>
              (new GradientFlowInitOps(flowInitOpParams));
            break;
          
          case GaborInit:
            flowInitOps = shared_ptr<FlowInitOps>
              (new JitteredFlowInitOps(flowInitOpParams,
                                       flowThetaJitters,
                                       flowNumScaleJitters));
            break;
            
          case PushPullInit:
            flowInitOps = shared_ptr<FlowInitOps>
              (new PushPullInitOps(flowInitOpParams,
                                   flowThetaJitters,
                                   flowNumScaleJitters));
        }
        
        flowInitOps->addProgressListener(&TextualProgressMonitor);
      }
      
      cout << "Calculating initial flow estimates..." << endl;
      tic();
      flow = flowInitOps->apply(imageBuffer);
      cout << "Done in " << toc()/1000000.f << " seconds." << endl;
      
      flowData = BufferArrayToDataArray(*flow);
      string outputName = outputBaseName + "-flow-initial";
      
      if (outputMatlab)
        WriteMatlabArray(outputName + ".mat", *flowData);
      
      if (outputPdf)
        WriteFlowToPDF(outputName + ".pdf", *flowData, pdfThresh, pdfDarken);
    }
    if (runFlowRelax) {
      if (isMatFile) {
        if (!(flowData = ReadMatlabArray<3>(imageFile)).get())
          die("Failed to read flow data");
        flow = DataArrayToBufferArray(*flowData);
      }
      
      if (!rlxFlowOp.get()) {
        RelaxFlowOp* temp =
          new RelaxFlowOp(rlxFlowParams, flowIters, flowDelta);
        rlxFlowOp = shared_ptr<RelaxFlowOp>(temp);
        rlxFlowOp->addProgressListener(&TextualProgressMonitor);
      }
      
      cout << "Relaxing flow..." << endl;
      tic();
      flow = rlxFlowOp->apply(*flow);
      cout << "Done in " << toc()/1000000.f << " seconds." << endl;
      
      flowData = BufferArrayToDataArray(*flow);
      string outputName = outputBaseName + "-flow-relaxed";
      
      if (outputMatlab)
        WriteMatlabArray(outputName + ".mat", *flowData);
      
      if (outputPdf)
        WriteFlowToPDF(outputName + ".pdf", *flowData, pdfThresh, pdfDarken);
    }
    
    if (argc > 1)
      cout << endl;
    
    --argc; ++argv;
  }
}

int main(int argc, char** argv) {
  if (!processOptions(--argc, ++argv))
    die("No commands specified; use --help to see commands");
  
  if (!argc)
    die("No input files specified");
  
  try {
    processImages(argc, argv);
  }
  catch (const exception& err) {
    die(err.what());
  }
}
