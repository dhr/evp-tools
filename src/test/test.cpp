#include <string>
#include <vector>

#define USE_TEXTURES 0

#include <clip.hpp>

#define NUM_THETAS 8
#define NUM_CURVS 1

#define IMAGE1 "leaves.jpg"
#define IMAGE2 "ocean-wave.jpg"

#define OUTPUT "output/"
#define IMAGES "images/"

//#define THING_TO_DO profileOp("filter")
//#define THING_TO_DO testGabor()
//#define THING_TO_DO testFlowModel()
//#define THING_TO_DO testFlowCompatibilities()
#define THING_TO_DO testCurveCompatibilities()

#include <evp.hpp>
#include <evp/io.hpp>
#include <evp/io/imageio.hpp>
#include <evp/util/tictoc.hpp>

using namespace clip;
using namespace evp;

inline void getImages(const std::vector<std::string> &imNames,
                      ImageBuffer* imBufs) {
  std::vector<std::string>::const_iterator it;
  for (it = imNames.begin(); it != imNames.end(); ++it, ++imBufs) {
    ImageData image;
    if (!ReadJpeg(IMAGES + *it, image)) {
      std::cerr << "Error reading image file!" << std::endl;
      exit(1);
    }
    
#if !USE_TEXTURES && !USE_CPU
    *imBufs = ImageBuffer(image);
#elif !USE_CPU
    *imBufs = ImageBuffer(image, 4, 1);
#else
    *imBufs = ImageBuffer(image, 1, 1);
#endif
  }
}

inline void testCurveCompatibilities() {
  RelaxCurveOpParams compatParams(Edges);
  CurveSupportOp connections(0.f, 0.1f, compatParams);
  const NDArray<ImageData,4> &compats = connections.components();
  WriteMatlabArray(OUTPUT "compatibilities-1.mat", compats);
//  return;
  
  WriteCurveCompatibilitiesToPDF(OUTPUT "curve-test-compatibility.pdf",
                                 compats, compatParams.piSymmetric());
  
  ImageData reduction(compats(0, 0, 0, 0).width(),
                      compats(0, 0, 0, 0).height());
  for (int kii = 0; kii < compats.size(3); kii++) {
    for (int tii = 0; tii < compats.size(2); tii++) {
      for (int ni = 0; ni < compats.size(1); ni++) {
        for (int tani = 0; tani < compats.size(0); tani++) {
          reduction.data() += compats(tani, 0, 0, 0).data();
        }
      }
    }
  }
  
  std::stringstream sstream;
  sstream << OUTPUT "curve-test-compatibility.csv";
  WriteCSV(sstream.str(), reduction);
}

inline void testFlowModel() {
  int size = 9;
  f64 scale = 1;
  
  FlowModel model(0, 0, 0*M_PI/8, 0.f, 1.f);
  std::ofstream out(OUTPUT "flow-test-model.pdf", std::ios::out);
  PDFWriter writer(out, size, size);
  writer.setLineCapStyle(1);
  writer.translate(size/2, size/2);
  writer.setLineWidth(0.1);
  
  for (i32 x = -size/2; x <= size/2; x++) {
    for (i32 y = -size/2; y <= size/2; y++) {
      f64 theta = model.thetaAt(scale*x, scale*y);
      f64 cosTheta = cos(theta);
      f64 sinTheta = sin(theta);
      writer.drawLine(x + 0.5 - 0.5*cosTheta,
                      y + 0.5 - 0.5*sinTheta,
                      x + 0.5 + 0.5*cosTheta,
                      y + 0.5 + 0.5*sinTheta);
    }
  }
  
  writer.stroke();
  writer.finish();
}

inline void testFlowCompatibilities() {
  RelaxFlowOpParams params;
  params.flowSupport = CreateUniformInhibitionFlowSupportOp;
//  params.flowSupport = CreateInhibitionlessFlowSupportOp;
  params.numCurvatures = 5;
  params.subsamples = 3;
  
  for (i32 ti = 0; ti < params.numOrientations; ti++) {
    float t = ti*params.orientationStep;
    for (i32 kti = 0; kti < params.numCurvatures; kti++) {
      float kt = (kti - params.numCurvatures/2)*params.curvatureStep;
      for (i32 kni = 0; kni < params.numCurvatures; kni++) {
        float kn = (kni - params.numCurvatures/2)*params.curvatureStep;
        
        FlowSupportOpPtr connectionsOp(params.flowSupport(t, kt, kn, params));
        const NDArray<ImageData,3> &kernels = connectionsOp->kernels();
        
        std::stringstream ss;
        ss << "Compatibilities/compat-"
           << ti << "-" << kti << "-" << kni << ".pdf";
        WriteFlowCompatToPDF(ss.str(), kernels);
        
//        for (i32 ktj = 0; ktj < params.numCurvatures; ktj++) {
//          for (i32 knj = 0; knj < params.numCurvatures; knj++) {
//            ss.str("");
//            ss << OUTPUT "compat-" << ti << "-" << kti << "-" << kni << "-"
//               << ktj << "-" << knj << ".pdf";
//            WriteFlowCompatToPDF(ss.str(), kernels, ktj, knj);
//          }
//        }
      }
    }
  }
}

