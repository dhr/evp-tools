#include <sstream>

#include <evp.hpp>
#include <evp/io.hpp>
#include <evp/io/imageio.hpp>
#include <evp/util/tictoc.hpp>

using namespace std;
using namespace std::tr1;
using namespace evp;

i32 platformNum = 0;
i32 deviceNum = 0;

bool runCurveInit = false;
bool runCurveRelax = false;
bool runFlowInit = false;
bool runFlowRelax = false;

i32 numOrientations = 8;
i32 numCurvatures = 3;

string outputDir;

void die(const string& msg) {
  cerr << "Error: " << msg << "." << endl;
  exit(1);
}

void showHelp() {
  cout << "Usage: evp command[s] [options] images\n";
  cout << "  list-devices\t\t List available OpenCL devices.\n";
  cout << "  curve-init\t\t Run initial curve operators.\n";
  cout << "  curve-relax\t\t Run curve relaxation (implies curve-init).\n";
//  cout << "  flow-init\t\t Run initial flow operators.\n";
//  cout << "  flow-relax\t\t Run flow relaxation (implies flow-init).\n";
  cout << "  --help\t\t Show this help text.\n";
  cout << "  --platform <id>\t Select platform <id>. Default is 0.\n";
  cout << "  --device <id>\t\t Select device <id>. Default is 0.\n";
  cout << "  --nts <n>\t\t Use <n> orientations. Defaults to 8.\n";
  cout << "  --nks <n>\t\t Use <n> curvatures. Defaults to 3.\n";
  cout << "  --output-dir <dir>\t Use <dir> for output.\n";
  cout.flush();
}

void listDevices() {
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
}

void processOptions(int& argc, char**& argv) {
  while (argc) {
    string opt(*argv);
    
    if (opt == "list-devices") {
      listDevices();
      exit(0);
    }
    else if (opt == "curve-init") {
      runCurveInit = true;
    }
    else if (opt == "curve-relax") {
      runCurveInit = true;
      runCurveRelax = true;
    }
//    else if (opt == "flow-init") {
//      runFlowInit = true;
//    }
//    else if (opt == "flow-relax") {
//      runFlowRelax = true;
//      runFlowInit = true;
//    }
    else if (opt == "--help") {
      showHelp();
      exit(0);
    }
    else if (opt == "--platform") {
      --argc; ++argv; if (!argc) die("No argument supplied to --platform");
      stringstream ss(*argv);
      ss >> platformNum;
      platformNum--;
    }
    else if (opt == "--device") {
      --argc; ++argv; if (!argc) die("No argument supplied to --device");
      stringstream ss(*argv);
      ss >> deviceNum;
      deviceNum--;
    }
    else if (opt == "--nts") {
      --argc; ++argv; if (!argc) die("No argument supplied to --nts");
      stringstream ss(*argv);
      ss >> numOrientations;
      
      if (numOrientations <= 0 || numOrientations%2 != 0)
        die("Invalid number of orientations (must be > 0 and even)");
    }
    else if (opt == "--nks") {
      --argc; ++argv; if (!argc) die("No argument supplied to --nks");
      stringstream ss(*argv);
      ss >> numCurvatures;
      
      if (numCurvatures <= 0 || numCurvatures > 5 || numCurvatures%2 == 0)
        die("Invalid number of curvatures (must > 0, <= 5, and odd)");
    }
    else if (opt == "--output-dir") {
      --argc; ++argv; if (!argc) die("No argument supplied to --output-dir");
      outputDir = *argv;
    }
    else if (**argv == '-') {
      die("Unrecognized option " + opt);
      exit(1);
    }
    else {
      return;
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
  try {
    ClipInit(platformNum, deviceNum, Float16);
  }
  catch (const exception& err) {
    die(err.what());
  }
  
  SetEnqueuesPerFinish(1000);
  
  LLInitOpParams initOpParams(Edges, 8, numCurvatures);
  shared_ptr<LLInitOps> initOps;
  
  RelaxCurveOpParams relaxCurveParams(Edges, 8, numCurvatures);
  shared_ptr<RelaxCurveOp> relaxCurve;
  
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
    
    if (outputDir.length() == 0)
      outputDir = dirName;
    
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
    
    if (!initOps.get() && runCurveInit) {
      initOps = shared_ptr<LLInitOps>(new LLInitOps(initOpParams));
      initOps->addProgressListener(&progMon);
    }
    
    if (!relaxCurve.get() && runCurveRelax) {
      relaxCurve = shared_ptr<RelaxCurveOp>(new RelaxCurveOp(relaxCurveParams));
      relaxCurve->addProgressListener(&progMon);
    }
    
    ImageBuffer imageBuffer(imageData);
    CurveBuffersPtr edges;
    CurveDataPtr edgesData;
    
    cout << "Image " << ++soFar << "/" << total << ": " << baseName << endl;
    
    std::string outputName;
    
    if (runCurveInit) {
      cout << "Calculating initial estimates..." << endl;
      tic();
      edges = initOps->apply(imageBuffer);
      cout << "Done in " << toc()/1000000.f << " seconds." << endl;
      
      edgesData = ReadImageDataFromBufferArray(*edges);
      outputName = outputDir + "/" + baseName + "-curve-initial.mat";
      WriteMatlabArray(*edgesData, outputName);
    }
    
    if (runCurveRelax) {
      cout << "Relaxing..." << endl;
      tic();
      edges = relaxCurve->apply(*edges);
      cout << "Done in " << toc()/1000000.f << " seconds." << endl;
      
      edgesData = ReadImageDataFromBufferArray(*edges);
      outputName = outputDir + "/" + baseName + "-curve-relaxed.mat";
      WriteMatlabArray(*edgesData, outputName);
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