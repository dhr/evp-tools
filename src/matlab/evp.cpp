#include <mex.h>

#include <map>
#include <sstream>

#include <evp.hpp>
#include <evp/io.hpp>

using namespace std;
using namespace std::tr1;
using namespace clip;
using namespace evp;

struct MatlabCommand {
  virtual void initialize(int nrhs, const mxArray** prhs) = 0;
  
  virtual void
  execute(int nlhs, mxArray** plhs, int nrhs, const mxArray** prhs) = 0;
  
  virtual Monitorable* getMonitorable() = 0;
};

ImageBuffer CreateBufferFromMxArray(const mxArray* array) {
  i32 rows = mxGetM(array);
  i32 cols = mxGetN(array);
  
  f32* inData = static_cast<f32*>(mxGetData(array));
  ImageBuffer image(inData, rows, cols); // MATLAB is column-major...
  
  return image;
}

template<i32 N>
void CreateBufferArrayFromMxArray(const mxArray* array,
                                  NDArray<ImageBuffer,N>& buffers) {
  const mwSize* dims = mxGetDimensions(array);
  i32 width = dims[0]; // This looks wrong, but MATLAB is column-major...
  i32 height = dims[1];
  i32 imLen = width*height;
  
  typename NDArray<ImageBuffer,N>::size_type cnvtdDims[N];
  for (i32 i = 0; i < N; ++i)
    cnvtdDims[i] = 1;
  
  i32 n = mxGetNumberOfDimensions(array) - 2;
  for (i32 i = 0; i < n; ++i)
    cnvtdDims[i] = dims[i + 2];
  
  buffers = NDArray<ImageBuffer,2>(cnvtdDims);
  
  f32* arrayData = static_cast<f32*>(mxGetData(array));
  
  i64 offset = 0;
  for (i32 i = 0; i < buffers.numElems(); ++i) {
    buffers[i] = ImageBuffer(arrayData + offset, width, height);
    offset += imLen;
  }
}

template<i32 N>
mxArray* CreateMxArrayFromBufferArray(const NDArray<ImageBuffer,N>& buffers) {
  i32 width = buffers[0].width();
  i32 height = buffers[0].height();
  i32 imLen = width*height;
  
  mwSize dims[N + 2];
  dims[0] = width;
  dims[1] = height;
  for (i32 i = 0; i < N; ++i)
    dims[i + 2] = buffers.size(i);
  
  mxArray* output = mxCreateNumericArray(N + 2, dims, mxSINGLE_CLASS, mxREAL);
  f32* outData = static_cast<f32*>(mxGetData(output));
  
  i64 offset = 0;
  for (i32 i = 0; i < buffers.numElems(); ++i) {
    buffers[i].fetchData(outData + offset);
    offset += imLen;
  }
  
  return output;
}

class LogLinCommand : public MatlabCommand {
  FeatureType feature_;
  shared_ptr<LLInitOpParams> params_;
  shared_ptr<LLInitOps> initOps_;
  
 public:
  LogLinCommand(FeatureType feature) : feature_(feature) {}
  
  void initialize(int nrhs, const mxArray** prhs) {
    i32 nt = 8, nk = 5;
    f32 scl = 1;
    
    if (nrhs > 0) {
      i32 temp = mxGetScalar(prhs[0]);
      if (temp >= 3) nt = temp;
      else
        mexWarnMsgTxt("The number of orientations should be >= 3.");
    }
    
    if (nrhs > 1) {
      i32 temp = mxGetScalar(prhs[1]);
      if (temp == 0) nk = 1;
      if (temp >= 1 && temp%2) nk = temp;
      else
        mexWarnMsgTxt("The number of curvatures should be odd and >= 1.");
    }
    
    if (nrhs > 2) {
      f32 temp = mxGetScalar(prhs[2]);
      if (temp >= 1 && temp <= 10) scl = temp;
      else
        mexWarnMsgTxt("The scaling factor should be >= 1 and <= 10.");
    }
    
    LLInitOpParams *params = new LLInitOpParams(feature_, nt, nk, scl);
    params_ = shared_ptr<LLInitOpParams>(params);
    initOps_ = shared_ptr<LLInitOps>(new LLInitOps(*params_));
  }
  
  void execute(int nlhs, mxArray** plhs, int nrhs, const mxArray** prhs) {
    ImageBuffer image = CreateBufferFromMxArray(*prhs);
    NDArray<ImageBuffer,2> initial;
    initOps_->apply(image, initial);
    
    plhs[0] = CreateMxArrayFromBufferArray(initial);
  }
  