inline void enforceRotationalCurvatures(const FlowInitOpParams& initFlowParams,
                                        NDArray<ImageData,3>& flowCols) {
  float width = flowCols[0].width();
  float height = flowCols[1].width();
  float xc = width/2.f;
  float yc = height/2.f;
  
  float ts = initFlowParams.orientationStep;
  int nks = initFlowParams.numCurvatures;
  for (i32 ti = 0; ti < i32(flowCols.size(0)); ++ti) {
    for (i32 kt = 0; kt < i32(flowCols.size(1)); ++kt) {
      for (i32 kn = 0; kn < i32(flowCols.size(2)); ++kn) {
        ImageData& imdat = flowCols(ti, kt, kn);
        float cs = initFlowParams.curvatureStep;
        
        for (i32 y = 0; y < height; ++y) {
          for (i32 x = 0; x < width; ++x) {
            float xd = x - xc;
            float yd = y - yc;
            float t = atan2f(xd, -yd);
            float r = sqrtf(xd*xd + yd*yd);
            float k = 1/r;
            
            if (cmod(t - -ts/2) < 0 || cmod(t - (M_PI - ts/2)) > 0)
                k = -k;
            
            if (kn == nks/2 &&
                (fabsf(k - (kt - nks/2)*cs) < cs/2 ||
                 (kt == 0 && k < -nks/2*cs) ||
                 (kt == nks - 1 && k > nks/2*cs))) {
              continue;
            }
            
            imdat(x, y) = 0;
          }
        }
      }
    }
  }
}

inline void makePerfectRotationalFlow(FlowInitOpParams& initFlowParams,
                               NDArray<ImageData,3>& flowCols) {
  float width = flowCols[0].width();
  float height = flowCols[1].width();
  float xc = (width - 1)/2;
  float yc = (height - 1)/2;
  
  int nks = initFlowParams.numCurvatures;
  for (i32 ti = 0; ti < i32(flowCols.size(0)); ++ti) {
    for (i32 kt = 0; kt < i32(flowCols.size(1)); ++kt) {
      for (i32 kn = 0; kn < i32(flowCols.size(2)); ++kn) {
        ImageData& imdat = flowCols(ti, kt, kn);
        float ts = initFlowParams.orientationStep;
        float cs = initFlowParams.curvatureStep;
        
        for (i32 y = 0; y < height; ++y) {
          for (i32 x = 0; x < width; ++x) {
            float xd = x - xc;
            float yd = y - yc;
            float r = sqrtf(xd*xd + yd*yd);
            float t = atan2f(xd, -yd);
            float k = 1/r;
            
            if (cmod(t - -ts/2) < 0 || cmod(t - (M_PI - ts/2)) > 0)
                k = -k;
            
            if (kn == nks/2 &&
                fabsf(cmod(t - ti*ts, M_PI)) < ts/2 &&
                (fabsf(k - (kt - nks/2)*cs) < cs/2 ||
                 (kt == 0 && k < -nks/2*cs) ||
                 (kt == nks - 1 && k > nks/2*cs))) {
              imdat(x, y) = 0.2;
              continue;
            }
            
            imdat(x, y) = 0;
          }
        }
      }
    }
  }
}

void testSimpleOp(cl::Kernel &test,
                  ImageBuffer im1Buf, ImageBuffer im2Buf,
                  ImageBuffer output, cl::Event &event) {
  cl::KernelFunctor testFunctor = test.bind(CurrentQueue(), im1Buf.offset(),
      im1Buf.itemRange(), im1Buf.groupRange());
  
  event = testFunctor(im1Buf.mem(), im2Buf.mem(), output.mem());
  
  event.wait();
}

