#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <TFile.h>
#include <TGraph.h>
#include <TH1.h>
#include <TList.h>
#include <TMultiGraph.h>
#include <TObject.h>
#include <bits/chrono.h>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <atomic>
#include <functional>
#include <iostream>
#include <mutex>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "DataProcessing/SimDataPipeline.hpp"
#include "Graphics/Camera.hpp"
#include "Graphics/RenderStyle.hpp"
#include "PhysicsEngine/Gas.hpp"
#include "PhysicsEngine/Particle.hpp"
#include "doctest.h"
#include "testingAddons.hpp"

std::atomic<double> GS::Particle::mass = 10.;
std::atomic<double> GS::Particle::radius = 1.;

TH1D defaultH{};

TEST_CASE("Loosely testing the SimDataPipeline class") {
  SUBCASE("Throwing behaviour") {
    // Null statsize
    CHECK_THROWS(GS::SimDataPipeline(0, 1., defaultH));
    // Null and negative framerate
    CHECK_THROWS(GS::SimDataPipeline(1, 0., defaultH));
    CHECK_THROWS(GS::SimDataPipeline(1, -10., defaultH));
    GS::SimDataPipeline output{1, 1., defaultH};
    // Setting various parameters to invalid values
    CHECK_THROWS(output.setStatChunkSize(0));
    CHECK_THROWS(output.setFramerate(0.));
    CHECK_THROWS(output.setFramerate(-15.));
    CHECK_THROWS(output.setStatSize(0));
  }
  static bool i{0};
  SUBCASE("Simulation test demos") {
    std::string texts[2]{
        "Execute multiple particle rendering + graphs demo? (y/n) ",
        "Execute single particle just graphs demo? (y/n) "};
    std::cout << texts[i];
    char response;
    std::cin >> response;
    if (response == 'y') {
      std::cout << "Loading resources." << std::endl;
      TFile input{"assets/input.root"};
      TH1D* speedsHTemplate{dynamic_cast<TH1D*>(input.Get("speedsHTemplate"))};
      GS::SimDataPipeline output{1, 24., *speedsHTemplate};
      GS::Gas gas{10, 100., 30., -5.};
      output.setStatChunkSize(10);
      gas.simulate(30, output);
      output.setDone();
      sf::Texture ball;
      ball.loadFromFile("assets/lightBall.png");
      sf::Texture placeHolder;
      placeHolder.loadFromFile("assets/placeholder.png");
      sf::Font font;
      font.loadFromFile("assets/JetBrains-Mono-Nerd-Font-Complete.ttf");
      output.setFont(font);
      GS::RenderStyle style{ball};
      GS::Camera camera{{45, 37.5, 25}, {-30, -20, -6}, 1., 90., 720, 300};
      std::cout << "Resources loaded, starting processing." << std::endl;
      output.processData(camera, style);
      std::cout << "Done processing, starting getVideo session." << std::endl;
      SUBCASE("Running getVideo a bit and showing output") {
        std::cout << "Loading resources for getVideo." << std::endl;
        TList* graphsList = new TList();
        TMultiGraph* pGraphs{dynamic_cast<TMultiGraph*>(input.Get("pGraphs"))};
        TGraph* kBGraph{dynamic_cast<TGraph*>(input.Get("kBGraph"))};
        TGraph* mfpGraph{dynamic_cast<TGraph*>(input.Get("mfpGraph"))};
        if (pGraphs->IsZombie() || kBGraph->IsZombie() ||
            mfpGraph->IsZombie()) {
          throw std::runtime_error(
              "Couldn't find one or more graphs in provided root file.");
        }
        graphsList->Add(pGraphs);
        graphsList->Add(kBGraph);
        graphsList->Add(mfpGraph);
        std::cout << "Resources loaded. Running throw checks." << std::endl;
        SUBCASE("Throwing behaviour with window size") {
          CHECK_THROWS(output.getVideo(GS::VideoOpts::all, {600, 700},
                                       placeHolder, *graphsList));
          CHECK_THROWS(output.getVideo(GS::VideoOpts::all, {900, 500},
                                       placeHolder, *graphsList));
          CHECK_THROWS(output.getVideo(GS::VideoOpts::justStats, {500, 700},
                                       placeHolder, *graphsList));
          CHECK_THROWS(output.getVideo(GS::VideoOpts::justStats, {700, 500},
                                       placeHolder, *graphsList));
          TList empty{};
          CHECK_THROWS(output.getVideo(GS::VideoOpts::all, {800, 600},
                                       placeHolder, empty));
        }
        std::cout << "Calling getVideo." << std::endl;
        std::vector<sf::Texture> video{output.getVideo(
            GS::VideoOpts::all, {800, 600}, placeHolder, *graphsList, true)};
        // display in a window
        std::cout << "Starting display of video with " << video.size()
                  << " frames." << std::endl;
        sf::RenderWindow window{sf::VideoMode(800, 600), "getVideo display"};
        window.setFramerateLimit(24);
        sf::Event e;
        sf::Sprite auxS;
        while (response == 'y') {
          for (sf::Texture& t : video) {
            while (window.pollEvent(e)) {
              if (e.type == sf::Event::Closed) {
                window.close();
              }
            }
            if (window.isOpen()) {
              auxS.setTexture(t);
              window.draw(auxS);
              window.display();
            } else {
              break;
            }
          }
          std::cout << "Replay? (y/n) ";
          std::cin >> response;
        }
        std::cout << "Closing window." << std::endl;
        window.close();
        std::cout << "Deleting TObjects." << std::endl;
        graphsList->Delete();
        delete graphsList;
        i = true;
      }
      std::cout << "Starting single particle getVideo." << std::endl;
      GS::SimDataPipeline singleOutput{1, 24., *speedsHTemplate};
      GS::Gas onePGas{1, 50., 20., -9.};
      onePGas.simulate(10, singleOutput);
      singleOutput.setDone();
      singleOutput.setFont(font);
      // same
      std::cout << "Done simulating, processing data." << std::endl;
      singleOutput.processData();
      SUBCASE("Running getVideo on single particle gas and showing output") {
        std::cout << "Loading getVideo resources." << std::endl;
        // display in a window
        TList* graphsList = new TList();
        TMultiGraph* pGraphs{dynamic_cast<TMultiGraph*>(input.Get("pGraphs"))};
        TGraph* kBGraph{dynamic_cast<TGraph*>(input.Get("kBGraph"))};
        TGraph* mfpGraph{dynamic_cast<TGraph*>(input.Get("mfpGraph"))};
        if (pGraphs->IsZombie() || kBGraph->IsZombie() ||
            mfpGraph->IsZombie()) {
          throw std::runtime_error(
              "Couldn't find one or more graphs in provided root file.");
        }
        graphsList->Add(pGraphs);
        graphsList->Add(kBGraph);
        graphsList->Add(mfpGraph);
        CHECK_THROWS(singleOutput.getVideo(GS::VideoOpts::justStats, {600, 400},
                                           placeHolder, *graphsList));
        CHECK_THROWS(singleOutput.getVideo(GS::VideoOpts::justStats, {400, 600},
                                           placeHolder, *graphsList));
        std::vector<sf::Texture> video{
            singleOutput.getVideo(GS::VideoOpts::justStats, {600, 600},
                                  placeHolder, *graphsList, true)};
        // display in a window
        sf::RenderWindow window{sf::VideoMode(600, 600), "getVideo display"};
        window.setFramerateLimit(24);
        sf::Event e;
        sf::Sprite auxS;
        while (response == 'y') {
          for (sf::Texture& t : video) {
            while (window.pollEvent(e)) {
              if (e.type == sf::Event::Closed) {
                window.close();
              }
            }
            if (window.isOpen()) {
              auxS.setTexture(t);
              window.draw(auxS);
              window.display();
            } else {
              break;
            }
          }
          std::cout << "Replay? (y/n) ";
          std::cin >> response;
        }
        window.close();
        graphsList->Delete();
        delete graphsList;
        delete speedsHTemplate;
      }
      std::cout << "Done, going to next test case." << std::endl;
    } else {
      std::cout << "OK. Going to next text case." << std::endl;
    }
  }
}

