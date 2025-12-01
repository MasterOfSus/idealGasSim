#include <RtypesCore.h>
#include <TCanvas.h>
#include <TGraph.h>
#include <TImage.h>
#include <TMultiGraph.h>

#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "SimDataPipeline.hpp"

// Hi, my name's Liam, I was the person who wrote this,
// I just wanted to say that I'm sorry for everything

namespace GS {

bool isNegligible(double epsilon, double x);  // implemented in TdStats.cpp

bool isIntMultOf(double x, double dt) {
  return isNegligible(x - dt * std::round(x / dt), dt);
}

// chatGPT wrote pretty much all of this function
void RGBA32toRGBA8(UInt_t const* rgbaBffr, size_t w, size_t h,
                   std::vector<sf::Uint8>& pixels) {
  size_t total{w * h};
  pixels.resize(total * 4);

  for (size_t idx{0}; idx < total; ++idx) {
    UInt_t rgba = rgbaBffr[idx];
    size_t base = idx * 4;

    pixels[base + 0] = (rgba >> 24) & 0xFF;  // R
    pixels[base + 1] = (rgba >> 16) & 0xFF;  // G
    pixels[base + 2] = (rgba >> 8) & 0xFF;   // B
    pixels[base + 3] = (rgba >> 0) & 0xFF;   // A
  }
}
// chatGPT written code ends here

std::string round2(double x) {
  static std::ostringstream ss;
  ss.str("");
  ss.clear();
  ss << std::fixed << std::setprecision(2) << x;

  return ss.str();
}

std::string scientificNotation(double x, size_t nDigits = 2) {
  static std::ostringstream ss;
  ss.str("");
  ss.clear();
  ss << std::scientific << std::setprecision(nDigits) << std::uppercase << x;
  return ss.str();
}

std::vector<sf::Texture> GS::SimDataPipeline::getVideo(
    VideoOpts opt, sf::Vector2i windowSize, sf::Texture const& placeholder,
    TList& outputGraphs, bool emptyStats,
    std::function<void(TH1D&, VideoOpts)> fitLambda,
    std::array<std::function<void()>, 4> drawLambdas) {
  static int nExec{0};
  std::cerr << std::endl
            << "Started getVideo, call number " << ++nExec << std::endl;

  if ((opt == VideoOpts::justStats &&
       (windowSize.x < 600 || windowSize.y < 600)) ||
      (opt != VideoOpts::justStats &&
       (windowSize.x < 800 || windowSize.y < 600))) {
    throw std::invalid_argument(
        "getVideo error: provided window size is too small");
  }

  if (outputGraphs.GetSize() < 3) {
    throw std::invalid_argument(
        "getVideo error: provided graphsList with less than three elements");
  }
  if (!(outputGraphs.At(0)->IsA() == TMultiGraph::Class() &&
        outputGraphs.At(1)->IsA() == TGraph::Class() &&
        outputGraphs.At(2)->IsA() == TGraph::Class())) {
    throw std::invalid_argument(
        "getVideo error: provided graphs list with wrong object types");
  }

  if (((TMultiGraph*)outputGraphs.At(0))->GetListOfGraphs()->GetSize() != 7) {
    throw std::invalid_argument(
        "getVideo error: provided multigraph number of graphs != 7");
  }

  if (font.getInfo().family.empty()) {
    throw std::runtime_error("getVideo error: called without setting font");
  }

  std::deque<std::pair<sf::Texture, double>> renders{};
  std::vector<TdStats> stats{};
  std::optional<double> gTime;
  double gDeltaT;

  auto getRendersT0 = [&](auto const& couples) {
    if (couples.size()) {
      assert(gTime.has_value());
      return couples[0].second;
    } else if (gTime.has_value()) {
      return *gTime;
    } else {
      throw std::logic_error(
          "getVideo error: tried to get render time for output with no time "
          "set");
    }
  };

  std::cerr << "Got to data extraction phase." << std::endl;
  // extraction of renders and/or stats clusters compatible with
  // processing algorithm
  // fTime_ initialization, either through gTime_ or through stats[0]
  switch (opt) {
    case VideoOpts::justGas: {  // case scope
      std::cerr << "Case justGas." << std::endl;
      {  // lock scope
        std::lock_guard<std::mutex> rGuard{rendersMtx};
        {  // locks scope 2
          std::lock_guard<std::mutex> gTimeGuard{gTimeMtx};
          gTime = this->gTime;
        }  // end of locks scope 2
        if (!gTime.has_value()) {
          return {};
        }
        gDeltaT = this->gDeltaT.load();
        if (this->renders.size()) {
          renders = std::move(this->renders);
          this->renders.clear();
        } else {
          return {};
        }
      }  // end of lock scope
      if ((!fTime.has_value() || !isIntMultOf(*gTime - *fTime, gDeltaT))) {
        fTime = getRendersT0(renders) - gDeltaT;
      }
      std::cerr << "Extracted renders size " << renders.size() << std::endl;
      std::cerr << "Renders time0 = " << renders.front().second
                << " and time = " << renders.back().second
                << " with gTime = " << *gTime << " and fTime = " << *fTime
                << std::endl;
      break;
    }  // end of case scope
    case VideoOpts::justStats: {  // lock/case scope
      std::cerr << "Case justStats." << std::endl;
      std::lock_guard<std::mutex> sGuard{statsMtx};
      {  // locks scope 2
        std::lock_guard<std::mutex> gTimeGuard{gTimeMtx};
        gTime = this->gTime;
      }  // end of locks scope 2
      gDeltaT = this->gDeltaT.load();
      std::cerr << "Loaded gTime and gDeltaT." << std::endl;
      if (this->stats.size()) {
        std::cerr << "Found stats size " << this->stats.size() << std::endl;
        // setting fTime_
        if (!fTime.has_value()) {
          std::cerr << "Setting fTime." << std::endl;
          if (gTime.has_value()) {
            fTime = *gTime +
                    gDeltaT * std::floor((this->stats[0].getTime0() - *gTime) /
                                         gDeltaT);
          } else {
            fTime = this->stats.front().getTime0() - gDeltaT;
          }
        }
        if (this->stats.back().getTime() >= *fTime + gDeltaT) {
          std::cerr << "Extracting stats vector." << std::endl;
          // find first where the next frame's time is before it
          // -> the one before it necessarily "contains" the frame time
          // this pattern used in the rest of this file
          auto sStartI{std::upper_bound(this->stats.begin(), this->stats.end(),
                                        *fTime + gDeltaT,
                                        [](double value, TdStats const& s) {
                                          return value < s.getTime0();
                                        }) -
                       1};
          std::cerr << "Found sStartI " << sStartI - this->stats.begin()
                    << std::endl;
          if (sStartI < this->stats.begin()) {
            std::cerr << "Setting sStartI to stats begin." << std::endl;
            sStartI = this->stats.begin();
          }
          if (emptyStats) {
            std::cerr << "Moving stats vector." << std::endl;
            stats.insert(stats.end(), std::make_move_iterator(sStartI),
                         std::make_move_iterator(this->stats.end()));
            this->stats.clear();
          } else {
            std::cerr << "Copying stats vector." << std::endl;
            stats = std::vector<TdStats>{sStartI, this->stats.end()};
          }
        } else {
          std::cerr
              << "Found fTime + gDeltaT > stats back time, returning empty."
              << std::endl;
          return {};
        }
      } else {
        std::cerr << "Found empty stats_, returning empty vector." << std::endl;
        return {};
      }
      break;
    }  // end of lock/case scope
    case VideoOpts::all:
      std::cerr << "VideoOpt all." << std::endl;
    case VideoOpts::gasPlusCoords: {  // locks/case scope
      std::cerr << "VideoOpt gasPlusCoords." << std::endl;
      std::unique_lock<std::mutex> resultsLock{outputMtx};
      outputCv.wait_for(resultsLock, std::chrono::milliseconds(50),
                        [this]() { return addedResults.load(); });
      std::lock_guard<std::mutex> rGuard{rendersMtx};
      std::lock_guard<std::mutex> sGuard{statsMtx};
      {  // lock scope 2
        std::lock_guard<std::mutex> gTimeGuard{gTimeMtx};
        gTime = this->gTime;
      }  // end of lock scope 2
      gDeltaT = this->gDeltaT.load();
      std::cerr << "Initialized gTime with value "
                << (gTime.has_value() ? std::to_string(*gTime) : "empty")
                << ", gDeltaT with value " << gDeltaT << std::endl;
      if (!gTime.has_value()) {
        return {};
      }
      if (this->renders.size() || this->stats.size()) {
        std::cerr << "Found sizes: renders_ = " << this->renders.size()
                  << ", stats_ = " << this->stats.size() << std::endl;
        //  dealing with fTime_
        if (!fTime.has_value() || !isIntMultOf(*gTime - *fTime, gDeltaT)) {
          if (!this->stats.size()) {
            fTime = getRendersT0(this->renders) - gDeltaT;
          } else if (!this->renders.size()) {
            fTime =
                *gTime +
                gDeltaT * std::floor((this->stats.front().getTime0() - *gTime) /
                                     gDeltaT);
            assert(isIntMultOf(*fTime - *gTime, gDeltaT));
          } else {
            if (getRendersT0(this->renders) > this->stats.front().getTime0()) {
              fTime = getRendersT0(this->renders) +
                      gDeltaT * std::floor((this->stats[0].getTime0() -
                                            getRendersT0(this->renders)) /
                                           gDeltaT);
              std::cerr << "Found getRendersT0 = "
                        << getRendersT0(this->renders)
                        << ", stats beginning = " << this->stats[0].getTime0()
                        << "; set fTime_ to " << *fTime << std::endl;
            } else {
              fTime = getRendersT0(this->renders) - gDeltaT;
              std::cerr << "Found getRendersT0 = "
                        << getRendersT0(this->renders)
                        << ", stats beginning = " << this->stats[0].getTime0()
                        << "; set fTime_ to " << *fTime << std::endl;
            }
          }
          std::cerr << "Set fTime_. Now fTime_.has_value() returns "
                    << fTime.has_value() << " with value = " << fTime.value()
                    << std::endl;
        }

        std::cerr << "Starting vector segments extraction." << std::endl;
        //  extracting algorithm-compatible vector segments
        if (this->stats.size()) {
          auto sStartI{std::upper_bound(this->stats.begin(), this->stats.end(),
                                        *fTime + gDeltaT,
                                        [](double value, TdStats const& s) {
                                          return value < s.getTime0();
                                        }) -
                       1};
          if (sStartI < this->stats.begin()) {
            sStartI = this->stats.begin();
          }
          std::cerr << "There's nothing to fear but BIG SCARY MONSTERS "
                       "AAAAAHHHHH!!!!!!!!\n";
          std::cerr << "Found sStartI = " << sStartI - this->stats.begin()
                    << std::endl;
          std::cerr << "fTime_ value = " << *fTime
                    << " with stats_ front Time0 = "
                    << this->stats.front().getTime0()
                    << " and Time = " << this->stats.front().getTime()
                    << std::endl;
          if (sStartI != this->stats.end()) {
            auto gStartI{std::upper_bound(this->renders.begin(),
                                          this->renders.end(), *fTime + gDeltaT,
                                          [](double value, auto const& render) {
                                            return value <= render.second;
                                          })};
            assert(isIntMultOf(*gTime - *fTime, gDeltaT));
            auto gEndI{std::upper_bound(this->renders.begin(),
                                        this->renders.end(),
                                        this->stats.back().getTime(),
                                        [](double value, auto const& render) {
                                          return value < render.second;
                                        })};
            std::cerr << "Found gStartI = " << gStartI - this->renders.begin()
                      << ", gEndI = " << gEndI - this->renders.begin()
                      << ", with gSize = " << renders.size() << std::endl;
            if (emptyStats) {
              stats.insert(stats.end(), std::make_move_iterator(sStartI),
                           std::make_move_iterator(this->stats.end()));
              this->stats.clear();
            } else {
              stats = std::vector(sStartI, this->stats.end());
            }
            assert(gStartI <= gEndI);
            renders.insert(renders.begin(), std::make_move_iterator(gStartI),
                           std::make_move_iterator(gEndI));
            this->renders.clear();
          }
          std::cerr << "Extracted vectors. Result sizes: stats size = "
                    << stats.size() << ", renders size = " << renders.size()
                    << std::endl;
          if (!stats.size() && !renders.size()) {
            std::cerr << "Returning because found both vectors empty."
                      << std::endl;
            return {};
          }
        } else {
          auto gStartI{std::upper_bound(this->renders.begin(),
                                        this->renders.end(), *fTime + gDeltaT,
                                        [](double value, auto const& render) {
                                          return value <= render.second;
                                        })};
          // same as above
          renders.insert(renders.begin(), std::make_move_iterator(gStartI),
                         std::make_move_iterator(this->renders.end()));
          this->renders.clear();
        }
      } else {
        std::cerr << "Found empty stats and renders, returning empty vector."
                  << std::endl;
        return {};
      }
      addedResults.store(false);
      resultsLock.unlock();
      break;
    }  // end of locks/case scope
    default:
      throw std::invalid_argument("Invalid video option provided.");
  }

  std::cerr << "Got to second phase." << std::endl;

  // recurrent variables setup
  // declarations
  // text
  std::ostringstream Temp{};
  sf::Text TText;
  std::ostringstream Vol{};
  sf::Text VText;
  std::ostringstream Num{};
  sf::Text NText;
  sf::RectangleShape txtBox;
  sf::Text timeText;
  timeText.setFont(font);
  timeText.setCharacterSize(18);
  timeText.setFillColor(sf::Color::Black);
  timeText.setOutlineColor(sf::Color::White);
  timeText.setOutlineThickness(1.);
  timeText.setPosition(windowSize.x * 0.01, windowSize.y * 0.01);
  // graphs
  TMultiGraph& pGraphs{*(TMultiGraph*)outputGraphs.At(0)};
  TGraph& kBGraph{*(TGraph*)outputGraphs.At(1)};
  TGraph& mfpGraph{*(TGraph*)outputGraphs.At(2)};
  TCanvas cnvs{};
  // image stuff
  TImage* trnsfrImg;
  std::vector<sf::Uint8> pixels;
  sf::Texture auxTxtr;
  sf::Sprite auxSprt;
  sf::RenderTexture frame;
  sf::Sprite box;
  std::vector<sf::Texture> frames{};

  auto drawObj{[&](auto& TDrawable, double percPosX, double percPosY,
                   char const* drawOpts = "",
                   std::function<void()> drawLambda = {}) {
    cnvs.Clear();
    TDrawable.Draw(drawOpts);
    if (drawLambda) {
      drawLambda();
    }
    cnvs.Modified();
    cnvs.Update();
    trnsfrImg = TImage::Create();
    trnsfrImg->FromPad(&cnvs);
    auto* buf{trnsfrImg->GetRgbaArray()};
    if (!buf) {
      throw std::runtime_error("Failed to retrieve image RGBA array, aborting");
    }
    RGBA32toRGBA8(trnsfrImg->GetRgbaArray(), trnsfrImg->GetWidth(),
                  trnsfrImg->GetHeight(), pixels);
    auxTxtr.update(pixels.data());
    auxSprt.setTexture(auxTxtr, true);
    auxSprt.setPosition(windowSize.x * percPosX, windowSize.y * percPosY);
    frame.draw(auxSprt);
  }};

  {  // charSize
    double charSize{windowSize.y *
                    (opt == VideoOpts::gasPlusCoords ? 0.5 / 9. : 0.05)};
    TText.setCharacterSize(charSize);
    VText.setCharacterSize(charSize);
    NText.setCharacterSize(charSize);
  }
  // definitions as per available data
  switch (opt) {
    case VideoOpts::gasPlusCoords:
    case VideoOpts::all:
    case VideoOpts::justStats:
      if (stats.size()) {
        Temp << std::fixed << std::setprecision(2) << stats[0].getTemp();
        TText = {"T = " + Temp.str() + "K", font, 18};
        TText.setFont(font);
        TText.setFillColor(sf::Color::Black);
        VText = {"V = " + scientificNotation(stats[0].getVolume()) + "m^3",
                 font, 18};
        VText.setFont(font);
        VText.setFillColor(sf::Color::Black);
        Num << std::fixed << std::setprecision(2) << stats[0].getNParticles();
        NText = {" N = " + Num.str(), font, 18};
        NText.setFont(font);
        NText.setFillColor(sf::Color::Black);
        trnsfrImg = TImage::Create();
      }
      break;
    case VideoOpts::justGas:
      break;
    default:
      throw std::invalid_argument("Invalid video option provided.");
  }

  // processing
  std::cerr << "Got to phase 3." << std::endl;
  std::cerr << "Working vector sizes:\nRenders: " << renders.size()
            << "\nStats:" << stats.size() << std::endl;
  frame.create(windowSize.x, windowSize.y);
  switch (opt) {
    case VideoOpts::justGas: {
      if (renders.size()) {
        assert(isIntMultOf(*fTime - *gTime, gDeltaT));
        assert(gTime == renders.back().second);
        assert(fTime <= getRendersT0(renders));
        box.setPosition(0, 0);
        size_t rIndex{0};
        while (*fTime + gDeltaT < gTime ||
               isNegligible(*fTime + gDeltaT - *gTime, gDeltaT)) {
          *fTime += gDeltaT;
          timeText.setString("time = " + round2(*fTime));
          assert(isIntMultOf(*gTime - *fTime, gDeltaT));
          if (renders.size() > rIndex &&
              isNegligible(*fTime - renders[rIndex].second, gDeltaT)) {
            box.setScale(
                windowSize.x / (float)renders[rIndex].first.getSize().x,
                windowSize.y / (float)renders[rIndex].first.getSize().y);
            box.setTexture(renders[rIndex++].first);
            frame.draw(box);
            frame.draw(timeText);
            frame.display();
          } else {
            box.setScale(windowSize.x / (float)placeholder.getSize().x,
                         windowSize.y / (float)placeholder.getSize().y);
            box.setTexture(placeholder, true);
            frame.draw(box);
            frame.draw(timeText);
            frame.display();
          }
          frames.emplace_back(frame.getTexture());
        }
        renders.clear();
      }
      break;
    }
    case VideoOpts::justStats:
      if (stats.size()) {
        if (*fTime + gDeltaT <
            stats.front()
                .getTime0()) {  // insert empty data and use placeholder render
          auto& stat{stats.front()};
          for (size_t i{0}; i < 7; ++i) {
            TGraph* graph{(TGraph*)pGraphs.GetListOfGraphs()->At(i)};
            graph->AddPoint(*fTime, -1.);
            graph->AddPoint(stat.getTime0(), -1.);
          }

          mfpGraph.AddPoint(*fTime, -1.);
          mfpGraph.AddPoint(stat.getTime0(), -1.);
          kBGraph.AddPoint(*fTime, -1.);
          kBGraph.AddPoint(stat.getTime0(), -1.);

          frame.clear();

          cnvs.SetCanvasSize(windowSize.x * 0.5, windowSize.y * 0.45);
          ;
          auxTxtr.create(windowSize.x * 0.5, windowSize.y * 0.45);

          drawObj(pGraphs, 0., 0., "APL", drawLambdas[0]);
          drawObj(kBGraph, 0., windowSize.y * 0.55, "APL", drawLambdas[1]);

          cnvs.SetCanvasSize(windowSize.x * 0.5, windowSize.y * 0.5);
          auxTxtr.create(windowSize.x * 0.5, windowSize.y * 0.5);

          drawObj(mfpGraph, 0.5, 0.5, "APL", drawLambdas[3]);

          txtBox.setSize({windowSize.x * 0.5f, windowSize.y * 0.1f});
          txtBox.setPosition(0., windowSize.y * 0.45);
          VText.setPosition(windowSize.x * (0. + 0.025),
                            windowSize.y * (0.45 + 0.025));
          NText.setPosition(windowSize.x * (0. + 0.175),
                            windowSize.y * (0.45 + 0.025));
          TText.setPosition(windowSize.x * (0. + 0.325),
                            windowSize.y * (0.45 + 0.025));
          frame.draw(txtBox);
          frame.draw(VText);
          frame.draw(NText);
          frame.draw(TText);

          box.setScale(windowSize.x * 0.5 / (float)placeholder.getSize().x,
                       windowSize.y * 0.5 / (float)placeholder.getSize().y);
          box.setPosition(windowSize.x * 0.5, windowSize.y);
          box.setTexture(placeholder, true);
          frame.draw(box);

          frame.display();

          while (*fTime + gDeltaT < stat.getTime0()) {
            *fTime += gDeltaT;
            frames.emplace_back(frame.getTexture());
          }
        }

        for (TdStats const& stat : stats) {
          for (size_t i{0}; i < 7; ++i) {
            TGraph* graph{(TGraph*)pGraphs.GetListOfGraphs()->At(i)};
            if (i < 6) {
              graph->AddPoint(stat.getTime0(), stat.getPressure(Wall(i)));
              graph->AddPoint(stat.getTime(), stat.getPressure(Wall(i)));
            } else {
              graph->AddPoint(stat.getTime0(), stat.getPressure());
              graph->AddPoint(stat.getTime(), stat.getPressure());
            }
          }

          mfpGraph.AddPoint(stat.getTime0(), stat.getMeanFreePath());
          mfpGraph.AddPoint(stat.getTime(), stat.getMeanFreePath());
          kBGraph.AddPoint(stat.getTime0(),
                           stat.getPressure() * stat.getVolume() /
                               (stat.getNParticles() * stat.getTemp()));
          kBGraph.AddPoint(stat.getTime(),
                           stat.getPressure() * stat.getVolume() /
                               (stat.getNParticles() * stat.getTemp()));

          TH1D speedH{stat.getSpeedH()};

          if (fitLambda) {
            fitLambda(speedH, opt);
          }

          frame.clear();

          cnvs.SetCanvasSize(windowSize.x * 0.5, windowSize.y * 0.45);
          auxTxtr.create(windowSize.x * 0.5, windowSize.y * 0.45);

          drawObj(pGraphs, 0., 0., "APL", drawLambdas[0]);
          drawObj(kBGraph, 0., 0.55, "APL", drawLambdas[1]);

          cnvs.SetCanvasSize(windowSize.x * 0.5, windowSize.y * 0.5);
          auxTxtr.create(windowSize.x * 0.5, windowSize.y * 0.5);

          drawObj(speedH, 0.5, 0., "HIST", drawLambdas[2]);
          drawObj(mfpGraph, 0.5, 0.5, "APL", drawLambdas[3]);

          txtBox.setSize({windowSize.x * 0.5f, windowSize.y * 0.1f});
          txtBox.setPosition(0., windowSize.y * 0.45);
          VText.setPosition(windowSize.x * (0. + 0.025),
                            windowSize.y * (0.45 + 0.025));
          NText.setPosition(windowSize.x * (0. + 0.175),
                            windowSize.y * (0.45 + 0.025));
          TText.setPosition(windowSize.x * (0. + 0.325),
                            windowSize.y * (0.45 + 0.025));
          frame.draw(txtBox);
          frame.draw(VText);
          frame.draw(NText);
          frame.draw(TText);

          frame.display();

          while (*fTime + gDeltaT < stat.getTime()) {
            *fTime += gDeltaT;
            frames.emplace_back(frame.getTexture());
          }
        }
        trnsfrImg->Delete();
        stats.clear();
      }
      break;
    case VideoOpts::gasPlusCoords:
      if (renders.size()) {
        assert(gTime == renders.back().second);
        assert(fTime <= getRendersT0(renders));
      }
      {  // case scope
        timeText.setPosition(windowSize.x * 0.51, windowSize.y * 0.01);
        sf::Vector2u gasSize{static_cast<unsigned>(windowSize.x * 0.5),
                             static_cast<unsigned>(windowSize.y * 0.9)};
        if (stats.size()) {
          std::cerr << "Found stats size = " << stats.size()
                    << " the dough smells like my dick man the girls eat it."
                    << std::endl;
          auxTxtr.create(windowSize.x * 0.5, windowSize.y * 0.5);
          if (*fTime + gDeltaT < stats.front().getTime0()) {
            std::cerr << "Inserting empty data" << std::endl;
            for (size_t i{0}; i < 7; ++i) {
              TGraph* graph{(TGraph*)pGraphs.GetListOfGraphs()->At(i)};
              graph->AddPoint(*fTime, -1.);
              graph->AddPoint(stats.front().getTime0(), -1.);
            }

            kBGraph.AddPoint(*fTime, -1.);
            kBGraph.AddPoint(stats.front().getTime0(), -1.);

            frame.clear();

            cnvs.SetCanvasSize(windowSize.x * 0.5, windowSize.y * 0.5);
            std::cerr << "Calling drawObj, oh boy" << std::endl;
            drawObj(pGraphs, 0., 0., "APL", drawLambdas[0]);
            drawObj(kBGraph, 0., 0.5, "APL", drawLambdas[1]);

            txtBox.setSize({windowSize.x * 0.5f, windowSize.y * 1.f / 9.f});
            txtBox.setPosition(windowSize.x * 0.5, windowSize.y * 8. / 9.);
            VText.setPosition(windowSize.x * (0.5 + 0.025),
                              windowSize.y * (8. / 9. + 1. / 36.));
            NText.setPosition(windowSize.x * (0.5 + 0.175),
                              windowSize.y * (8. / 9. + 1. / 36.));
            TText.setPosition(windowSize.x * (0.5 + 0.325),
                              windowSize.y * (8. / 9. + 1. / 36.));
            frame.draw(txtBox);
            frame.draw(VText);
            frame.draw(NText);
            frame.draw(TText);
            // insert paired renders or placeholders
            box.setPosition(windowSize.x * 0.5, 0.);
            size_t rIndex{0};
            while (*fTime + gDeltaT < stats.front().getTime0()) {
              *fTime += gDeltaT;
              timeText.setString("time = " + round2(*fTime));
              assert(isIntMultOf(*gTime - *fTime, gDeltaT));
              if (renders.size() > rIndex &&
                  isNegligible(*fTime - renders[rIndex].second, gDeltaT)) {
                box.setScale(
                    gasSize.x / (float)renders[rIndex].first.getSize().x,
                    gasSize.y / (float)renders[rIndex].first.getSize().y);
                box.setTexture(renders[rIndex++].first);
                frame.draw(box);
                frame.draw(timeText);
                frame.display();
              } else {
                box.setScale(gasSize.x / (float)placeholder.getSize().x,
                             gasSize.y / (float)placeholder.getSize().y);
                box.setTexture(placeholder, true);
                frame.draw(box);
                frame.draw(timeText);
                frame.display();
              }
              frames.emplace_back(frame.getTexture());
            }
          }
          assert(*fTime + gDeltaT >= stats.front().getTime0());
          size_t rIndex{0};
          while (*fTime + gDeltaT < stats.back().getTime()) {
            auto s{std::upper_bound(stats.begin(), stats.end(),
                                    *fTime + gDeltaT,
                                    [](double value, TdStats const& stat) {
                                      return value < stat.getTime0();
                                    }) -
                   1};
            assert(s >= stats.begin());
            assert(s < stats.end());
            TdStats const& stat{*s};
            // make the graphs picture
            for (size_t i{0}; i < 7; ++i) {
              TGraph* graph{(TGraph*)pGraphs.GetListOfGraphs()->At(i)};
              if (i < 6) {
                graph->AddPoint(stat.getTime0(), stat.getPressure(Wall(i)));
                graph->AddPoint(stat.getTime(), stat.getPressure(Wall(i)));
              } else {
                graph->AddPoint(stat.getTime0(), stat.getPressure());
                graph->AddPoint(stat.getTime(), stat.getPressure());
              }
            }
            kBGraph.AddPoint(stat.getTime0(),
                             stat.getPressure() * stat.getVolume() /
                                 (stat.getNParticles() * stat.getTemp()));
            kBGraph.AddPoint(stat.getTime(),
                             stat.getPressure() * stat.getVolume() /
                                 (stat.getNParticles() * stat.getTemp()));

            TH1D h{};

            if (fitLambda) {
              fitLambda(h, opt);
            }

            frame.clear();

            cnvs.SetCanvasSize(windowSize.x * 0.5, windowSize.y * 0.5);
            drawObj(pGraphs, 0., 0., "APL", drawLambdas[0]);
            drawObj(kBGraph, 0., 0.5, "APL", drawLambdas[1]);

            txtBox.setSize({windowSize.x * 0.5f, windowSize.y * 1.f / 9.f});
            txtBox.setPosition(windowSize.x * 0.5, windowSize.y * 8. / 9.);
            VText.setPosition(windowSize.x * (0.5 + 0.025),
                              windowSize.y * (8. / 9. + 1. / 36.));
            NText.setPosition(windowSize.x * (0.5 + 0.175),
                              windowSize.y * (8. / 9. + 1. / 36.));
            TText.setPosition(windowSize.x * (0.5 + 0.325),
                              windowSize.y * (8. / 9. + 1. / 36.));
            frame.draw(txtBox);
            frame.draw(VText);
            frame.draw(NText);
            frame.draw(TText);
            // insert paired renders or placeholders
            box.setPosition(windowSize.x * 0.5, 0.);
            while (*fTime + gDeltaT < stat.getTime()) {
              *fTime += gDeltaT;
              timeText.setString("time = " + round2(*fTime));
              assert(isIntMultOf(*gTime - *fTime, gDeltaT));
              if (renders.size() > rIndex &&
                  isNegligible(*fTime - renders[rIndex].second, gDeltaT)) {
                box.setScale(
                    gasSize.x / (float)renders[rIndex].first.getSize().x,
                    gasSize.y / (float)renders[rIndex].first.getSize().y);
                box.setTexture(renders[rIndex++].first);
                frame.draw(box);
                frame.draw(timeText);
                frame.display();
              } else {
                box.setScale(gasSize.x / (float)placeholder.getSize().x,
                             gasSize.y / (float)placeholder.getSize().y);
                box.setTexture(placeholder, true);
                frame.draw(box);
                frame.draw(timeText);
                frame.display();
              }
              frames.emplace_back(frame.getTexture());
            }
          }  // while (fTime_ + gDeltaT_ < stats.back().getTime())
          trnsfrImg->Delete();
        } else if (renders.size()) {
          std::cerr << "Found renders size = " << renders.size()
                    << " but I never pop erections." << std::endl;
          assert(renders.back().second == gTime);
          if (*fTime + gDeltaT < gTime ||
              isNegligible(*fTime + gDeltaT - *gTime, gDeltaT)) {
            std::cerr << "Found fTime + gDeltaT " << *fTime + gDeltaT << " < "
                      << *gTime << ", gTime." << std::endl;
            for (size_t i{0}; i < 7; ++i) {
              TGraph* graph{(TGraph*)pGraphs.GetListOfGraphs()->At(i)};
              graph->AddPoint(*fTime, -1.);
              graph->AddPoint(*gTime, -1.);
            }

            kBGraph.AddPoint(*fTime, -1.);
            kBGraph.AddPoint(*gTime, -1.);

            frame.clear();

            cnvs.SetCanvasSize(windowSize.x * 0.5, windowSize.y * 0.5);
            std::cerr << "Calling drawObj" << std::endl;
            drawObj(pGraphs, 0., 0., "APL", drawLambdas[0]);
            drawObj(kBGraph, 0., 0.5, "APL", drawLambdas[1]);
            std::cerr << "Called drawObj" << std::endl;

            txtBox.setSize({windowSize.x * 0.5f, windowSize.y * 1.f / 9.f});
            txtBox.setPosition(windowSize.x * 0.5, windowSize.y * 8. / 9.);
            VText.setPosition(windowSize.x * (0.5 + 0.025),
                              windowSize.y * (8. / 9. + 1. / 36.));
            NText.setPosition(windowSize.x * (0.5 + 0.175),
                              windowSize.y * (8. / 9. + 1. / 36.));
            TText.setPosition(windowSize.x * (0.5 + 0.325),
                              windowSize.y * (8. / 9. + 1. / 36.));
            frame.draw(txtBox);
            frame.draw(VText);
            frame.draw(NText);
            frame.draw(TText);
            box.setPosition(windowSize.x * 0.5, 0.);
            std::cerr << "Got to actual renders insertion." << std::endl;
            size_t rIndex{0};
            while (*fTime + gDeltaT < gTime ||
                   isNegligible(*fTime + gDeltaT - *gTime, gDeltaT)) {
              *fTime += gDeltaT;
              timeText.setString("time = " + round2(*fTime));
              assert(isIntMultOf(*gTime - *fTime, gDeltaT));
              if (renders.size() > rIndex &&
                  isNegligible(*fTime - renders[rIndex].second, gDeltaT)) {
                std::cerr << "Inserting render n. " << rIndex << std::endl;
                box.setScale(
                    gasSize.x / (float)renders[rIndex].first.getSize().x,
                    gasSize.y / (float)renders[rIndex].first.getSize().y);
                box.setTexture(renders[rIndex++].first);
                frame.draw(box);
                frame.draw(timeText);
                frame.display();
              } else {
                std::cerr << "Inserting placeholder." << std::endl;
                box.setScale(gasSize.x / (float)placeholder.getSize().x,
                             gasSize.y / (float)placeholder.getSize().y);
                box.setTexture(placeholder, true);
                frame.draw(box);
                frame.draw(timeText);
                frame.display();
              }
              frames.emplace_back(frame.getTexture());
            }
          }
          trnsfrImg->Delete();
        }  // else if renders.size()
        renders.clear();
        break;
      }  // case scope end
    case VideoOpts::all: {  // case scope
      std::cerr << "Inserting frames: ";
      if (renders.size()) {
        assert(fTime <= getRendersT0(renders));
        assert(gTime == renders.back().second);
      }
      timeText.setPosition(windowSize.x * 0.26, windowSize.y * 0.01);
      sf::Vector2u gasSize{static_cast<unsigned>(windowSize.x * 0.5),
                           static_cast<unsigned>(windowSize.y * 0.9)};
      if (stats.size()) {
        std::cerr << "Found stats size = " << stats.size()
                  << " I don't wanna be around a woman anymo." << std::endl;
        auxTxtr.create(windowSize.x * 0.25, windowSize.y * 0.5);
        if (*fTime + gDeltaT < stats.front().getTime0()) {
          for (size_t i{0}; i < 7; ++i) {
            TGraph* graph{(TGraph*)pGraphs.GetListOfGraphs()->At(i)};
            graph->AddPoint(*fTime, -1.);
            graph->AddPoint(stats.front().getTime0(), -1.);
          }

          mfpGraph.AddPoint(*fTime, -1.);
          mfpGraph.AddPoint(stats.front().getTime0(), -1.);
          kBGraph.AddPoint(*fTime, -1.);
          kBGraph.AddPoint(stats.front().getTime0(), -1.);
          TH1D speedH{};

          frame.clear();

          cnvs.SetCanvasSize(windowSize.x * 0.25, windowSize.y * 0.5);
          drawObj(pGraphs, 0., 0., "APL", drawLambdas[0]);
          drawObj(kBGraph, 0., 0.5, "APL", drawLambdas[1]);
          drawObj(speedH, 0.75, 0., "HIST", drawLambdas[2]);
          drawObj(mfpGraph, 0.75, 0.5, "APL", drawLambdas[3]);

          txtBox.setSize({windowSize.x * 0.5f, windowSize.y * 0.1f});
          txtBox.setPosition(windowSize.x * 0.25, windowSize.y * 0.9);
          VText.setPosition(windowSize.x * (0.25 + 0.025),
                            windowSize.y * (0.9 + 0.025));
          NText.setPosition(windowSize.x * (0.25 + 0.175),
                            windowSize.y * (0.9 + 0.025));
          TText.setPosition(windowSize.x * (0.25 + 0.325),
                            windowSize.y * (0.9 + 0.025));
          frame.draw(txtBox);
          frame.draw(VText);
          frame.draw(NText);
          frame.draw(TText);

          // insert paired renders or placeholders
          box.setPosition(windowSize.x * 0.25, 0.);
          size_t rIndex{0};
          while (*fTime + gDeltaT < stats.front().getTime0()) {
            *fTime += gDeltaT;
            timeText.setString("time = " + round2(*fTime));
            assert(isIntMultOf(*gTime - *fTime, gDeltaT));
            if (renders.size() > rIndex &&
                isNegligible(*fTime - renders[rIndex].second, gDeltaT)) {
              box.setScale(
                  gasSize.x / (float)renders[rIndex].first.getSize().x,
                  gasSize.y / (float)renders[rIndex].first.getSize().y);
              box.setTexture(renders[rIndex++].first);
              frame.draw(box);
              frame.draw(timeText);
              frame.display();
            } else {
              box.setScale(gasSize.x / (float)placeholder.getSize().x,
                           gasSize.y / (float)placeholder.getSize().y);
              box.setTexture(placeholder, true);
              frame.draw(box);
              frame.draw(timeText);
              frame.display();
            }
            frames.emplace_back(frame.getTexture());
          }
        }
        size_t rIndex{0};
        while (*fTime + gDeltaT < stats.back().getTime()) {
          assert(*fTime + gDeltaT >= stats.front().getTime0());
          auto s{std::upper_bound(stats.begin(), stats.end(), *fTime + gDeltaT,
                                  [gDeltaT](double value, TdStats const& stat) {
                                    return value < stat.getTime0();
                                  }) -
                 1};
          std::cerr << std::endl
                    << "Switching to s = " << s - stats.begin()
                    << ": fTime = " << *fTime << ", s.t0 = " << s->getTime0()
                    << " ";
          assert(s >= stats.begin());
          assert(s != stats.end());
          TdStats const& stat{*s};
          // make the graphs picture
          for (size_t k{0}; k < 7; ++k) {
            TGraph* graph{(TGraph*)pGraphs.GetListOfGraphs()->At(k)};
            if (k < 6) {
              graph->AddPoint(stat.getTime0(), stat.getPressure(Wall(k)));
              graph->AddPoint(stat.getTime(), stat.getPressure(Wall(k)));
            } else {
              graph->AddPoint(stat.getTime0(), stat.getPressure());
              graph->AddPoint(stat.getTime(), stat.getPressure());
            }
          }
          mfpGraph.AddPoint(stat.getTime0(), stat.getMeanFreePath());
          mfpGraph.AddPoint(stat.getTime(), stat.getMeanFreePath());
          kBGraph.AddPoint(stat.getTime0(),
                           stat.getPressure() * stat.getVolume() /
                               (stat.getNParticles() * stat.getTemp()));
          kBGraph.AddPoint(stat.getTime(),
                           stat.getPressure() * stat.getVolume() /
                               (stat.getNParticles() * stat.getTemp()));
          /*
          std::cerr << "Added kB measurement to graph from starting data: \n"
          << "Pressure = " << stat.getPressure()
          << "\nVolume = " << stat.getVolume()
          << "\nTemperature = " << stat.getTemp()
          << "\nNParticles = " << stat.getNParticles() << std::endl;
          */
          TH1D speedH;
          speedH = stat.getSpeedH();

          if (fitLambda) {
            fitLambda(speedH, opt);
          }

          frame.clear();

          cnvs.SetCanvasSize(windowSize.x * 0.25, windowSize.y * 0.5);
          drawObj(pGraphs, 0., 0., "APL", drawLambdas[0]);
          drawObj(kBGraph, 0., 0.5, "APL", drawLambdas[1]);
          drawObj(speedH, 0.75, 0., "HIST", drawLambdas[2]);
          drawObj(mfpGraph, 0.75, 0.5, "APL", drawLambdas[3]);

          txtBox.setSize({windowSize.x * 0.5f, windowSize.y * 0.1f});
          txtBox.setPosition(windowSize.x * 0.25, windowSize.y * 0.9);
          VText.setPosition(windowSize.x * (0.25 + 0.025),
                            windowSize.y * (0.9 + 0.025));
          NText.setPosition(windowSize.x * (0.25 + 0.175),
                            windowSize.y * (0.9 + 0.025));
          TText.setPosition(windowSize.x * (0.25 + 0.325),
                            windowSize.y * (0.9 + 0.025));
          frame.draw(txtBox);
          frame.draw(VText);
          frame.draw(NText);
          frame.draw(TText);

          box.setPosition(windowSize.x * 0.25, 0.);
          while (*fTime + gDeltaT < stat.getTime()) {
            *fTime += gDeltaT;
            timeText.setString("time = " + round2(*fTime));
            assert(isIntMultOf(*gTime - *fTime, gDeltaT));
            if (renders.size() > rIndex &&
                isNegligible(*fTime - renders[rIndex].second, gDeltaT)) {
              box.setScale(
                  gasSize.x / (float)renders[rIndex].first.getSize().x,
                  gasSize.y / (float)renders[rIndex].first.getSize().y);
              box.setTexture(renders[(rIndex++)].first);
              frame.draw(box);
              frame.draw(timeText);
              frame.display();
            } else {
              box.setScale(gasSize.x / (float)placeholder.getSize().x,
                           gasSize.y / (float)placeholder.getSize().y);
              box.setTexture(placeholder, true);
              frame.draw(box);
              frame.draw(timeText);
              frame.display();
            }
            frames.emplace_back(frame.getTexture());
            std::cerr << rIndex << " ";
          }
        }  // while (fTime_ + gDeltaT_ < stats.back().getTime())
      } else if (renders.size()) {
        std::cerr << "Found renders size = " << renders.size()
                  << " they so fucking loud." << std::endl;
        for (size_t i{0}; i < 7; ++i) {
          TGraph* graph{(TGraph*)pGraphs.GetListOfGraphs()->At(i)};
          graph->AddPoint(*fTime, -1.);
          graph->AddPoint(*gTime, -1.);
        }

        mfpGraph.AddPoint(*fTime, -1.);
        mfpGraph.AddPoint(*gTime, -1.);
        kBGraph.AddPoint(*fTime, -1.);
        kBGraph.AddPoint(*gTime, -1.);
        TH1D speedH{};

        frame.clear();

        cnvs.SetCanvasSize(windowSize.x * 0.25, windowSize.y * 0.5);
        drawObj(pGraphs, 0., 0., "APL", drawLambdas[0]);
        drawObj(kBGraph, 0., 0.5, "APL", drawLambdas[1]);
        drawObj(speedH, 0.75, 0., "HIST", drawLambdas[2]);
        drawObj(mfpGraph, 0.75, 0.5, "APL", drawLambdas[3]);

        txtBox.setSize({windowSize.x * 0.5f, windowSize.y * 0.1f});
        txtBox.setPosition(windowSize.x * 0.25, windowSize.y * 0.9);
        VText.setPosition(windowSize.x * (0.25 + 0.025),
                          windowSize.y * (0.9 + 0.025));
        NText.setPosition(windowSize.x * (0.25 + 0.175),
                          windowSize.y * (0.9 + 0.025));
        TText.setPosition(windowSize.x * (0.25 + 0.325),
                          windowSize.y * (0.9 + 0.025));
        frame.draw(txtBox);
        frame.draw(VText);
        frame.draw(NText);
        frame.draw(TText);

        box.setPosition(windowSize.x * 0.25, 0.);
        size_t rIndex{0};
        while (*fTime + gDeltaT < gTime ||
               isNegligible(*fTime + gDeltaT - *gTime, gDeltaT)) {
          *fTime += gDeltaT;
          timeText.setString("time = " + round2(*fTime));
          assert(isIntMultOf(*gTime - *fTime, gDeltaT));
          if (renders.size() > rIndex &&
              isNegligible(*fTime - renders[rIndex].second, gDeltaT)) {
            box.setScale(gasSize.x / (float)renders[rIndex].first.getSize().x,
                         gasSize.y / (float)renders[rIndex].first.getSize().y);
            box.setTexture(renders[rIndex++].first);
            frame.draw(box);
            frame.draw(timeText);
            frame.display();
          } else {
            box.setScale(gasSize.x / (float)placeholder.getSize().x,
                         gasSize.y / (float)placeholder.getSize().y);
            box.setTexture(placeholder, true);
            frame.draw(box);
            frame.draw(timeText);
            frame.display();
          }
          frames.emplace_back(frame.getTexture());
        }
      }  // else if renders.size()
      renders.clear();
      trnsfrImg->Delete();
      break;
    }  // case scope end
    default:
      throw std::invalid_argument("Invalid video option provided.");
  }

  std::cerr << "\nDone, returning." << std::endl;
  return frames;
}

}  // namespace GS
