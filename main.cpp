#include <array>
#include <atomic>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstdio>
#include <exception>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <mutex>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <SFML/Window/WindowStyle.hpp>

#include <RtypesCore.h>
#include <TAxis.h>
#include <TF1.h>
#include <TFile.h>
#include <TGraph.h>
#include <TH1.h>
#include <TLine.h>
#include <TList.h>
#include <TMultiGraph.h>
#include <TObject.h>

#include "DataProcessing/SimDataPipeline.hpp"
#include "Graphics/Camera.hpp"
#include "Graphics/RenderStyle.hpp"
#include "PhysicsEngine/GSVector.hpp"
#include "PhysicsEngine/Gas.hpp"
#include "PhysicsEngine/Particle.hpp"
#include "gasSim/Libs/INIReader.h"
#include "gasSim/Libs/cxxopts.hpp"

std::atomic<double> GS::Particle::radius{1.};
std::atomic<double> GS::Particle::mass{1.};

GS::GSVectorD stovec(std::string s) {
	if (s.size() == 0) {
		throw std::invalid_argument("Cannot provide empty string.");
	}
  // checking for braces and erasing the first one
  if (s.front() != '{' || s.back() != '}') {
    throw std::invalid_argument("Missing opening and closing braces.");
  }
  s.erase(s.begin());

  std::string x1{}, x2{}, x3{};
  enum coordinate { x, y, z, done };
  coordinate workingC{x};
  // removing spaces
  for (char c : s) {
    if (workingC == done) {
      return {std::stod(x1), std::stod(x2), std::stod(x3)};
    }
    if (c == ',') {
      switch (workingC) {
        case x:
          workingC = y;
          break;
        case y:
          workingC = z;
          break;
        case z:
        case done:
          throw std::invalid_argument("Too many commas.");
          break;
      }
    } else if (c == '}') {
      workingC = done;
    } else {
      switch (workingC) {
        case x:
          x1.push_back(c);
          break;
        case y:
          x2.push_back(c);
          break;
        case z:
          x3.push_back(c);
          break;
        case done:
          break;
      }
    }
  }
  return {std::stod(x1), std::stod(x2), std::stod(x3)};
}

GS::VideoOpts stovideoopts(std::string s) {
  if (s == "all") {
    return GS::VideoOpts::all;
  } else if (s == "gasPlusCoords") {
    return GS::VideoOpts::gasPlusCoords;
  } else if (s == "justStats") {
    return GS::VideoOpts::justStats;
  } else if (s == "justGas") {
    return GS::VideoOpts::justGas;
  } else {
    throw std::invalid_argument(
        "String not corresponding to available videoOpt.");
  }
}

void throwIfZombie(TObject* o, std::string message) {
  if (o->IsZombie()) {
    throw std::runtime_error(message);
  }
}