TEST_CASE(
    "Bullying SimDataPipeline by calling all its methods at random intervals "
    "from multiple threads") {
  char response;
  std::cout << "Execute SimDataPipeline stress test? (y/n) ";
  std::cin >> response;
  sf::Font font;
  font.loadFromFile("assets/JetBrains-Mono-Nerd-Font-Complete.ttf");
  while (response == 'y') {
    std::cout << "Loading resources." << std::endl;
    GS::Gas g{10, 50., 20.};
    TFile input{"assets/input.root"};
    TH1D* speedsHTemplate{dynamic_cast<TH1D*>(input.Get("speedsHTemplate"))};
    GS::SimDataPipeline output{10, 24., *speedsHTemplate};
    GS::Gas gas{10, 100., 30., -5.};
    output.setStatChunkSize(1);
    output.setFont(font);
    std::cout << "Simulating." << std::endl;
    std::thread simThread{[&] {
      gas.simulate(100, output);
      output.setDone();
    }};
    std::cout << "Done simulating. Loading extra resources." << std::endl;
    sf::Texture ball;
    ball.loadFromFile("assets/lightBall.png");
    sf::Texture placeHolder;
    placeHolder.loadFromFile("assets/placeholder.png");
    GS::RenderStyle style{ball};
    GS::Camera camera{{30, 25, 15}, {-20, -15, -5}, 1., 90., 720, 300};
    TList* graphsList = new TList();
    TMultiGraph* pGraphs{dynamic_cast<TMultiGraph*>(input.Get("pGraphs"))};
    TGraph* kBGraph{dynamic_cast<TGraph*>(input.Get("kBGraph"))};
    TGraph* mfpGraph{dynamic_cast<TGraph*>(input.Get("mfpGraph"))};
    if (pGraphs->IsZombie() || kBGraph->IsZombie() || mfpGraph->IsZombie()) {
      throw std::runtime_error(
          "Couldn't find one or more graphs in provided root file.");
    }
    graphsList->Add(pGraphs);
    graphsList->Add(kBGraph);
    graphsList->Add(mfpGraph);

    std::cout << "Done loading. Adding disturbances to manager." << std::endl;

    std::mutex coutMtx;

    GS::randomThreadsMgr manager{};
    manager.add([&] {
      {
        std::lock_guard<std::mutex> coutGuard{coutMtx};
        std::cout << "Calling getRawDataSize()" << std::endl;
      }
      output.getRawDataSize();
    });
    manager.add([&] {
      {
        std::lock_guard<std::mutex> coutGuard{coutMtx};
        std::cout << "Calling getFramerate" << std::endl;
      }
      output.getFramerate();
    });
    manager.add([&] {
      {
        std::lock_guard<std::mutex> coutGuard{coutMtx};
        std::cout << "Calling getStatSize()" << std::endl;
      }
      output.getStatSize();
    });
    manager.add([&] {
      {
        std::lock_guard<std::mutex> coutGuard{coutMtx};
        std::cout << "Calling getStatChunkSize()" << std::endl;
      }
      output.getStatChunkSize();
    });
    manager.add([&] {
      {
        std::lock_guard<std::mutex> coutGuard{coutMtx};
        std::cout << "Calling isProcessing()" << std::endl;
      }
      output.isProcessing();
    });
    manager.add([&] {
      {
        std::lock_guard<std::mutex> coutGuard{coutMtx};
        std::cout << "Calling isDone()" << std::endl;
      }
      output.isDone();
    });
    manager.add([&] {
      thread_local std::mt19937 r{std::random_device{}()};
      std::uniform_int_distribution<unsigned> emptyQueueD{0, 1};
      bool emptyQueue{static_cast<bool>(emptyQueueD(r))};
      {
        std::lock_guard<std::mutex> coutGuard{coutMtx};
        std::cout << "Calling getStats(" << emptyQueue << ")" << std::endl;
      }
      output.getStats(emptyQueue);
    });
    manager.add([&] {
      thread_local std::mt19937 r{std::random_device{}()};
      std::uniform_int_distribution<unsigned> emptyQueueD{0, 1};
      bool emptyQueue{static_cast<bool>(emptyQueueD(r))};
      {
        std::lock_guard<std::mutex> coutGuard{coutMtx};
        std::cout << "Calling getRenders(" << emptyQueue << ")" << std::endl;
      }
      output.getRenders(emptyQueueD(r));
    });
    manager.add([&] {
      {
        std::lock_guard<std::mutex> coutGuard{coutMtx};
        std::cout << "Calling setStatSize()" << std::endl;
      }
      thread_local std::mt19937 r{std::random_device{}()};
      std::uniform_int_distribution<unsigned> emptyQueueD{1, 5};
      output.setStatSize(emptyQueueD(r));
      {
        std::lock_guard<std::mutex> coutGuard{coutMtx};
        std::cout << "statSize set to " + std::to_string(output.getStatSize())
                  << std::endl;
      }
    });

    {
      std::lock_guard<std::mutex> coutGuard{coutMtx};
      std::cout << "Starting disturbances." << std::endl;
    }
    manager.start();

    {
      std::lock_guard<std::mutex> coutGuard{coutMtx};
      std::cout << "Starting processing." << std::endl;
    }
    std::thread processThread{[&] {
      output.processData(camera, style, true);
      {
        std::lock_guard<std::mutex> coutGuard{coutMtx};
        std::cout << "Done processing." << std::endl;
      }
    }};

    sf::RenderWindow window{sf::VideoMode(800, 600), "getVideo display"};
    window.setFramerateLimit(24);
    sf::Sprite auxS;
    std::vector<sf::Texture> video{};
    sf::Event e;
    {
      std::lock_guard<std::mutex> coutGuard{coutMtx};
      std::cout << "Starting display." << std::endl;
    }
    while (true) {
      thread_local std::mt19937 r{std::random_device{}()};
      std::uniform_int_distribution<unsigned> optD{0, 3};
      std::uniform_int_distribution<unsigned> emptyQueueD{0, 1};

      {
        std::lock_guard<std::mutex> coutGuard{coutMtx};
        std::cout << "Calling getVideo." << std::endl;
      }
      video.clear();
      video = output.getVideo(GS::VideoOpts(optD(r)), {800, 600}, placeHolder,
                              *graphsList, emptyQueueD(r));
      if (video.size()) {
        {
          std::lock_guard<std::mutex> coutGuard{coutMtx};
          std::cout << "Video size = " + std::to_string(video.size()) +
                           std::string(", displaying.")
                    << std::endl;
        }
        for (sf::Texture& t : video) {
          while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed) {
              window.close();
              break;
            }
          }
          if (window.isOpen()) {
            auxS.setTexture(t, true);
            window.draw(auxS);
            window.display();
          }
        }
      } else {
        {
          std::lock_guard<std::mutex> coutGuard{coutMtx};
          std::cout << "Video empty, checking if output is done." << std::endl;
        }
        if (output.isDone() && output.getRawDataSize() < output.getStatSize() &&
            !output.isProcessing()) {
          {
            std::lock_guard<std::mutex> coutGuard{coutMtx};
            std::cout << "Closing. Bye!" << std::endl;
          }
          window.close();
          break;
        }
      }
    }

    {
      std::lock_guard<std::mutex> coutGuard{coutMtx};
      std::cout << "Joining process and video threads." << std::endl;
    }
    if (simThread.joinable()) {
      simThread.join();
    }
    if (processThread.joinable()) {
      processThread.join();
    }
    {
      std::lock_guard<std::mutex> coutGuard{coutMtx};
      std::cout << "Joining disturbances. Waiting ten seconds for all threads "
                   "to finish."
                << std::endl;
    }
    manager.finish();
    std::this_thread::sleep_for(std::chrono::seconds(10));
    std::cout << "Repeat SimDataPipeline stress test? (y/n) ";
    std::cin >> response;
    graphsList->Delete();
    delete graphsList;
    delete speedsHTemplate;
  };
  std::cout << "Bye!" << std::endl;
}