  Monitorable* getMonitorable() { return initOps_.get(); }
};

struct LLEdgesCommand : public LogLinCommand {
  LLEdgesCommand() : LogLinCommand(Edges) {}
};

struct LLLinesCommand : public LogLinCommand {
  LLLinesCommand() : LogLinCommand(Lines) {}
};

class RelaxCurveCommand : public MatlabCommand {
  FeatureType feature_;
  shared_ptr<RelaxCurveOpParams> params_;
  shared_ptr<RelaxCurveOp> relax_;
  
 public:
  RelaxCurveCommand(FeatureType feature) : feature_(feature) {}
  
  void initialize(int nrhs, const mxArray** prhs) {
    i32 nt = 8, nk = 5;
    
    if (nrhs > 0) {
      i32 temp = mxGetScalar(prhs[0]);
      if (temp >= 3) nt = temp;
      else
        mexWarnMsgTxt("The number of orientations should be >= 3.");
    }
    
    if (nrhs > 1) {
      i32 temp = mxGetScalar(prhs[1]);
      if (temp == 0) nk = 1;
      if (temp >= 1 && temp%2 != 0) nk = temp;
      else
        mexWarnMsgTxt("The number of curvatures should be odd and >= 1.");
    }
    
    RelaxCurveOpParams *params = new RelaxCurveOpParams(feature_, nt, nk);
    params_ = shared_ptr<RelaxCurveOpParams>(params);
    relax_ = shared_ptr<RelaxCurveOp>(new RelaxCurveOp(*params_));
  }
  
  void execute(int nlhs, mxArray** plhs, int nrhs, const mxArray** prhs) {
    NDArray<ImageBuffer,2> initial;
    NDArray<ImageBuffer,2> relaxed;
    
    CreateBufferArrayFromMxArray(prhs[0], initial);
    relax_->apply(initial, relaxed);
    
    plhs[0] = CreateMxArrayFromBufferArray(relaxed);
  }
  
  Monitorable* getMonitorable() { return relax_.get(); }
};

struct RelaxEdgesCommand : public RelaxCurveCommand {
  RelaxEdgesCommand() : RelaxCurveCommand(Edges) {}
};

struct RelaxLinesCommand : public RelaxCurveCommand {
  RelaxLinesCommand() : RelaxCurveCommand(Edges) {}
};

class MexProgressMonitor {
  shared_ptr<string> callbackName_; // Must be a pointer, for copying to work
  
 public:
  MexProgressMonitor() : callbackName_(new string()) {}
 
  void setCallbackName(const mxArray* name) {
    if (name == NULL)
      *callbackName_ = "";
    else if (mxGetClassID(name) == mxCHAR_CLASS)
      *callbackName_ = mxArrayToString(name);
    else {
      *callbackName_ = "";
      mexErrMsgTxt("Invalid 'progmon' argument; should be a function name.");
    }
  }
  
  void operator()(f32 progress) {
    if (callbackName_->length()) {
      const char* temp = callbackName_->c_str();
      mxArray* name = mxCreateCharMatrixFromStrings(1, &temp);
      mxArray* val = mxCreateDoubleScalar(progress);
      mxArray* rhs[2] = {name, val};
      mexCallMATLAB(0, NULL, 2, rhs, "feval");
      mxDestroyArray(val);
      mxDestroyArray(name);
    }
  }
};

void initCommands(map< string,
                       pair< shared_ptr<MatlabCommand>,
                             bool > >& commands) {
  commands["lledges"].first =
    shared_ptr<MatlabCommand>(new LLEdgesCommand());
  commands["relaxedges"].first =
    shared_ptr<MatlabCommand>(new RelaxEdgesCommand());
  commands["lllines"].first =
    shared_ptr<MatlabCommand>(new LLLinesCommand());
  commands["relaxedges"].first =
    shared_ptr<MatlabCommand>(new RelaxLinesCommand());
}


inline void openCLNotificationHandler(const char *errInfo,
                                      const void *, size_t, void *) {
  stringstream ss;
  ss << "OpenCL Notification: " << errInfo;
	mexWarnMsgTxt(ss.str().c_str());
}