void testFilter(cl::Kernel &filter, ImageBuffer im1Buf, ImageBuffer output,
                cl::Event &event) {
//  srand(4);
  ImageData kernel = MakeGabor(M_PI*3/8, 4, 0, 4, 1.5);
//  ImageData kernel(fsize, fsize);
//  for (int y = 0; y < fsize; ++y) {
//    for (int x = 0; x < fsize; ++x) {
//      kernel(x, y) = Gaussian(x - fsize/2, sqrt(2.f), false)*
//                              DGaussian(y - fsize/2, sqrt(2.f), 1);
////      kernel(x, y) = rand();
//    }
//  }
  kernel.balance().normalize();
  
#if 0
  ImageBuffer kernBuf(kernel, CurrentFilterValueType(), 4, 1, Texture);
#else
  ImageBuffer kernBuf(kernel, CurrentFilterValueType(), 4, 1, Global);
#endif
  
  int nElems = 1 ? 1 : 4;
  int wgWidth = 1 ? 1 : 16;
  int wgHeight = 1 ? 1 : 16;
  
  cl::NDRange wkItmRange(im1Buf.paddedWidth()/nElems, im1Buf.paddedHeight());
  cl::NDRange wkGrpRange(wgWidth, wgHeight);
  
  cl::KernelFunctor filterFunctor =
    filter.bind(CurrentQueue(), cl::NullRange, wkItmRange, wkGrpRange);
  
#if !USE_TEXTURES && !USE_CPU
  int fsize = 19;
  int halfKernWidth = kernBuf.paddedWidth()/2;
  int apronRem = halfKernWidth%4;
  int apronWidth = halfKernWidth + (apronRem ? 4 - apronRem : 0);
  cl::LocalSpaceArg imCacheSize = {(4*wgWidth + 2*apronWidth)*
                                   (wgHeight + 2*(fsize/2) - !(fsize%2))*
                                   sizeof(float)};
  cl::LocalSpaceArg filtCacheSize = {kernBuf.paddedWidth()*
                                     kernBuf.paddedHeight()*sizeof(float)};
//  cl::LocalSpaceArg filtCacheSize = {1};
#elif 0
  cl::LocalSpaceArg imCacheSize = {(4*wgWidth + 2*(fsize/2))*
                                   (wgHeight + 2*(fsize/2) - !(fsize%2))*
                                   sizeof(float)};
#endif
  
  event = filterFunctor(im1Buf.mem(),
#if !USE_TEXTURES && !USE_CPU
                        imCacheSize,
#endif
                        kernBuf.mem(),
#if !USE_TEXTURES && !USE_CPU
                        filtCacheSize,
#endif
                        kernBuf.paddedWidth(),
                        kernBuf.paddedHeight(),
                        output.mem());
  
  event.wait();
}

void profileOp(std::string kernelName) {
  ClipInit(clip::GPU);
  
  std::vector<std::string> imNames;
  imNames.push_back(IMAGE1);
  imNames.push_back(IMAGE2);
  ImageBuffer imBufs[2];
  getImages(imNames, imBufs);
  
  ImageBuffer output(imBufs[0].width(), imBufs[0].height(),
                     CurrentImBufValueType(),
                     imBufs[0].xAlign(), imBufs[0].yAlign());
  
  cl::Event event;
  
  cl::Kernel kernel = GetKernel(kernelName.c_str());
  if (kernelName == "filter") {
    testFilter(kernel, imBufs[0], output, event);
  } else {
    testSimpleOp(kernel, imBufs[0], imBufs[1], output, event);
  }
  
//  long startTime, endTime;
//  startTime = event.getProfilingInfo<CL_PROFILING_COMMAND_START>();
//  endTime = event.getProfilingInfo<CL_PROFILING_COMMAND_END>();
//  
//  std::cout << "Done in " << (endTime - startTime)/1000000.f << " "
//            << "milliseconds.\n" << std::endl;
  
  ImageData outputData = output.fetchData();
  
  WriteJpeg(OUTPUT "profile-output.jpg", outputData, true);
  WriteCSV(OUTPUT "profile-output.csv", outputData);
}

int main(i32, char *const []) {
  try {
    THING_TO_DO;
  }
  catch (const cl::Error& err) {
    std::cerr << "ERROR: " << err.what() << "(" << err.err() << ")"
              << std::endl;
  }
  catch (const std::exception& exc) {
    std::cerr << "ERROR: " << exc.what() << std::endl;
  }
  
#ifdef _WIN32
	getchar();
#endif
}