int main(int argc, const char* argv[]) {
  try {
    std::cout << "Welcome. Starting idealGasSim gas simulation.\n";

    // trying to make root stop throwing temper tantrums over my variables
    TH1D::AddDirectory(kFALSE);

    cxxopts::Options options(
        "idealGasSim",
        "An event-based ideal gas simulator with graphic data visualization");

    options.add_options()("h,help", "Print this help message")(
        "c,config",
        "Use the given path as the configuration file, defaults to "
        "configs/gasSim_demo.ini",
        cxxopts::value<std::string>());

    auto opts = options.parse(argc, argv);

    // No args or explicit help → print help and exit
    if (argc == 1 || opts["help"].as<bool>()) {
      std::cout << options.help() << '\n';
      return 0;
    }

    std::string path = opts.count("config") != 0
                           ? opts["config"].as<std::string>()
                           : "configs/gasSim_demo.ini";

    INIReader config(path);
    if (config.ParseError() != 0) {
      throw std::runtime_error("Failed to load " + path);
    }

    std::cout << "Starting resources loading. Using config file at path "
              << path << std::endl;

    double pMass{config.GetReal("simulation parameters", "pMass", 1.)};
		if (pMass <= 0) {
			throw std::invalid_argument("Found non-positive mass in config file.");
		}
    GS::Particle::setMass(pMass);
    double pRadius{config.GetReal("simulation parameters", "pRadius", 1.)};
		if (pRadius <= 0) {
			throw std::invalid_argument("Found non-positive radius in config file.");
		}
    GS::Particle::setRadius(pRadius);

    std::string ROOTInputPath{"inputs/"};
    ROOTInputPath +=
        config.Get("simulation parameters", "ROOTInputPath", "input").c_str();
    ROOTInputPath += ".root";
    TFile inputFile{TFile(ROOTInputPath.c_str())};
    throwIfZombie(&inputFile,
                  "Failed to load provided input file path: " + ROOTInputPath);

    std::shared_ptr<TH1D> speedsHTemplate{
        dynamic_cast<TH1D*>(inputFile.Get("speedsHTemplate"))};
    speedsHTemplate->SetDirectory(nullptr);
    throwIfZombie(speedsHTemplate.get(),
                  "Failed to load speedsHTemplate from input file.");

    long nStats{config.GetInteger("simulation parameters", "nStats", 1)};
    if (nStats <= 0) {
      throw std::invalid_argument("Provided negative nStats.");
    }

    float framerate{config.GetFloat("output", "framerate", 60.f)};

    GS::SimDataPipeline output{
        static_cast<unsigned>(nStats > 0 ? nStats
                                         : throw std::invalid_argument(
                                               "Provided negative nStats.")),
        framerate, *speedsHTemplate};
    sf::Font font;
    font.loadFromFile("assets/JetBrains-Mono-Nerd-Font-Complete.ttf");
    output.setFont(font);
    size_t nParticles{static_cast<size_t>(
        config.GetInteger("simulation parameters", "nParticles", 1))};
		if (nParticles > LONG_MAX) {
			throw(std::invalid_argument("Found non-positive radius in config file."));
		}
    double targetT{config.GetReal("simulation parameters", "targetT", 1)};
		if (targetT <= 0) {
			throw std::invalid_argument("Found non-positive target temperature in config file.");
		}
    double boxSide{config.GetReal("simulation parameters", "boxSide", 1.)};
		if (boxSide <= 0) {
			throw std::invalid_argument("Found non-positive box side in config file.");
		}
    GS::Gas gas{nParticles, targetT, boxSide};

    double targetBufferTime{config.GetReal("output", "targetBuffer", 2.5)};
		if (targetBufferTime <= 0) {
			throw std::invalid_argument("Found non-positive target buffer time in config file.");
		}

    double desiredStatChunkSize{
        targetBufferTime * M_PI *
        std::pow(GS::Particle::getRadius() / gas.getBoxSide(), 2.) *
        std::accumulate(gas.getParticles().begin(), gas.getParticles().end(),
                        0.,
                        [](double acc, const GS::Particle& p) {
                          return acc + p.speed.norm();
                        }) /
        static_cast<double>(nStats) / gas.getBoxSide()};
		if (desiredStatChunkSize > SIZE_MAX) {
			throw std::runtime_error("Computed desired stat chunk size is too big. Check your config file.");
		}

    output.setStatChunkSize(static_cast<size_t>(
        desiredStatChunkSize > 1 ? desiredStatChunkSize : 1));

    std::atomic<bool> stop{false};
    std::mutex coutMtx;
    size_t nIters{static_cast<size_t>(
        config.GetInteger("simulation parameters", "nIters", 0))};
		if (nIters > LONG_MAX) {
			throw std::runtime_error("Found negative iterations number in config file.");
		}
    std::thread simThread{[&, nIters] {
      try {
        gas.simulate(nIters, output, [&] { return stop.load(); });
      } catch (std::runtime_error const& e) {
        std::lock_guard<std::mutex> coutGuard{coutMtx};
        std::cout << "Runtime error: " << e.what() << std::endl;
        std::terminate();
      }
      std::lock_guard<std::mutex> coutGuard{coutMtx};
      std::cout
          << "Simulation thread done.                                      \r"
          << std::endl;
    }};

    {
      std::lock_guard<std::mutex> coutGuard{coutMtx};
      std::cout << "Simulation thread running." << std::endl;
    }

    GS::GSVectorF camPos{static_cast<GS::GSVectorF>(stovec(config.Get(
        "rendering", "camPos",
        (std::ostringstream() << "{" << boxSide * 1.5 << ", " << boxSide * 1.25
                              << ", " << boxSide * 0.75 << '}')
            .str())))};
    GS::GSVectorF camSight{static_cast<GS::GSVectorF>(stovec(config.Get(
        "rendering", "camSight",
        (std::ostringstream()
         << "{" << boxSide * 0.5 - camPos.x << ", " << boxSide * 0.5 - camPos.y
         << ", " << boxSide * 0.5 - camPos.z << '}')
            .str())))};

    sf::Vector2u windowSize{
        static_cast<unsigned>(config.GetInteger("output", "videoResX", 800)),
        static_cast<unsigned>(config.GetInteger("output", "videoResY", 600))};
		if (windowSize.x > INT_MAX || windowSize.y > INT_MAX) {
			throw std::invalid_argument("Found negative videoResX or videoResY in config file.");
		}

    GS::VideoOpts videoOpt{
        stovideoopts(config.Get("output", "videoOpt", "justGas"))};
    sf::Vector2u gasSize{1, 1};
    switch (videoOpt) {
      case GS::VideoOpts::justGas:
        gasSize = windowSize;
        break;
      case GS::VideoOpts::justStats:
        break;
      case GS::VideoOpts::gasPlusCoords:
        gasSize = {static_cast<unsigned>(windowSize.x * 0.5),
                   static_cast<unsigned>(windowSize.y * 8. / 9.)};
        break;
      case GS::VideoOpts::all:
        gasSize = {static_cast<unsigned>(windowSize.x * 0.5),
                   static_cast<unsigned>(windowSize.y * 0.9)};
        break;
    }

    GS::Camera camera{camPos,
                      camSight,
                      config.GetFloat("render", "camLength", 1.f),
                      config.GetFloat("render", "fov", 90.f),
                      gasSize.x,
                      gasSize.y};

    sf::Texture particleTex;
    std::string particleTexPath{"assets/"};
    particleTexPath += config.Get("render", "particleTexPath", "lightBall");
    particleTexPath += ".png";
		if (!particleTex.loadFromFile(particleTexPath)) {
			throw std::invalid_argument("Failed to load particle texture from config file path.");
		}
    GS::RenderStyle style{particleTex};
    style.setWallsColor(sf::Color(static_cast<unsigned>(
        config.GetInteger("render", "wallsColor", 0x50fa7b80))));
    style.setBGColor(sf::Color(static_cast<unsigned>(
        config.GetInteger("render", "gasBgColor", 0xffffffff))));
    style.setWallsOpts(config.Get("render", "wallsOpts", "ufdl"));

    bool mfpMemory{config.GetBoolean("output", "mfpMemory", true)};

    const int frameTimems{static_cast<int>(1000. / framerate)};

    std::thread processThread;
    if (videoOpt != GS::VideoOpts::justStats) {
      // process stats and graphics
      processThread = std::thread([&, mfpMemory] {
        output.processData(camera, style, mfpMemory,
                           [&] { return stop.load(); });
        std::lock_guard<std::mutex> coutGuard{coutMtx};
        std::cout << "Data processing thread done.                         "
                     "               \r"
                  << std::endl;
      });
    } else {
      // process only stats
      processThread = std::thread([&, mfpMemory] {
        try {
          output.processData(mfpMemory, [&] { return stop.load(); });
        } catch (std::runtime_error const& e) {
          std::lock_guard<std::mutex> coutGuard{coutMtx};
          std::cout << "Runtime error: " << e.what() << std::endl;
          std::terminate();
        }
        std::lock_guard<std::mutex> coutGuard{coutMtx};
        std::cout << "Data processing thread done.                         "
                     "               \r"
                  << std::endl;
      });
    }

    {
      std::lock_guard<std::mutex> coutGuard{coutMtx};
      std::cout << "Processing thread running." << std::endl;
    }

    auto graphsList = std::make_unique<TList>();
    graphsList->SetOwner(kFALSE);
    TMultiGraph* pGraphs{dynamic_cast<TMultiGraph*>(inputFile.Get("pGraphs"))};
    throwIfZombie(pGraphs, "Failed to load pressure graphs multigraph.");
    TGraph* kBGraph{dynamic_cast<TGraph*>(inputFile.Get("kBGraph"))};
    throwIfZombie(kBGraph, "Failed to load pressure graphs multigraph.");
    TGraph* mfpGraph{dynamic_cast<TGraph*>(inputFile.Get("mfpGraph"))};
    throwIfZombie(mfpGraph, "Failed to load pressure graphs multigraph.");
    graphsList->Add(pGraphs);
    graphsList->Add(kBGraph);
    graphsList->Add(mfpGraph);

    std::shared_ptr<TF1> pLineF{dynamic_cast<TF1*>(inputFile.Get("pLineF"))};
    throwIfZombie(pLineF.get(), "Failed to load pLineF from input file.");
    std::shared_ptr<TF1> kBGraphF{
        dynamic_cast<TF1*>(inputFile.Get("kBGraphF"))};
    throwIfZombie(kBGraphF.get(), "Failed to load kBGraphF from input file.");
    std::shared_ptr<TF1> maxwellF{
        dynamic_cast<TF1*>(inputFile.Get("maxwellF"))};
    throwIfZombie(maxwellF.get(), "Failed to load maxwellF from input file.");
    std::shared_ptr<TF1> mfpGraphF{
        dynamic_cast<TF1*>(inputFile.Get("mfpGraphF"))};
    throwIfZombie(mfpGraphF.get(), "Failed to load mfpGraphF from input file.");
    std::shared_ptr<TH1D> cumulatedSpeedsH{
        dynamic_cast<TH1D*>(inputFile.Get("cumulatedSpeedsH"))};
    cumulatedSpeedsH->SetDirectory(nullptr);
    throwIfZombie(cumulatedSpeedsH.get(),
                  "Failed to load cumulatedSpeedsH from input file.");
    std::shared_ptr<TLine> meanLine{
        dynamic_cast<TLine*>(inputFile.Get("meanLine"))};
    throwIfZombie(meanLine.get(), "Failed to load meanLine from input file.");
    std::shared_ptr<TLine> meanSqLine{
        dynamic_cast<TLine*>(inputFile.Get("meanSqLine"))};
    throwIfZombie(meanSqLine.get(),
                  "Failed to load meanSqLine from input file.");
    std::shared_ptr<TF1> expP{dynamic_cast<TF1*>(inputFile.Get("expP"))};
    throwIfZombie(expP.get(), "Failed to load expP from input file.");
    std::shared_ptr<TF1> expkB{dynamic_cast<TF1*>(inputFile.Get("expkB"))};
    throwIfZombie(expkB.get(), "Failed to load expkB from input file.");
    std::shared_ptr<TF1> expMFP{dynamic_cast<TF1*>(inputFile.Get("expMFP"))};
    throwIfZombie(expMFP.get(), "Failed to load expMFP from input file.");

    maxwellF->SetParameter(0, targetT);
    maxwellF->FixParameter(
        1, static_cast<double>(nParticles * output.getStatSize()));
    maxwellF->FixParameter(2, pMass);

    std::function<void(TH1D&, GS::VideoOpts)> fitLambda{[&](TH1D& speedsH,
                                                            GS::VideoOpts opt) {
      TGraph* genPGraph{
          dynamic_cast<TGraph*>(pGraphs->GetListOfGraphs()->At(6))};
      if (genPGraph->GetN()) {
        genPGraph->Fit(pLineF.get(), "Q");
        // Keep number of drawn points at 30
        if (genPGraph->GetN() >= 30) {
          pGraphs->GetXaxis()->SetRangeUser(
              genPGraph->GetPointX(genPGraph->GetN() - 30),
              genPGraph->GetPointX(genPGraph->GetN() - 1));
        }
      }
      if (kBGraph->GetN()) {
        if (kBGraph->GetN() >= 30) {
          kBGraph->GetXaxis()->SetRangeUser(
              kBGraph->GetPointX(kBGraph->GetN() - 30),
              kBGraph->GetPointX(kBGraph->GetN() - 1));
        }
        kBGraph->Fit(kBGraphF.get(), "Q");
        // keep most data visible but not squished down by outliers
        kBGraph->GetYaxis()->SetRangeUser(0., kBGraphF->GetParameter(0) * 3.25);
      }
      if (opt != GS::VideoOpts::gasPlusCoords) {
        if (static_cast<bool>(speedsH.GetEntries())) {
          speedsH.Fit(maxwellF.get(), "Q");
          // skibidi
          cumulatedSpeedsH->Add(&speedsH);
          meanLine->SetX1(speedsH.GetMean());
          meanLine->SetY1(0.);
          meanLine->SetX2(meanLine->GetX1());
          meanLine->SetY2(maxwellF->Eval(meanLine->GetX1()));
          double rms{speedsH.GetRMS()};
          meanSqLine->SetX1(
              std::sqrt(rms * rms + meanLine->GetX1() * meanLine->GetX1()));
          meanSqLine->SetY1(0.);
          meanSqLine->SetX2(meanSqLine->GetX1());
          meanSqLine->SetY2(maxwellF->Eval(meanSqLine->GetX1()));
        }
        if (mfpGraph->GetN()) {
          if (mfpGraph->GetN() >= 30) {
            mfpGraph->GetXaxis()->SetRangeUser(
                mfpGraph->GetPointX(mfpGraph->GetN() - 30),
                mfpGraph->GetPointX(mfpGraph->GetN() - 1));
          }
          mfpGraph->Fit(mfpGraphF.get(), "Q");
        }
      }
    }};

    expP->SetParameter(0, targetT * static_cast<double>(nParticles) /
                              static_cast<double>(std::pow(boxSide, 3.)));
    expkB->SetParameter(0, 1);
    expMFP->SetParameter(0, targetT / M_SQRT2 / expP->GetParameter(0) /
                                (M_PI * pRadius * pRadius));

    std::array<std::function<void()>, 4> drawLambdas{
        [&]() {
          TGraph* genPGraph{
              dynamic_cast<TGraph*>(pGraphs->GetListOfGraphs()->At(0))};
          if (genPGraph->GetN()) {
            pLineF->GetXaxis()->SetRangeUser(
                0., genPGraph->GetPointX(genPGraph->GetN() - 1));
          }
          pLineF->Draw("SAME");
          expP->SetRange(0., genPGraph->GetXaxis()->GetXmax());
          expP->Draw("SAME");
        },
        [&]() {
          kBGraphF->Draw("SAME");
          expkB->SetRange(0., kBGraph->GetXaxis()->GetXmax());
          expkB->Draw("SAME");
        },
        [&]() {
          maxwellF->Draw("SAME");
          meanLine->Draw("SAME");
          meanSqLine->Draw("SAME");
        },
        [&]() {
          mfpGraphF->Draw("SAME");
          expMFP->SetRange(0., mfpGraph->GetXaxis()->GetXmax());
          expMFP->Draw("SAME");
        }};

    sf::Texture placeHolder;
    if (!placeHolder.loadFromFile(
        (std::ostringstream()
         << "assets/" << config.Get("render", "placeHolderName", "placeholder")
         << ".png")
            .str()
            .c_str())) {
			throw std::invalid_argument("Failed to load placeHolder texture from the path in the config file.");
		}

    while (!output.isProcessing()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    {
      std::lock_guard<std::mutex> coutGuard{coutMtx};
      std::cout << "All resources loaded. Starting video output processing."
                << std::endl;
    }

    if (config.GetBoolean("output", "saveVideo", false)) {
      {
        std::lock_guard<std::mutex> coutGuard{coutMtx};
        std::cout << "Starting video encoding." << std::endl;
      }
      std::ostringstream cmd{};
      cmd << "ffmpeg -y -f rawvideo -pixel_format rgba -video_size "
          << windowSize.x << "x" << windowSize.y << " -framerate "
          << output.getFramerate() << " -i - -c:v libx264 -pix_fmt yuv420p "
          << "outputs/videos/"
          << config.Get("output", "videoOutputName", "output") << ".mp4";

      FILE* ffmpeg = popen(cmd.str().c_str(), "w");
      if (!ffmpeg) {
        throw std::runtime_error("Failed to open ffmpeg pipe.");
      }
      bool lastBatch{false};
      sf::Image auxImg;
      auxImg.create(windowSize.x, windowSize.y);
      while (true) {
        if (output.isDone() && output.getRawDataSize() < output.getStatSize() &&
            !output.isProcessing()) {
          lastBatch = true;
        }
        std::vector<sf::Texture> frames = {
            output.getVideo(videoOpt, {windowSize.x, windowSize.y}, placeHolder,
                            *graphsList, true, fitLambda, drawLambdas)};
        int i{0};
        for (sf::Texture& t : frames) {
          auxImg = t.copyToImage();
          fwrite(auxImg.getPixelsPtr(), 1, windowSize.x * windowSize.y * 4,
                 ffmpeg);
          ++i;
          std::lock_guard<std::mutex> coutGuard{coutMtx};
          std::cout << "Encoding batch of " << frames.size() << " frames: "
                    << static_cast<float>(i) / static_cast<float>(frames.size())
                    << "% complete.                 \r";
          std::cout.flush();
        }
        if (lastBatch) {
          std::cout << "Encoding done!" << std::endl;
          break;
        }
      }
      pclose(ffmpeg);
    } else {  // no permanent video output requested ->
              // wiew-once live video
      {
        std::lock_guard<std::mutex> coutGuard{coutMtx};
        std::cout << "Starting video display." << std::endl;
      }
      sf::RenderWindow window(sf::VideoMode(windowSize.x, windowSize.y),
                              "GasSim Display", sf::Style::Default);
      sf::Texture lastWindowTxtr;
      lastWindowTxtr.create(windowSize.x, windowSize.y);
      {
        sf::Image blankWindow;
        blankWindow.create(windowSize.x, windowSize.y, sf::Color::White);
        lastWindowTxtr.update(blankWindow);
      }
      std::mutex windowMtx;  // used both for lastWindowTxtr and window
      window.setFramerateLimit(60);
      window.setActive(false);
      std::atomic<bool> pauseBufferLoop{false};
      std::atomic<bool> bufferKillSignal{false};
      std::atomic<int> queueNumber{1};
      std::atomic<int> launchedPlayThreadsN{0};
      std::atomic<int> droppedFrames{0};
      std::atomic<int> framesToDrop{0};
      std::atomic<int> processedFrames{0};

      auto playLambda{
          [&, frameTimems](std::shared_ptr<std::vector<sf::Texture>> rPtr,
                           int threadN) {
            sf::Sprite auxS;
            sf::Event e;
            while (threadN != queueNumber) {
              if (stop.load()) {
                break;
              }
              std::this_thread::sleep_for(
                  std::chrono::milliseconds(frameTimems));
            }
            if (!stop.load()) {
              pauseBufferLoop.store(true);
              std::lock_guard<std::mutex> windowGuard{windowMtx};
              if (window.isOpen()) {
                window.setActive();
                auto lastDrawEnd{std::chrono::high_resolution_clock::now()};
                float frameTimeS{static_cast<float>(frameTimems / 1000.)};
                std::chrono::duration<float> lastFrameDrawTime{};
                int i{0};
                for (const sf::Texture& r : *rPtr) {
                  while (window.pollEvent(e)) {
                    if (e.type == sf::Event::Closed) {
                      stop.store(true);
                      bufferKillSignal.store(true);
                      pauseBufferLoop.store(true);
                      window.close();
                      break;
                    }
                  }
                  if (window.isOpen()) {
                    if (!framesToDrop.load()) {
                      lastFrameDrawTime =
                          std::chrono::high_resolution_clock::now() -
                          lastDrawEnd;
                      if (lastFrameDrawTime.count() <= frameTimeS) {
                        auxS.setTexture(r);
                        window.draw(auxS);
                        window.display();
                      } else {
                        framesToDrop.fetch_add(
                            static_cast<int>(lastFrameDrawTime.count() /
                                             static_cast<float>(frameTimems)) -
                            1);
                        droppedFrames.fetch_add(1);
                      }
                      lastDrawEnd = std::chrono::high_resolution_clock::now();
                    } else {
                      framesToDrop.fetch_sub(1);
                      lastDrawEnd = std::chrono::high_resolution_clock::now();
                    }
                    processedFrames.fetch_sub(1);
                  } else {
                    break;
                  }
                  std::lock_guard<std::mutex> coutGuard{coutMtx};
                  std::cout << "Status: displaying. Progress: " << ++i
                            << " from batch of " << rPtr->size()
                            << " renders      \r";
                  std::cout.flush();
                }
                window.setActive(false);
                queueNumber++;
                if (launchedPlayThreadsN == threadN) {
                  if (rPtr->size()) {
                    lastWindowTxtr.update(std::move(rPtr->back()));
                  }
                  pauseBufferLoop.store(false);
                  std::lock_guard<std::mutex> coutGuard{coutMtx};
                  std::cout
                      << "Status: buffering.                                   "
                         "                          \r";
                }
              }
            }
          }};
      sf::Texture bufferingWheelT;
      if (bufferingWheelT.loadFromFile(
          std::string("assets/") +
          config.Get("output", "bufferingWheel", "jesse") +
          std::string(".png"))) {
				throw std::invalid_argument("Failed to load buffering wheel texture from the path in the config file.");
			}
      sf::Sprite bufferingWheel;
      bufferingWheel.setTexture(bufferingWheelT, true);
      bufferingWheel.setOrigin(
          static_cast<float>(bufferingWheelT.getSize().x) / 2.f,
          static_cast<float>(bufferingWheelT.getSize().y) / 2.f);
      bufferingWheel.setScale(
          static_cast<float>(windowSize.x) * 0.2f /
              static_cast<float>(bufferingWheelT.getSize().x),
          static_cast<float>(windowSize.y) * 0.2f /
              static_cast<float>(bufferingWheelT.getSize().y));
      bufferingWheel.setPosition(static_cast<float>(windowSize.x) / 2.f,
                                 static_cast<float>(windowSize.y) / 2.f);
      sf::Text bufferingText{"(UwU) loading (^w^)", font,
                             static_cast<unsigned>(windowSize.y * 0.025)};
      sf::Text progressText{
          "0 iterations awaiting processing\n0 stats instances,\n0 renders "
          "awaiting composition\n0 frames ready to visualize",
          font, static_cast<unsigned>(windowSize.y * 0.01)};
      bufferingText.setFillColor(sf::Color::Black);
      progressText.setFillColor(sf::Color::Black);
      bufferingText.setOutlineColor(sf::Color::White);
      progressText.setOutlineColor(sf::Color::White);
      bufferingText.setOutlineThickness(4);
      progressText.setOutlineThickness(2);
      bufferingText.setOrigin(bufferingText.getLocalBounds().width / 2.f,
                              bufferingText.getLocalBounds().height / 2.f);
      progressText.setOrigin(0., 0.);
      bufferingText.setPosition(
          static_cast<float>(bufferingWheel.getPosition().x),
          bufferingWheel.getPosition().y +
              static_cast<float>(windowSize.y) * 0.2f);
      progressText.setPosition(static_cast<float>(windowSize.y) * .05f,
                               static_cast<float>(windowSize.y) * .05f);
      std::thread bufferingLoop{[&, frameTimems]() {
        sf::Sprite auxS;
        sf::Event e;
        while (!bufferKillSignal.load()) {
          if (!pauseBufferLoop) {
            std::lock_guard<std::mutex> windowGuard{windowMtx};
            auxS.setTexture(lastWindowTxtr, true);
            while (window.isOpen() && !pauseBufferLoop.load() &&
                   !bufferKillSignal.load()) {
              while (window.pollEvent(e)) {
                if (e.type == sf::Event::Closed) {
                  stop.store(true);
                  bufferKillSignal.store(true);
                  pauseBufferLoop.store(true);
                  window.close();
                  break;
                }
              }
              // avoid drawing an extra frame on window close
              if (!window.isOpen()) {
                break;
              }
              window.setActive();
              window.draw(auxS);  // repaint last window texture
                                  // 120 degrees per second
              bufferingWheel.setRotation(
                  bufferingWheel.getRotation() +
                  120.f * static_cast<float>(frameTimems) / 1000.f);
              window.draw(bufferingWheel);
              window.draw(bufferingText);
              progressText.setString(
                  std::to_string(output.getRawDataSize()) +
                  " iterations awaiting processing\n" +
                  std::to_string(output.getNStats()) + " stats instances,\n" +
                  std::to_string(output.getNRenders()) +
                  " renders awaiting composition\n" +
                  std::to_string(processedFrames.load()) + " processed frames");
              window.draw(progressText);
              window.display();
            }
            window.setActive(false);
            if (!window.isOpen()) {
              break;
            }
            if (bufferKillSignal.load()) {
              break;
            }
          } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(frameTimems));
          }
        }
      }};
      bool lastBatch{false};
      // main thread composes video and sends batches through playthreads
      while (!stop.load()) {
        std::shared_ptr<std::vector<sf::Texture>> rendersPtr{
            std::make_shared<std::vector<sf::Texture>>()};
        rendersPtr->reserve(static_cast<size_t>(output.getFramerate()) * 10);
        while (static_cast<double>(rendersPtr->size()) <=
                   output.getFramerate() * targetBufferTime &&
               !stop.load()) {
          if (output.isDone() &&
              output.getRawDataSize() < output.getStatSize() &&
              !output.isProcessing()) {
            lastBatch = true;
          }
          std::vector<sf::Texture> v = {output.getVideo(
              videoOpt, {windowSize.x, windowSize.y}, placeHolder, *graphsList,
              true, fitLambda, drawLambdas)};
          processedFrames.fetch_add(static_cast<int>(v.size()));
          rendersPtr->insert(rendersPtr->end(),
                             std::make_move_iterator(v.begin()),
                             std::make_move_iterator(v.end()));
          if (lastBatch) {
            break;
          }
        }
        if (!lastBatch) {
          std::thread playThread{
              [playLambda, rendersPtr, &launchedPlayThreadsN] {
                launchedPlayThreadsN.fetch_add(1);
                playLambda(rendersPtr, launchedPlayThreadsN);
              }};
          if (!stop.load()) {
            playThread.detach();
          } else {
            playThread.join();
            bufferKillSignal.store(true);
            {
              std::lock_guard<std::mutex> coutGuard{coutMtx};
              std::cout
                  << "Window close detected through stop signal. Aborting."
                  << std::endl;
            }
            break;
          }
        } else {
          std::thread playThread{
              [playLambda, rendersPtr, &launchedPlayThreadsN] {
                launchedPlayThreadsN.fetch_add(1);
                playLambda(rendersPtr, launchedPlayThreadsN.load());
              }};
          playThread.join();
          bufferKillSignal.store(true);
          {
            std::lock_guard<std::mutex> coutGuard{coutMtx};
            std::cout << "Done displaying. Closing window." << std::endl;
          }
          break;
        }
      }  // while (!stop.load())
      pauseBufferLoop.store(true);
      // likely redundant, but one can never be too sure
      bufferKillSignal.store(true);
      if (bufferingLoop.joinable()) {
        bufferingLoop.join();
      }
      if (window.isOpen()) {
        window.close();
      }
      std::lock_guard<std::mutex> coutGuard{coutMtx};
      std::cout << "Dropped frames: " << droppedFrames.load()
                << ". Leftover data: " << output.getRawDataSize()
                << " collisions." << std::endl;
    }

    if (simThread.joinable()) {
      simThread.join();
    }
    if (processThread.joinable()) {
      processThread.join();
    }

    if (!stop.load()) {
      std::cout << "Saving results to file... ";
      std::cout.flush();
      inputFile.Close();
      auto rootOutput = std::make_unique<TFile>(
          (std::string("outputs/") +
           config.Get("output", "rootOutputName", "output") + ".root")
              .c_str(),
          "RECREATE");
			throwIfZombie(rootOutput.get(), "Failed to load/make root output file with path from config file.");

      rootOutput->SetTitle(rootOutput->GetName());
      rootOutput->cd();
      graphsList->Write();
      pLineF->Write();
      kBGraph->Write();
      kBGraphF->Write();
      mfpGraph->Write();
      mfpGraphF->Write();
      maxwellF->Write();
      cumulatedSpeedsH->Write();
      rootOutput->Close();
      std::cout << "done!" << std::endl;
    } else {
      std::cout << "Stop signal detected. Skipping results saving.\n";
    }

    graphsList->Delete();

    return 0;
  } catch (const std::runtime_error& error) {
    std::cout << "RUNTIME ERROR: " << error.what() << std::endl;
    return 1;
  } catch (const std::invalid_argument& error) {
    std::cout << "INVALID ARGUMENT PROVIDED: " << error.what() << std::endl;
    return 1;
  } catch (const std::logic_error& error) {
    std::cout << "LOGIC ERROR: " << error.what() << std::endl;
    return 1;
  }
}