void mexFunction(int nlhs, mxArray** plhs, int nrhs, const mxArray** prhs) {
  static bool initialized = false;
  static MexProgressMonitor progMon;
  static map< string, pair< shared_ptr<MatlabCommand>, bool > > commands;
  
  if (nrhs == 0) return;
  
  if (mxGetClassID(prhs[0]) != mxCHAR_CLASS)
    mexErrMsgTxt("First argument should be a command name.");
  
  string commandName = mxArrayToString(prhs[0]);
  
  --nrhs; ++prhs;
  try {
    if (commandName == "cpu") {
      ClipInit(CPU, openCLNotificationHandler);
      if (!initialized) initCommands(commands);
      initialized = true;
      return;
    }
    
    if (commandName == "gpu") {
      ClipInit(GPU, openCLNotificationHandler);
      if (!initialized) initCommands(commands);
      initialized = true;
      return;
    }
    
    if (commandName == "device") {
      vector<cl::Platform> platforms;
      cl::Platform::get(&platforms);
      
      vector<cl::Device> devices;
      
      if (nrhs == 0) {
        for (i32 i = 0; i < i32(platforms.size()); ++i) {
          cl::Platform p = platforms[i];
          stringstream ss;
          ss << "Platform #" << i + 1 << ": "
             << p.getInfo<CL_PLATFORM_NAME>() << "\n";
          mexPrintf(ss.str().c_str());

          i32 lineLen = ss.str().length() - 1;
          for (i32 n = 0; n < lineLen; ++n)
            mexPrintf("-");
          mexPrintf("\n");
          
          p.getDevices(CL_DEVICE_TYPE_ALL, &devices);
          for (i32 j = 0; j < i32(devices.size()); ++j) {
            cl::Device d = devices[j];
            mexPrintf("  Device #%d: ", j + 1);
            
            switch (d.getInfo<CL_DEVICE_TYPE>()) {
              case CL_DEVICE_TYPE_CPU:
                mexPrintf("(CPU)");
                break;
                
              case CL_DEVICE_TYPE_GPU:
                mexPrintf("(GPU)");
                break;
                
              case CL_DEVICE_TYPE_ACCELERATOR:
                mexPrintf("(Accelerator)");
                break;
                
              default:
                mexPrintf("(Unknown)");
                break;
            }
            
            mexPrintf(" %s\n", d.getInfo<CL_DEVICE_NAME>().c_str());
          }
          
          mexPrintf("\n");
        }
        
        return;
      }
      
      if (nrhs == 1)
        mexErrMsgTxt("Select both a platform and a device number.");
      
      i32 platformIndex = i32(mxGetScalar(prhs[0])) - 1;
      i32 deviceIndex = i32(mxGetScalar(prhs[1])) - 1;
      
      if (platformIndex < 0 || platformIndex >= i32(platforms.size()))
        mexErrMsgTxt("Invalid platform index.");
      
      platforms[platformIndex].getDevices(CL_DEVICE_TYPE_ALL, &devices);
      
      if (deviceIndex < 0 || deviceIndex >= i32(devices.size()))
        mexErrMsgTxt("Invalid device index.");
      
      ClipInit(devices[deviceIndex], openCLNotificationHandler);
      if (!initialized) initCommands(commands);
      initialized = true;
      return;
    }
    
    if (!initialized) {
      ClipInit(GPU, openCLNotificationHandler);
      initCommands(commands);
      initialized = true;
    }
    
    if (commandName == "config") {
      commandName = mxArrayToString(*prhs);
      if (commands.find(commandName) == commands.end())
        mexErrMsgTxt("Cannot configure -- invalid command name.");
      
      --nrhs; ++prhs;
      pair< shared_ptr<MatlabCommand>, bool >& command = commands[commandName];
      command.first->initialize(nrhs, prhs);
      command.first->getMonitorable()->addProgressListener(progMon);
      command.second = true;
      
      return;
    }
    
    if (commandName == "progmon") {
      progMon.setCallbackName(nrhs > 0 ? *prhs : NULL);
      return;
    }
    
    if (commandName == "clearcache") {
      ClearBufferCache();
      return;
    }
    
    if (commands.find(commandName) == commands.end())
      mexErrMsgTxt("Invalid command name.");
    
    pair< shared_ptr<MatlabCommand>, bool >& command = commands[commandName];
    
    if (!command.second) {
      command.first->initialize(0, NULL);
      command.first->getMonitorable()->addProgressListener(progMon);
      command.second = true;
    }
    
    command.first->execute(nlhs, plhs, nrhs, prhs);
  }
  catch (const cl::Error& err) {
    stringstream ss;
    ss << "OpenCL Error (" << err.err() << "): " << err.what();
    mexErrMsgTxt(ss.str().c_str());
  }
  catch (const std::exception& exc) {
    stringstream ss;
    ss << "Error: " << exc.what();
    mexErrMsgTxt(ss.str().c_str());
  }
  catch (...) {
    mexErrMsgTxt("An unidentified error occurred.");
  }
}