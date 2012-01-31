#include <string>
#include <vector>

#define CLIP_ENABLE_PROFILING 1
#define CLIP_DEFAULT_PROGRAM_SETTINGS \
  ProgramSettings(4, Float32, Float32, Texture)
#define TEST_DEVICE_TYPE GPU

#include <clip.hpp>
#include <evp.hpp>
#include <evp/io.hpp>
#include <evp/io/imageio.hpp>
#include <evp/util/tictoc.hpp>

#define NUM_THETAS 8
#define NUM_CURVS 1

//#define IMAGE1 "raven.jpg"
//#define IMAGE2 "camera.jpg"

#define IMAGE1 "raven.jpg"
#define IMAGE2 "trans-small.png"

#define OUTPUT "output/"
#define IMAGES "images/"

//#define THING_TO_DO profileOp()
//#define THING_TO_DO testJitterOps()
//#define THING_TO_DO testGradientOps()
//#define THING_TO_DO testGabor()
#define THING_TO_DO testFlowModel()
//#define THING_TO_DO testFlowCompatibilities()
//#define THING_TO_DO testCurveCompatibilities()

using namespace std;
using namespace clip;
using namespace evp;

inline void getImages(const vector<string> &imNames,
                      ImageBuffer* imBufs) {
  vector<string>::const_iterator it;
  for (it = imNames.begin(); it != imNames.end(); ++it, ++imBufs) {
    ImageData image;
    ReadImage(IMAGES + *it, image);
    
    *imBufs = ImageBuffer(image);
  }
}

inline vector<ImageBuffer> getImages() {
  vector<string> imNames;
  imNames.push_back(IMAGE1);
  imNames.push_back(IMAGE2);
  
  vector<ImageBuffer> imBufs(2);
  getImages(imNames, &imBufs[0]);
  return imBufs;
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
  
  stringstream sstream;
  sstream << OUTPUT "curve-test-compatibility.csv";
  WriteCSV(sstream.str(), reduction);
}

inline void testFlowModel() {
  int size = 9;
  f64 scale = 1;
  
  FlowModel model(0, 0, 5*M_PI/8, 0.2f, 0.2f);
  ofstream out(OUTPUT "flow-test-model.pdf", ios::out);
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
//  params.flowSupport = CreateUniformInhibitionFlowSupportOp;
  params.flowSupport = CreateInhibitionlessFlowSupportOp;
  params.numCurvatures = 5;
  params.subsamples = 1;
  
  for (i32 ti = 0; ti < params.numOrientations; ti++) {
    float t = ti*params.orientationStep;
    for (i32 kti = 0; kti < params.numCurvatures; kti++) {
      float kt = (kti - params.numCurvatures/2)*params.curvatureStep;
      for (i32 kni = 0; kni < params.numCurvatures; kni++) {
        float kn = (kni - params.numCurvatures/2)*params.curvatureStep;
        
        FlowSupportOpPtr connectionsOp(params.flowSupport(t, kt, kn, params));
        const NDArray<ImageData,3> &kernels = connectionsOp->kernels();
        
        stringstream ss;
        ss << "output/compat-"
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

void testJitterOps() {
  ClipInit(TEST_DEVICE_TYPE);
  
  vector<ImageBuffer> imBufs = getImages();
  
  FlowInitOpParams params(NUM_THETAS, NUM_CURVS);
  JitteredFlowInitOps initOps(params);
  initOps.addProgressListener(TextualProgressMonitor);
  tic();
  FlowBuffersPtr flow = initOps.apply(imBufs[0]);
  cout << "Done in " << toc()/1000 << " milliseconds." << endl;
  FlowDataPtr flowData = ReadImageDataFromBufferArray(*flow);
  
  WriteFlowToPDF(OUTPUT "gradient-out.pdf", *flowData);
}

void testGradientOps() {
  ClipInit(TEST_DEVICE_TYPE);
  
  vector<ImageBuffer> imBufs = getImages();
  
  GradientOp grad;
  ImageBuffer outXs = ~imBufs[0], outYs = ~imBufs[0];
  grad(imBufs[0], outXs, outYs);
  WriteJpeg(OUTPUT "out-xs.jpg", outXs.fetchData(), true);
  WriteJpeg(OUTPUT "out-ys.jpg", outYs.fetchData(), true);
  
//  FlowInitOpParams params(NUM_THETAS, NUM_CURVS);
//  GradientFlowInitOps initOps(params);
//  initOps.addProgressListener(TextualProgressMonitor);
//  tic();
//  FlowBuffersPtr flow = initOps.apply(imBufs[0]);
//  cout << "Done in " << toc()/1000 << " milliseconds." << endl;
//  FlowDataPtr flowData = ReadImageDataFromBufferArray(*flow);
//  WriteFlowToPDF(OUTPUT "flow-output.pdf", *flowData);
}

void testGabor() {
  f32 baseWavelength = 12;
  ImageData gabor = MakeGabor(0, baseWavelength, 0, baseWavelength, 1.5);
  WriteJpeg(OUTPUT "gabor.jpg", gabor, true);
}

void profileOp() {
  ClipInit(TEST_DEVICE_TYPE);
  
  vector<ImageBuffer> imBufs = getImages();
  
  srand(time(NULL));
  f32 off1 = f32(rand())/RAND_MAX - 0.5f;
  f32 off2 = 2*(f32(rand())/RAND_MAX - 0.5f);
  ImageData kernel = MakeGabor(M_PI/3 + off1*M_PI/8, 3, 0, 4 + off2, 1);
  ImageBuffer output = Filter(imBufs[0], kernel);

//  LLAnd(imBufs[0], imBufs[1], 16, false, 1.f, output);
  
  cout << "Done in " << LastEventTiming()/1000000.f << " "
       << "milliseconds.\n" << endl;
  
  ImageData outputData = output.fetchData();
  
  WriteJpeg(OUTPUT "rotglass-out.jpg", outputData, true);
}

int main(i32, char *const []) {
  try {
    THING_TO_DO;
  }
  catch (const cl::Error& err) {
    cerr << "ERROR: " << err.what() << "(" << err.err() << ")"
              << endl;
  }
  catch (const exception& exc) {
    cerr << "ERROR: " << exc.what() << endl;
  }
  
#ifdef _WIN32
	getchar();
#endif
}
