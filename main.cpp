#include <SFML/Graphics/Color.hpp>
#include <chrono>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

#include "gasSim/Libs/INIReader.h"

#include <RtypesCore.h>
#include <TF1.h>
#include <TFile.h>
#include <TGraph.h>
#include <TLegend.h>
#include <TLine.h>
#include <TMultiGraph.h>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/Text.hpp>

#include "gasSim/DataProcessing.hpp"
#include "gasSim/Input.hpp"

std::atomic<double> GS::Particle::radius{1.};
std::atomic<double> GS::Particle::mass{1.};

GS::GSVectorD stovec(std::string s) {
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
	// trying to make root stop throwing temper tantrums over my variables
	TH1D::AddDirectory(kFALSE);

  try {
    std::cout << "Welcome. Starting idealGasSim gas simulation.\n";
    auto opts{GS::optParse(argc, argv)};
    if (GS::optControl(argc, opts)) {
      return 0;
    }
    std::string path = opts.count("config") != 0
                           ? opts["config"].as<std::string>()
                           : "configs/gasSim_demo.ini";
    INIReader cFile(path);
    if (cFile.ParseError() != 0) {
      throw std::runtime_error("Can't load " + path);
    }

    std::cout << "Starting resources loading. Using config file at path "
              << path << std::endl;

    GS::Particle::setMass(
        cFile.GetReal("simulation parameters", "pMass", 1.));
    GS::Particle::setRadius(
        cFile.GetReal("simulation parameters", "pRadius", 1.));

    TFile inputFile{TFile(
        (std::ostringstream()
         << "inputs/"
         << cFile.Get("simulation parameters", "ROOTInputPath", "input").c_str()
         << ".root")
            .str()
            .c_str())};
		throwIfZombie(&inputFile, "Failed to load provided input file path.");

    TH1D speedsHTemplate{*((TH1D*)inputFile.Get("speedsHTemplate"))};
		speedsHTemplate.SetDirectory(nullptr);
		throwIfZombie(&inputFile, "Failed to load speedsHTemplate from input file.");

    long nStats{cFile.GetInteger("simulation parameters", "nStats", 1)};
    if (nStats <= 0) {
      throw std::invalid_argument("Provided negative nStats.");
    }
    GS::SimDataPipeline output{
        static_cast<unsigned>(nStats > 0 ? nStats
                                         : throw std::invalid_argument(
                                               "Provided negative nStats.")),
        cFile.GetFloat("output", "framerate", 60.f), speedsHTemplate};
		sf::Font font;
		font.loadFromFile("assets/JetBrains-Mono-Nerd-Font-Complete.ttf");
		output.setFont(font);
    GS::Gas gas{static_cast<size_t>(
                    cFile.GetInteger("simulation parameters", "nParticles", 1)),
                cFile.GetFloat("simulation parameters", "targetT", 1),
                cFile.GetFloat("simulation parameters", "boxSide", 1.)};

    double targetBufferTime{cFile.GetReal("output", "targetBuffer", 2.5)};

    double desiredStatChunkSize{
        targetBufferTime * M_PI *
        std::pow(GS::Particle::getRadius() / gas.getBoxSide(), 2.) *
        std::accumulate(gas.getParticles().begin(), gas.getParticles().end(),
                        0.,
                        [](double acc, const GS::Particle& p) {
                          return acc + p.speed.norm();
                        }) /
        nStats / gas.getBoxSide()};
    output.setStatChunkSize(desiredStatChunkSize > 1. ? desiredStatChunkSize
                                                      : 1);
    double gasSide{gas.getBoxSide()};

    std::mutex coutMtx;
    std::thread simThread{[&] {
			try {
      gas.simulate(cFile.GetInteger("simulation parameters", "nIters", 0),
                   output);
			} catch (std::runtime_error const& e) {
				std::lock_guard<std::mutex> coutGuard{coutMtx};
				std::cout << "Runtime error: " << e.what() << std::endl;
				std::terminate();
			}
      std::lock_guard<std::mutex> coutGuard{coutMtx};
      std::cout << "Simulation thread done.                                      \r" << std::endl;
    }};

    {
      std::lock_guard<std::mutex> coutGuard{coutMtx};
      std::cout << "Simulation thread running." << std::endl;
    }

    GS::GSVectorF camPos{static_cast<GS::GSVectorF>(stovec(cFile.Get(
        "rendering", "camPos",
        (std::ostringstream() << "{" << gasSide * 1.5 << ", " << gasSide * 1.25
                              << ", " << gasSide * 0.75 << '}')
            .str())))};
    GS::GSVectorF camSight{static_cast<GS::GSVectorF>(stovec(cFile.Get(
        "rendering", "camSight",
        (std::ostringstream()
         << "{" << gasSide * 0.5 - camPos.x << ", " << gasSide * 0.5 - camPos.y
         << ", " << gasSide * 0.5 - camPos.z << '}')
            .str())))};

    sf::Vector2u windowSize{
        static_cast<unsigned>(cFile.GetInteger("output", "videoResX", 800)),
        static_cast<unsigned>(cFile.GetInteger("output", "videoResY", 600))};

    sf::Vector2u gasSize{1, 1};
    switch (stovideoopts(cFile.Get("output", "videoOpt", "justGas"))) {
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
                      cFile.GetFloat("render", "camLength", 1.f),
                      cFile.GetFloat("render", "fov", 90.f),
                      gasSize.x,
                      gasSize.y};

    sf::Texture particleTex;
    particleTex.loadFromFile(
        "assets/" + cFile.Get("render", "particleTexPath", "lightBall") +
        ".png");
    GS::RenderStyle style{particleTex};
    style.setWallsColor(
        sf::Color(cFile.GetInteger("render", "wallsColor", 0x50fa7b80)));
    style.setBGColor(
        sf::Color(cFile.GetInteger("render", "gasBgColor", 0xffffffff)));
    style.setWallsOpts(cFile.Get("render", "wallsOpts", "ufdl"));

    bool mfpMemory{cFile.GetBoolean("output", "mfpMemory", true)};

    const int frameTimems{static_cast<int>(1000. / output.getFramerate())};

    std::thread processThread;
    if (stovideoopts(cFile.Get("output", "videoOpt", "justGas")) !=
        GS::VideoOpts::justStats) {
      processThread =
          std::thread([&, mfpMemory] {
            output.processData(camera, style, mfpMemory);
            std::lock_guard<std::mutex> coutGuard{coutMtx};
            std::cout << "Data processing thread done.                         "
                         "               \r"
                      << std::endl;
          });
    } else {
      processThread =
          std::thread([&, mfpMemory] {
						try {
							output.processData(mfpMemory);
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

		std::vector<TObject*> allRootPtrs {};

    TList* graphsList = new TList();
		graphsList->SetOwner(kFALSE);
    allRootPtrs.emplace_back(graphsList);
    TMultiGraph* pGraphs = (TMultiGraph*)inputFile.Get("pGraphs");
		throwIfZombie(pGraphs, "Failed to load pressure graphs multigraph.");
    TGraph* kBGraph = (TGraph*)inputFile.Get("kBGraph");
		throwIfZombie(kBGraph, "Failed to load pressure graphs multigraph.");
    TGraph* mfpGraph = (TGraph*)inputFile.Get("mfpGraph");
		throwIfZombie(mfpGraph, "Failed to load pressure graphs multigraph.");
    graphsList->Add(pGraphs);
    graphsList->Add(kBGraph);
    graphsList->Add(mfpGraph);

    TF1* pLineF = (TF1*)inputFile.Get("pLineF");
		throwIfZombie(pLineF, "Failed to load pLineF from input file.");
    allRootPtrs.emplace_back(pLineF);
    TF1* kBGraphF = (TF1*)inputFile.Get("kBGraphF");
		throwIfZombie(kBGraphF, "Failed to load kBGraphF from input file.");
    allRootPtrs.emplace_back(kBGraphF);
    TF1* maxwellF = (TF1*)inputFile.Get("maxwellF");
		throwIfZombie(maxwellF, "Failed to load maxwellF from input file.");
    allRootPtrs.emplace_back(maxwellF);
    TF1* mfpGraphF = (TF1*)inputFile.Get("mfpGraphF");
		throwIfZombie(mfpGraphF, "Failed to load mfpGraphF from input file.");
    allRootPtrs.emplace_back(mfpGraphF);
    TH1D* cumulatedSpeedsH = (TH1D*)inputFile.Get("cumulatedSpeedsH");
		cumulatedSpeedsH->SetDirectory(nullptr);
		throwIfZombie(cumulatedSpeedsH, "Failed to load cumulatedSpeedsH from input file.");
    allRootPtrs.emplace_back(cumulatedSpeedsH);
    TLine* meanLine = (TLine*)inputFile.Get("meanLine");
		throwIfZombie(meanLine, "Failed to load meanLine from input file.");
    allRootPtrs.emplace_back(meanLine);
    TLine* meanSqLine = (TLine*)inputFile.Get("meanSqLine");
		throwIfZombie(meanSqLine, "Failed to load meanSqLine from input file.");
    allRootPtrs.emplace_back(meanSqLine);
    TF1* expP = (TF1*)inputFile.Get("expP");
		throwIfZombie(expP, "Failed to load expP from input file.");
    allRootPtrs.emplace_back(expP);
    TF1* expkB = (TF1*)inputFile.Get("expkB");
		throwIfZombie(expkB, "Failed to load expkB from input file.");
    allRootPtrs.emplace_back(expkB);
    TF1* expMFP = (TF1*)inputFile.Get("expMFP");
		throwIfZombie(expMFP, "Failed to load expMFP from input file.");
    allRootPtrs.emplace_back(expMFP);

    maxwellF->SetParameter(
        0, cFile.GetReal("simulation parameters", "targetT", 1.));
    maxwellF->FixParameter(
        1, cFile.GetInteger("simulation parameters", "nParticles", 1) *
               output.getStatSize());
    maxwellF->FixParameter(2,
                           cFile.GetReal("simulation parameters", "pMass", 1));

    std::function<void(TH1D&, GS::VideoOpts)> fitLambda{
        [&](TH1D& speedsH, GS::VideoOpts opt) {
          TGraph* genPGraph{(TGraph*)pGraphs->GetListOfGraphs()->At(6)};
          if (genPGraph->GetN()) {
            genPGraph->Fit(pLineF, "Q");
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
            kBGraph->Fit(kBGraphF, "Q");
						// keep most data visible but not squished down by outliers
						kBGraph->GetYaxis()->SetRangeUser(0., kBGraphF->GetParameter(0) * 3.25);
          }
          if (opt != GS::VideoOpts::gasPlusCoords) {
            if (speedsH.GetEntries()) {
              speedsH.Fit(maxwellF, "Q");
							// skibidi
              cumulatedSpeedsH->Add(&speedsH);
              meanLine->SetX1(speedsH.GetMean());
              meanLine->SetY1(0.);
              meanLine->SetX2(meanLine->GetX1());
              meanLine->SetY2(maxwellF->Eval(meanLine->GetX1()));
              double rms{speedsH.GetRMS()};
              meanSqLine->SetX1(
                  sqrt(rms * rms + meanLine->GetX1() * meanLine->GetX1()));
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
              mfpGraph->Fit(mfpGraphF, "Q");
            }
          }
        }};

    expP->SetParameter(
        0,
        cFile.GetFloat("simulation parameters", "targetT", 1) *
            cFile.GetInteger("simulation parameters", "nParticles", 1) /
            std::pow(cFile.GetReal("simulation parameters", "boxSide", 1), 3.));
    expkB->SetParameter(0, 1);
    expMFP->SetParameter(
        0, cFile.GetFloat("simulation parameters", "targetT", 1) / M_SQRT2 /
               expP->GetParameter(0) /
               (M_PI * GS::Particle::getRadius() * GS::Particle::getRadius()));
    // std::cout << "Expected MFP set at " << expMFP->GetParameter(0) << "
    // units." << std::endl;

    std::array<std::function<void()>, 4> drawLambdas{
        [&]() {
          TGraph* genPGraph{(TGraph*)pGraphs->GetListOfGraphs()->At(0)};
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

    GS::VideoOpts videoOpt{
        stovideoopts(cFile.Get("output", "videoOpt", "all"))};
    sf::Texture placeHolder;
    placeHolder.loadFromFile(
        (std::ostringstream()
         << "assets/" << cFile.Get("render", "placeHolderName", "placeholder")
         << ".png")
            .str()
            .c_str());

    while (!output.isProcessing()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    {
      std::lock_guard<std::mutex> coutGuard{coutMtx};
      std::cout << "All resources loaded. Starting video output processing."
                << std::endl;
    }

    if (cFile.GetBoolean("output", "saveVideo", "false")) {
      {
        std::lock_guard<std::mutex> coutGuard{coutMtx};
        std::cout << "Starting video encoding." << std::endl;
      }
      std::ostringstream cmd{};
      cmd << "ffmpeg -y -f rawvideo -pixel_format rgba -video_size "
          << windowSize.x << "x" << windowSize.y << " -framerate "
          << output.getFramerate() << " -i - -c:v libx264 -pix_fmt yuv420p "
          << "outputs/videos/"
          << cFile.Get("output", "videoOutputName", "output") << ".mp4";

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
        std::vector<sf::Texture> frames = {output.getVideo(
            videoOpt, {(int)windowSize.x, (int)windowSize.y}, placeHolder,
            *graphsList, true, fitLambda, drawLambdas)};
        int i{0};
        for (sf::Texture& t : frames) {
          auxImg = t.copyToImage();
          fwrite(auxImg.getPixelsPtr(), 1, windowSize.x * windowSize.y * 4,
                 ffmpeg);
          ++i;
          std::lock_guard<std::mutex> coutGuard{coutMtx};
          std::cout << "Encoding batch of " << frames.size()
                    << " frames: " << (float)i / (float)(frames.size())
                    << "% complete.                 \r";
          std::cout.flush();
        }
        if (lastBatch) {
          std::cout << "Encoding done!" << std::endl;
          break;
        }
      }
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
      std::mutex windowMtx; // used both for lastWindowTxtr and window
      window.setFramerateLimit(60);
      window.setActive(false);
      std::atomic<bool> stopBufferLoop{false};
      std::atomic<bool> bufferKillSignal{false};
      std::atomic<bool> stop{false};
      std::atomic<int> queueNumber{1};
      std::atomic<int> launchedPlayThreadsN{0};
      std::atomic<int> droppedFrames{0};
      std::atomic<int> framesToDrop{0};
			std::atomic<int> processedFrames{0};

      auto playLambda{[&, frameTimems](std::shared_ptr<std::vector<sf::Texture>> rPtr,
                                 int threadN) {
        sf::Sprite auxS;
        while (threadN != queueNumber) {
          if (stop.load()) break;
          std::this_thread::sleep_for(std::chrono::milliseconds(frameTimems));
        }
        if (!stop.load()) {
          stopBufferLoop = true;
          std::lock_guard<std::mutex> windowGuard{windowMtx};
          window.setActive();
          auto lastDrawEnd{std::chrono::high_resolution_clock::now()};
          float frameTimeS{static_cast<float>(frameTimems / 1000.)};
          std::chrono::duration<float> lastFrameDrawTime{};
          int i{0};
          for (const sf::Texture& r : *rPtr) {
            if (window.isOpen()) {
              if (!framesToDrop.load()) {
                lastFrameDrawTime =
                    std::chrono::high_resolution_clock::now() - lastDrawEnd;
                if (lastFrameDrawTime.count() <= frameTimeS) {
                  auxS.setTexture(r);
                  window.draw(auxS);
                  window.display();
                } else {
                  framesToDrop.fetch_add(
                      static_cast<int>(lastFrameDrawTime.count() /
                                       frameTimems) -
                      1);
                  droppedFrames.fetch_add(1);
                }
                lastDrawEnd = std::chrono::high_resolution_clock::now();
              } else {
                framesToDrop.fetch_sub(1);
                lastDrawEnd = std::chrono::high_resolution_clock::now();
              }
							processedFrames.fetch_sub(1);
            }
            std::lock_guard<std::mutex> coutGuard{coutMtx};
            std::cout << "Status: displaying. Progress: " << ++i
                      << " from batch of " << rPtr->size()
                      << " renders      \r";
            std::cout.flush();
          }
          queueNumber++;
          if (launchedPlayThreadsN == threadN) {
						lastWindowTxtr = rPtr->back();
            stopBufferLoop = false;
            std::lock_guard<std::mutex> coutGuard{coutMtx};
            std::cout << "Status: buffering.                                   "
                         "                          \r";
          }
          window.setActive(false);
        }
      }};
			sf::Texture bufferingWheelT;
			bufferingWheelT.loadFromFile(
				std::string("assets/") + cFile.Get("output", "bufferingWheel", "jesse") + std::string(".png")
			);
			sf::Sprite bufferingWheel;
			bufferingWheel.setTexture(bufferingWheelT, true);
			bufferingWheel.setOrigin(
					bufferingWheelT.getSize().x / 2.,
					bufferingWheelT.getSize().y / 2.
			);
			bufferingWheel.setScale(
					windowSize.x * 0.2 / bufferingWheelT.getSize().x,
					windowSize.y * 0.2 / bufferingWheelT.getSize().y
			);
			bufferingWheel.setPosition(windowSize.x / 2., windowSize.y / 2.);
			sf::Text bufferingText {
				"(UwU) loading (^w^)",
				font,
				static_cast<unsigned>(windowSize.y * 0.025)
			};
			sf::Text progressText {
				"0 iterations awaiting processing\n0 stats instances,\n0 renders awaiting composition\n0 frames ready to visualize",
				font,
				static_cast<unsigned>(windowSize.y * 0.01)
			};
			bufferingText.setFillColor(sf::Color::Black);
			progressText.setFillColor(sf::Color::Black);
			bufferingText.setOutlineColor(sf::Color::White);
			progressText.setOutlineColor(sf::Color::White);
			bufferingText.setOutlineThickness(4);
			progressText.setOutlineThickness(2);
			// I have no idea why the x value is that but hey it works
			// (the character size may() not be in pixels?)
			bufferingText.setOrigin(
					bufferingText.getLocalBounds().width / 2.f,
					bufferingText.getLocalBounds().height / 2.f
			);
			progressText.setOrigin(
				0., 0.
			);
			bufferingText.setPosition(
					bufferingWheel.getPosition().x,
					bufferingWheel.getPosition().y + windowSize.y * 0.2f);
			progressText.setPosition(
					windowSize.y * .05,
					windowSize.y * .05
			);
      std::thread bufferingLoop{[&, frameTimems, windowSize]() {
				sf::Sprite auxS;
				auxS.setTexture(lastWindowTxtr, true);
        while (true) {
          if (!stopBufferLoop) {
            std::lock_guard<std::mutex> windowGuard{windowMtx};
            window.setActive();
            while (window.isOpen() && !stopBufferLoop) {
							window.draw(auxS); // repaint last window texture
						 	// 120 degrees per second
							bufferingWheel.setRotation(bufferingWheel.getRotation() + 120. * frameTimems / 1000.);
							window.draw(bufferingWheel);
							window.draw(bufferingText);
							progressText.setString(
									std::to_string(output.getRawDataSize()) +
									" iterations awaiting processing\n" +
									std::to_string(output.getNStats()) +
									" stats instances,\n" +
									std::to_string(output.getNRenders()) +
									" renders awaiting composition\n" +
									std::to_string(processedFrames.load()) + 
									" processed frames"
							);
							window.draw(progressText);
              window.display();
              if (bufferKillSignal) {
                break;
              }
            }
            if (!window.isOpen()) {
              window.setActive(false);
              break;
            }
            if (bufferKillSignal) {
              break;
            }
            window.setActive(false);
          } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(frameTimems));
          }
          if (bufferKillSignal) {
            break;
          }
        }
      }};
      bool lastBatch{false};
      sf::Event e{};
      while (!stop) {
        std::shared_ptr<std::vector<sf::Texture>> rendersPtr{
            std::make_shared<std::vector<sf::Texture>>()};
        rendersPtr->reserve(output.getFramerate() * 10);
        while (rendersPtr->size() <= output.getFramerate() * targetBufferTime) {
          while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed) {
              window.close();
              bufferKillSignal.store(true);
              stop.store(true);
              std::this_thread::sleep_for(std::chrono::seconds(1));
              break;
            }
          }
          if (output.isDone() &&
              output.getRawDataSize() < output.getStatSize() &&
              !output.isProcessing()) {
            lastBatch = true;
          }
          std::vector<sf::Texture> v = {output.getVideo(
              videoOpt, {(int)windowSize.x, (int)windowSize.y}, placeHolder,
              *graphsList, true, fitLambda, drawLambdas)};
					processedFrames.fetch_add(v.size());
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
          playThread.detach();
        } else {
          std::thread playThread{
              [playLambda, rendersPtr, &launchedPlayThreadsN] {
                launchedPlayThreadsN.fetch_add(1);
                playLambda(rendersPtr, launchedPlayThreadsN.load());
              }};
          playThread.join();
          bufferKillSignal = true;
          {
            std::lock_guard<std::mutex> coutGuard{coutMtx};
            std::cout << "Done displaying. Closing window." << std::endl;
          }
          break;
        }
      }
      stopBufferLoop = true;
      if (bufferingLoop.joinable()) {
        bufferingLoop.join();
      }
      window.close();
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

    std::cout << "Saving results to file... ";
    std::cout.flush();
    inputFile.Close();
    TFile* rootOutput{new TFile(
        (std::ostringstream()
         << "outputs/" << cFile.Get("output", "rootOutputName", "output")
         << ".root")
            .str()
            .c_str(),
        "RECREATE")};

    rootOutput->SetTitle(rootOutput->GetName());
    rootOutput->cd();
    allRootPtrs.emplace_back(rootOutput);
    graphsList->Write();
    pLineF->Write();
    kBGraph->Write();
    kBGraphF->Write();
    mfpGraph->Write();
    mfpGraphF->Write();
    maxwellF->Write();
    cumulatedSpeedsH->Write();

    rootOutput->Close();

		for (TObject* o: allRootPtrs) {
			delete(o);
		}

    std::cout << "done!" << std::endl;

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
