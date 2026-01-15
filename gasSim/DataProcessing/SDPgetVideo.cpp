#include "SimDataPipeline.hpp"

#include "DataProcessing/TdStats.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Window/Context.hpp>
#include <SFML/Config.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/System/String.hpp>
#include <SFML/System/Vector2.hpp>

#include <TH1.h>
#include <RtypesCore.h>
#include <TCanvas.h>
#include <TGraph.h>
#include <TImage.h>
#include <TMultiGraph.h>

#include <iomanip>
#include <sstream>
#include <TList.h>
#include <TObject.h>
#include <assert.h>
#include <bits/chrono.h>
#include <stddef.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <condition_variable>
#include <deque>
#include <functional>
#include <iterator>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// Hi, my name's Liam, I am the person who wrote this,
// I just wanted to say that I'm sorry for everything

namespace GS {

enum class Wall;

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

    pixels[base + 0] = static_cast<sf::Uint8>((rgba >> 24) & 0xFF);  // R
    pixels[base + 1] = static_cast<sf::Uint8>((rgba >> 16) & 0xFF);  // G
    pixels[base + 2] = static_cast<sf::Uint8>((rgba >> 8) & 0xFF);   // B
    pixels[base + 3] = static_cast<sf::Uint8>((rgba >> 0) & 0xFF);   // A
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
  ss << std::scientific << std::setprecision(static_cast<int>(nDigits)) << std::uppercase << x;
  return ss.str();
}

std::vector<sf::Texture> GS::SimDataPipeline::getVideo(
    VideoOpts opt, sf::Vector2u windowSize, sf::Texture const& placeholder,
    TList& outputGraphs, bool emptyStats,
    std::function<void(TH1D&, VideoOpts)> fitLambda,
    std::array<std::function<void()>, 4> drawLambdas) {
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

  if (dynamic_cast<TMultiGraph*>(outputGraphs.At(0))->GetListOfGraphs()->GetSize() != 7) {
    throw std::invalid_argument(
        "getVideo error: provided multigraph number of graphs != 7");
  }

  if (font.getInfo().family.empty()) {
    throw std::runtime_error("getVideo error: called without setting font");
  }

  std::deque<std::pair<sf::Texture, double>> rendersL{};
  std::vector<TdStats> statsL{};
  std::optional<double> gTimeL;
  double gDeltaTL;

  auto getRendersT0 = [&](auto const& couples) {
    if (couples.size()) {
      assert(gTimeL.has_value());
      return couples[0].second;
    } else if (gTimeL.has_value()) {
      return *gTimeL;
    } else {
      throw std::logic_error(
          "getVideo error: tried to get render time for output with no time "
          "set");
    }
  };

  // extraction of renders and/or stats clusters compatible with
  // processing algorithm
  // fTime initialization, either through gTime or through statsL[0]
  switch (opt) {
    case VideoOpts::justGas: {  // case scope
      {                         // lock scope
				sf::Context c;
        std::lock_guard<std::mutex> gTimeGuard{gTimeMtx};
        std::lock_guard<std::mutex> rGuard{rendersMtx};
        gTimeL = gTime;
        if (!gTimeL.has_value()) {
          return {};
        }
        gDeltaTL = gDeltaT.load();
        if (renders.size()) {
          rendersL = std::move(renders);
          renders.clear();
        } else {
          return {};
        }
      }  // end of lock scope
      if ((!fTime.has_value() || !isIntMultOf(*gTimeL - *fTime, gDeltaTL))) {
        fTime = getRendersT0(rendersL) - gDeltaTL;
      }
      break;
    }  // end of case scope
    case VideoOpts::justStats: {  // lock/case scope
      std::lock_guard<std::mutex> gTimeGuard{gTimeMtx};
      std::lock_guard<std::mutex> sGuard{statsMtx};
      gTimeL = gTime;
      gDeltaTL = gDeltaT.load();
      if (stats.size()) {
        // setting fTime_
        if (!fTime.has_value()) {
          if (gTimeL.has_value()) {
            fTime = *gTimeL +
                    gDeltaTL * std::floor((stats[0].getTime0() - *gTimeL) /
                                         gDeltaTL);
          } else {
            fTime = stats.front().getTime0() - gDeltaTL;
          }
        }
        if (stats.back().getTime() >= *fTime + gDeltaTL) {
          // find first where the next frame's time is before it
          // -> the one before it necessarily "contains" the frame time
          // this pattern used in the rest of this file
          auto sStartI{std::upper_bound(stats.begin(), stats.end(),
                                        *fTime + gDeltaTL,
                                        [](double value, TdStats const& s) {
                                          return value < s.getTime0();
                                        }) -
                       1};
          if (sStartI < stats.begin()) {
            sStartI = stats.begin();
          }
          if (emptyStats) {
            statsL.insert(statsL.end(), std::make_move_iterator(sStartI),
                         std::make_move_iterator(stats.end()));
            stats.clear();
          } else {
            statsL = std::vector<TdStats>{sStartI, stats.end()};
          }
        } else {
          return {};
        }
      } else {
        return {};
      }
      break;
    }  // end of lock/case scope
    case VideoOpts::all:
    case VideoOpts::gasPlusCoords: {  // locks/case scope
			sf::Context c;
      std::unique_lock<std::mutex> resultsLock{outputMtx};
      outputCv.wait_for(resultsLock, std::chrono::milliseconds(50),
                        [this]() { return addedResults.load(); });
      std::lock_guard<std::mutex> gTimeGuard{gTimeMtx};
      std::lock_guard<std::mutex> rGuard{rendersMtx};
      std::lock_guard<std::mutex> sGuard{statsMtx};
      gTimeL = gTime;
      gDeltaTL = gDeltaT.load();
      if (!gTimeL.has_value()) {
        return {};
      }
      if (renders.size() || stats.size()) {
        //  dealing with fTime_
        if (!fTime.has_value() || !isIntMultOf(*gTimeL - *fTime, gDeltaTL)) {
          if (!stats.size()) {
            fTime = getRendersT0(renders) - gDeltaTL;
          } else if (!renders.size()) {
            fTime =
                *gTimeL +
                gDeltaTL * std::floor((stats.front().getTime0() - *gTimeL) /
                                     gDeltaTL);
            assert(isIntMultOf(*fTime - *gTimeL, gDeltaTL));
          } else {
            if (getRendersT0(renders) > stats.front().getTime0()) {
              fTime = getRendersT0(renders) +
                      gDeltaTL * std::floor((stats[0].getTime0() -
                                            getRendersT0(renders)) /
                                           gDeltaTL);
            } else {
              fTime = getRendersT0(renders) - gDeltaTL;
            }
          }
        }

        //  extracting algorithm-compatible vector segments
        if (stats.size()) {
          auto sStartI{std::upper_bound(stats.begin(), stats.end(),
                                        *fTime + gDeltaTL,
                                        [](double value, TdStats const& s) {
                                          return value < s.getTime0();
                                        }) -
                       1};
          if (sStartI < stats.begin()) {
            sStartI = stats.begin();
          }
          if (sStartI != stats.end()) {
            auto gStartI{std::upper_bound(renders.begin(),
                                          renders.end(), *fTime + gDeltaTL,
                                          [](double value, auto const& render) {
                                            return value <= render.second;
                                          })};
            assert(isIntMultOf(*gTimeL - *fTime, gDeltaTL));
            auto gEndI{std::upper_bound(renders.begin(),
                                        renders.end(),
                                        stats.back().getTime(),
                                        [](double value, auto const& render) {
                                          return value < render.second;
                                        })};
            if (emptyStats) {
              statsL.insert(statsL.end(), std::make_move_iterator(sStartI),
                           std::make_move_iterator(stats.end()));
              stats.clear();
            } else {
              statsL = std::vector(sStartI, stats.end());
            }
            assert(gStartI <= gEndI);
            rendersL.insert(rendersL.begin(), std::make_move_iterator(gStartI),
                           std::make_move_iterator(gEndI));
            renders.clear();
          }
          if (!statsL.size() && !rendersL.size()) {
            return {};
          }
        } else {
          auto gStartI{std::upper_bound(renders.begin(),
                                        renders.end(), *fTime + gDeltaTL,
                                        [](double value, auto const& render) {
                                          return value <= render.second;
                                        })};
          // same as above
          rendersL.insert(rendersL.begin(), std::make_move_iterator(gStartI),
                         std::make_move_iterator(renders.end()));
          renders.clear();
        }
      } else {
        return {};
      }
      addedResults.store(false);
      resultsLock.unlock();
      break;
    }  // end of locks/case scope
    default:
      throw std::invalid_argument("Invalid video option provided.");
  }

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
  timeText.setPosition(static_cast<float>(windowSize.x) * 0.01f, static_cast<float>(windowSize.y) * 0.01f);
  // graphs
  TMultiGraph& pGraphs{*dynamic_cast<TMultiGraph*>(outputGraphs.At(0))};
  TGraph& kBGraph{*dynamic_cast<TGraph*>(outputGraphs.At(1))};
  TGraph& mfpGraph{*dynamic_cast<TGraph*>(outputGraphs.At(2))};
  TCanvas cnvs{};
  // image stuff
  TImage* trnsfrImg = TImage::Create();
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
    trnsfrImg->FromPad(&cnvs);
    auto* buf{trnsfrImg->GetRgbaArray()};
    if (!buf) {
      throw std::runtime_error("Failed to retrieve image RGBA array, aborting");
    }
    RGBA32toRGBA8(buf, trnsfrImg->GetWidth(),
                  trnsfrImg->GetHeight(), pixels);
		delete[] buf;
    auxTxtr.update(pixels.data());
    auxSprt.setTexture(auxTxtr, true);
    auxSprt.setPosition(static_cast<float>(windowSize.x) * static_cast<float>(percPosX), static_cast<float>(windowSize.y) * static_cast<float>(percPosY));
    frame.draw(auxSprt);
  }};

  {  // charSize
    unsigned charSize{static_cast<unsigned>(windowSize.y *
                    (opt == VideoOpts::gasPlusCoords ? 0.5 / 9. : 0.05))};
    TText.setCharacterSize(charSize);
    VText.setCharacterSize(charSize);
    NText.setCharacterSize(charSize);
  }
  // definitions as per available data
  switch (opt) {
    case VideoOpts::gasPlusCoords:
    case VideoOpts::all:
    case VideoOpts::justStats:
      if (statsL.size()) {
        Temp << std::fixed << std::setprecision(2) << statsL[0].getTemp();
        TText = {"T = " + Temp.str() + "K", font, 18};
        TText.setFont(font);
        TText.setFillColor(sf::Color::Black);
        VText = {"V = " + scientificNotation(statsL[0].getVolume()) + "m^3",
                 font, 18};
        VText.setFont(font);
        VText.setFillColor(sf::Color::Black);
        Num << std::fixed << std::setprecision(2) << statsL[0].getNParticles();
        NText = {" N = " + Num.str(), font, 18};
        NText.setFont(font);
        NText.setFillColor(sf::Color::Black);
      }
      break;
    case VideoOpts::justGas:
      break;
    default:
      throw std::invalid_argument("Invalid video option provided.");
  }

  // processing
  frame.create(windowSize.x, windowSize.y);
  switch (opt) {
    case VideoOpts::justGas: {
      if (rendersL.size()) {
        assert(isIntMultOf(*fTime - *gTimeL, gDeltaTL));
        assert(fTime < getRendersT0(rendersL) || isNegligible(*fTime - getRendersT0(rendersL), gDeltaTL));
        box.setPosition(0, 0);
        size_t rIndex{0};
        while (*fTime + gDeltaTL < gTimeL ||
               isNegligible(*fTime + gDeltaTL - *gTimeL, gDeltaTL)) {
          *fTime += gDeltaTL;
          timeText.setString("time = " + round2(*fTime));
          assert(isIntMultOf(*gTimeL - *fTime, gDeltaTL));
          if (rendersL.size() > rIndex &&
              isNegligible(*fTime - rendersL[rIndex].second, gDeltaTL)) {
            box.setScale(
                static_cast<float>(windowSize.x) / static_cast<float>(rendersL[rIndex].first.getSize().x),
                static_cast<float>(windowSize.y) / static_cast<float>(rendersL[rIndex].first.getSize().y));
            box.setTexture(rendersL[rIndex++].first);
            frame.draw(box);
            frame.draw(timeText);
            frame.display();
          } else {
            box.setScale(static_cast<float>(windowSize.x) / static_cast<float>(placeholder.getSize().x),
                         static_cast<float>(windowSize.y) / static_cast<float>(placeholder.getSize().y));
            box.setTexture(placeholder, true);
            frame.draw(box);
            frame.draw(timeText);
            frame.display();
          }
          frames.emplace_back(frame.getTexture());
        }
        rendersL.clear();
      }
      break;
    }
    case VideoOpts::justStats:
      if (statsL.size()) {
        if (*fTime + gDeltaTL <
            statsL.front()
                .getTime0()) {  // insert empty data and use placeholder render
          auto& stat{statsL.front()};
          for (size_t i{0}; i < 7; ++i) {
            TGraph* graph{dynamic_cast<TGraph*>(pGraphs.GetListOfGraphs()->At(static_cast<int>(i)))};
            graph->AddPoint(*fTime, -1.);
            graph->AddPoint(stat.getTime0(), -1.);
          }

          mfpGraph.AddPoint(*fTime, -1.);
          mfpGraph.AddPoint(stat.getTime0(), -1.);
          kBGraph.AddPoint(*fTime, -1.);
          kBGraph.AddPoint(stat.getTime0(), -1.);

          frame.clear();

          cnvs.SetCanvasSize(static_cast<unsigned>(static_cast<float>(windowSize.x) * 0.5f), static_cast<unsigned>(static_cast<float>(windowSize.y) * 0.45f));
          ;
          auxTxtr.create(static_cast<unsigned>(static_cast<float>(windowSize.x) * 0.5f), static_cast<unsigned>(static_cast<float>(windowSize.y) * 0.45f));

          drawObj(pGraphs, 0., 0., "APL", drawLambdas[0]);
          drawObj(kBGraph, 0., windowSize.y * 0.55, "APL", drawLambdas[1]);

          cnvs.SetCanvasSize(static_cast<unsigned>(static_cast<float>(windowSize.x) * 0.5f), static_cast<unsigned>(static_cast<float>(windowSize.y) * 0.5f));
          auxTxtr.create(static_cast<unsigned>(static_cast<float>(windowSize.x) * 0.5f), static_cast<unsigned>(static_cast<float>(windowSize.y) * 0.5f));

          drawObj(mfpGraph, 0.5, 0.5, "APL", drawLambdas[3]);

          txtBox.setSize({static_cast<float>(windowSize.x) * 0.5f, static_cast<float>(windowSize.y) * 0.1f});
          txtBox.setPosition(0.f, static_cast<float>(windowSize.y) * 0.45f);
          VText.setPosition(static_cast<float>(windowSize.x) * (0.f + 0.025f),
                            static_cast<float>(windowSize.y) * (0.45f + 0.025f));
          NText.setPosition(static_cast<float>(windowSize.x) * (0.f + 0.175f),
                            static_cast<float>(windowSize.y) * (0.45f + 0.025f));
          TText.setPosition(static_cast<float>(windowSize.x) * (0.f + 0.325f),
                            static_cast<float>(windowSize.y) * (0.45f + 0.025f));
          frame.draw(txtBox);
          frame.draw(VText);
          frame.draw(NText);
          frame.draw(TText);

          box.setScale(static_cast<float>(windowSize.x) * 0.5f / static_cast<float>(placeholder.getSize().x),
                       static_cast<float>(windowSize.y) * 0.5f / static_cast<float>(placeholder.getSize().y));
          box.setPosition(static_cast<float>(windowSize.x) * 0.5f, static_cast<float>(windowSize.y));
          box.setTexture(placeholder, true);
          frame.draw(box);

          frame.display();

          while (*fTime + gDeltaTL < stat.getTime0()) {
            *fTime += gDeltaTL;
            frames.emplace_back(frame.getTexture());
          }
        }

        for (TdStats const& stat : statsL) {
          for (size_t i{0}; i < 7; ++i) {
            TGraph* graph{dynamic_cast<TGraph*>(pGraphs.GetListOfGraphs()->At(static_cast<int>(i)))};
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
                               (static_cast<double>(stat.getNParticles()) * stat.getTemp()));
          kBGraph.AddPoint(stat.getTime(),
                           stat.getPressure() * stat.getVolume() /
                               (static_cast<double>(stat.getNParticles()) * stat.getTemp()));

          TH1D speedH{stat.getSpeedH()};

          if (fitLambda) {
            fitLambda(speedH, opt);
          }

          frame.clear();

          cnvs.SetCanvasSize(static_cast<unsigned>(static_cast<float>(windowSize.x) * 0.5f), static_cast<unsigned>(static_cast<float>(windowSize.y) * 0.45f));
          auxTxtr.create(static_cast<unsigned>(static_cast<float>(windowSize.x) * 0.5f), static_cast<unsigned>(static_cast<float>(windowSize.y) * 0.45f));

          drawObj(pGraphs, 0., 0., "APL", drawLambdas[0]);
          drawObj(kBGraph, 0., 0.55, "APL", drawLambdas[1]);

          cnvs.SetCanvasSize(static_cast<unsigned>(static_cast<float>(windowSize.x) * 0.5f), static_cast<unsigned>(static_cast<float>(windowSize.y) * 0.5));
          auxTxtr.create(static_cast<unsigned>(static_cast<float>(windowSize.x) * 0.5f), static_cast<unsigned>(static_cast<float>(windowSize.y) * 0.5));

          drawObj(speedH, 0.5, 0., "HIST", drawLambdas[2]);
          drawObj(mfpGraph, 0.5, 0.5, "APL", drawLambdas[3]);

          txtBox.setSize({static_cast<float>(windowSize.x) * 0.5f, static_cast<float>(windowSize.y) * 0.1f});
          txtBox.setPosition(0.f, static_cast<float>(windowSize.y) * 0.45f);
          VText.setPosition(static_cast<float>(windowSize.x) * (0.f + 0.025f),
                            static_cast<float>(windowSize.y) * (0.45f + 0.025f));
          NText.setPosition(static_cast<float>(windowSize.x) * (0.f + 0.175f),
                            static_cast<float>(windowSize.y) * (0.45f + 0.025f));
          TText.setPosition(static_cast<float>(windowSize.x) * (0.f + 0.325f),
                            static_cast<float>(windowSize.y) * (0.45f + 0.025f));
          frame.draw(txtBox);
          frame.draw(VText);
          frame.draw(NText);
          frame.draw(TText);

          frame.display();

          while (*fTime + gDeltaTL < stat.getTime()) {
            *fTime += gDeltaTL;
            frames.emplace_back(frame.getTexture());
          }
        }
        statsL.clear();
      }
      break;
    case VideoOpts::gasPlusCoords:
      if (rendersL.size()) {
        assert(gTimeL == rendersL.back().second);
				assert(fTime < getRendersT0(rendersL) || isNegligible(*fTime - getRendersT0(rendersL), gDeltaTL));
      }
      {  // case scope
        timeText.setPosition(static_cast<float>(windowSize.x) * 0.51f, static_cast<float>(windowSize.y) * 0.01f);
        sf::Vector2u gasSize{static_cast<unsigned>(windowSize.x * 0.5),
                             static_cast<unsigned>(windowSize.y * 0.9)};
        if (statsL.size()) {
          auxTxtr.create(static_cast<unsigned>(static_cast<float>(windowSize.x) * 0.5f), static_cast<unsigned>(static_cast<float>(windowSize.y) * 0.5f));
          if (*fTime + gDeltaTL < statsL.front().getTime0()) {
            for (size_t i{0}; i < 7; ++i) {
              TGraph* graph{dynamic_cast<TGraph*>(pGraphs.GetListOfGraphs()->At(static_cast<int>(i)))};
              graph->AddPoint(*fTime, -1.);
              graph->AddPoint(statsL.front().getTime0(), -1.);
            }

            kBGraph.AddPoint(*fTime, -1.);
            kBGraph.AddPoint(statsL.front().getTime0(), -1.);

            frame.clear();

            cnvs.SetCanvasSize(static_cast<unsigned>(static_cast<float>(windowSize.x) * 0.5f), static_cast<unsigned>(static_cast<float>(windowSize.y) * 0.5f));
            drawObj(pGraphs, 0., 0., "APL", drawLambdas[0]);
            drawObj(kBGraph, 0., 0.5, "APL", drawLambdas[1]);

            txtBox.setSize({static_cast<float>(windowSize.x) * 0.5f, static_cast<float>(windowSize.y) * 1.f / 9.f});
            txtBox.setPosition(static_cast<float>(windowSize.x) * 0.5f, static_cast<float>(windowSize.y) * 8.f / 9.f);
            VText.setPosition(static_cast<float>(windowSize.x) * (0.5f + 0.025f),
                              static_cast<float>(windowSize.y) * (8.f / 9.f + 1.f / 36.f));
            NText.setPosition(static_cast<float>(windowSize.x) * (0.5f + 0.175f),
                              static_cast<float>(windowSize.y) * (8.f / 9.f + 1.f / 36.f));
            TText.setPosition(static_cast<float>(windowSize.x) * (0.5f + 0.325f),
                              static_cast<float>(windowSize.y) * (8.f / 9.f + 1.f / 36.f));
            frame.draw(txtBox);
            frame.draw(VText);
            frame.draw(NText);
            frame.draw(TText);
            // insert paired renders or placeholders
            box.setPosition(static_cast<float>(windowSize.x) * 0.5f, 0.f);
            size_t rIndex{0};
            while (*fTime + gDeltaTL < statsL.front().getTime0()) {
              *fTime += gDeltaTL;
              timeText.setString("time = " + round2(*fTime));
              assert(isIntMultOf(*gTimeL - *fTime, gDeltaTL));
              if (rendersL.size() > rIndex &&
                  isNegligible(*fTime - rendersL[rIndex].second, gDeltaTL)) {
                box.setScale(
                    static_cast<float>(gasSize.x) / static_cast<float>(rendersL[rIndex].first.getSize().x),
                    static_cast<float>(gasSize.y) / static_cast<float>(rendersL[rIndex].first.getSize().y));
                box.setTexture(rendersL[rIndex++].first);
                frame.draw(box);
                frame.draw(timeText);
                frame.display();
              } else {
                box.setScale(static_cast<float>(gasSize.x) / static_cast<float>(placeholder.getSize().x),
                             static_cast<float>(gasSize.y) / static_cast<float>(placeholder.getSize().y));
                box.setTexture(placeholder, true);
                frame.draw(box);
                frame.draw(timeText);
                frame.display();
              }
              frames.emplace_back(frame.getTexture());
            }
          }
          assert(*fTime + gDeltaTL >= statsL.front().getTime0());
          size_t rIndex{0};
          while (*fTime + gDeltaTL < statsL.back().getTime()) {
            auto s{std::upper_bound(statsL.begin(), statsL.end(),
                                    *fTime + gDeltaTL,
                                    [](double value, TdStats const& stat) {
                                      return value < stat.getTime0();
                                    }) -
                   1};
            assert(s >= statsL.begin());
            assert(s < statsL.end());
            TdStats const& stat{*s};
            // make the graphs picture
            for (size_t i{0}; i < 7; ++i) {
              TGraph* graph{dynamic_cast<TGraph*>(pGraphs.GetListOfGraphs()->At(static_cast<int>(i)))};
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
                                 (static_cast<double>(stat.getNParticles()) * stat.getTemp()));
            kBGraph.AddPoint(stat.getTime(),
                             stat.getPressure() * stat.getVolume() /
                                 (static_cast<double>(stat.getNParticles()) * stat.getTemp()));

            TH1D h{};

            if (fitLambda) {
              fitLambda(h, opt);
            }

            frame.clear();

            cnvs.SetCanvasSize(static_cast<unsigned>(static_cast<float>(windowSize.x) * 0.5f), static_cast<unsigned>(static_cast<float>(windowSize.y) * 0.5f));
            drawObj(pGraphs, 0., 0., "APL", drawLambdas[0]);
            drawObj(kBGraph, 0., 0.5, "APL", drawLambdas[1]);

            txtBox.setSize({static_cast<float>(windowSize.x) * 0.5f, static_cast<float>(windowSize.y) * 1.f / 9.f});
            txtBox.setPosition(static_cast<float>(windowSize.x) * 0.5f, static_cast<float>(windowSize.y) * 8.f / 9.f);
            VText.setPosition(static_cast<float>(windowSize.x) * (0.5f + 0.025f),
                              static_cast<float>(windowSize.y) * (8.f / 9.f + 1.f / 36.f));
            NText.setPosition(static_cast<float>(windowSize.x) * (0.5f + 0.175f),
                              static_cast<float>(windowSize.y) * (8.f / 9.f + 1.f / 36.f));
            TText.setPosition(static_cast<float>(windowSize.x) * (0.5f + 0.325f),
                              static_cast<float>(windowSize.y) * (8.f / 9.f + 1.f / 36.f));
            frame.draw(txtBox);
            frame.draw(VText);
            frame.draw(NText);
            frame.draw(TText);
            // insert paired renders or placeholders
            box.setPosition(static_cast<float>(windowSize.x) * 0.5f, 0.f);
            while (*fTime + gDeltaTL < stat.getTime()) {
              *fTime += gDeltaTL;
              timeText.setString("time = " + round2(*fTime));
              assert(isIntMultOf(*gTimeL - *fTime, gDeltaTL));
              if (rendersL.size() > rIndex &&
                  isNegligible(*fTime - rendersL[rIndex].second, gDeltaTL)) {
                box.setScale(
                    static_cast<float>(gasSize.x) / static_cast<float>(rendersL[rIndex].first.getSize().x),
                    static_cast<float>(gasSize.y) / static_cast<float>(rendersL[rIndex].first.getSize().y));
                box.setTexture(rendersL[rIndex++].first);
                frame.draw(box);
                frame.draw(timeText);
                frame.display();
              } else {
                box.setScale(static_cast<float>(gasSize.x) / static_cast<float>(placeholder.getSize().x),
                             static_cast<float>(gasSize.y) / static_cast<float>(placeholder.getSize().y));
                box.setTexture(placeholder, true);
                frame.draw(box);
                frame.draw(timeText);
                frame.display();
              }
              frames.emplace_back(frame.getTexture());
            }
          }  // while (fTime_ + gDeltaTL < statsL.back().getTime())
        } else if (rendersL.size()) {
          assert(rendersL.back().second == gTimeL);
          if (*fTime + gDeltaTL < gTimeL ||
              isNegligible(*fTime + gDeltaTL - *gTimeL, gDeltaTL)) {
            for (size_t i{0}; i < 7; ++i) {
              TGraph* graph{dynamic_cast<TGraph*>(pGraphs.GetListOfGraphs()->At(static_cast<int>(i)))};
              graph->AddPoint(*fTime, -1.);
              graph->AddPoint(*gTimeL, -1.);
            }

            kBGraph.AddPoint(*fTime, -1.);
            kBGraph.AddPoint(*gTimeL, -1.);

            frame.clear();

            cnvs.SetCanvasSize(static_cast<unsigned>(static_cast<float>(windowSize.x) * 0.5f), static_cast<unsigned>(static_cast<float>(windowSize.y) * 0.5f));
            drawObj(pGraphs, 0., 0., "APL", drawLambdas[0]);
            drawObj(kBGraph, 0., 0.5, "APL", drawLambdas[1]);

            txtBox.setSize({static_cast<float>(windowSize.x) * 0.5f, static_cast<float>(windowSize.y) * 1.f / 9.f});
            txtBox.setPosition(static_cast<float>(windowSize.x) * 0.5f, static_cast<float>(windowSize.y) * 8.f / 9.f);
            VText.setPosition(static_cast<float>(windowSize.x) * (0.5f + 0.025f),
                              static_cast<float>(windowSize.y) * (8.f / 9.f + 1.f / 36.f));
            NText.setPosition(static_cast<float>(windowSize.x) * (0.5f + 0.175f),
                              static_cast<float>(windowSize.y) * (8.f / 9.f + 1.f / 36.f));
            TText.setPosition(static_cast<float>(windowSize.x) * (0.5f + 0.325f),
                              static_cast<float>(windowSize.y) * (8.f / 9.f + 1.f / 36.f));
            frame.draw(txtBox);
            frame.draw(VText);
            frame.draw(NText);
            frame.draw(TText);
            box.setPosition(static_cast<float>(windowSize.x) * 0.5f, 0.f);
            size_t rIndex{0};
            while (*fTime + gDeltaTL < gTimeL ||
                   isNegligible(*fTime + gDeltaTL - *gTimeL, gDeltaTL)) {
              *fTime += gDeltaTL;
              timeText.setString("time = " + round2(*fTime));
              assert(isIntMultOf(*gTimeL - *fTime, gDeltaTL));
              if (rendersL.size() > rIndex &&
                  isNegligible(*fTime - rendersL[rIndex].second, gDeltaTL)) {
                box.setScale(
                    static_cast<float>(gasSize.x) / static_cast<float>(rendersL[rIndex].first.getSize().x),
                    static_cast<float>(gasSize.y) / static_cast<float>(rendersL[rIndex].first.getSize().y));
                box.setTexture(rendersL[rIndex++].first);
                frame.draw(box);
                frame.draw(timeText);
                frame.display();
              } else {
                box.setScale(static_cast<float>(gasSize.x) / static_cast<float>(placeholder.getSize().x),
                             static_cast<float>(gasSize.y) / static_cast<float>(placeholder.getSize().y));
                box.setTexture(placeholder, true);
                frame.draw(box);
                frame.draw(timeText);
                frame.display();
              }
              frames.emplace_back(frame.getTexture());
            }
          }
        }  // else if rendersL.size()
        rendersL.clear();
        break;
      }  // case scope end
    case VideoOpts::all: {  // case scope
      if (rendersL.size()) {
        assert(fTime < getRendersT0(rendersL) || isNegligible(*fTime - getRendersT0(rendersL), gDeltaTL));
        assert(gTimeL == rendersL.back().second);
      }
      timeText.setPosition(static_cast<float>(windowSize.x) * 0.26f, static_cast<float>(windowSize.y) * 0.01f);
      sf::Vector2u gasSize{static_cast<unsigned>(windowSize.x * 0.5),
                           static_cast<unsigned>(windowSize.y * 0.9)};
      if (statsL.size()) {
        auxTxtr.create(static_cast<unsigned>(static_cast<float>(windowSize.x) * 0.25f), static_cast<unsigned>(static_cast<float>(windowSize.y) * 0.5f));
        if (*fTime + gDeltaTL < statsL.front().getTime0()) {
          for (size_t i{0}; i < 7; ++i) {
            TGraph* graph{dynamic_cast<TGraph*>(pGraphs.GetListOfGraphs()->At(static_cast<int>(i)))};
            graph->AddPoint(*fTime, -1.);
            graph->AddPoint(statsL.front().getTime0(), -1.);
          }

          mfpGraph.AddPoint(*fTime, -1.);
          mfpGraph.AddPoint(statsL.front().getTime0(), -1.);
          kBGraph.AddPoint(*fTime, -1.);
          kBGraph.AddPoint(statsL.front().getTime0(), -1.);
          TH1D speedH{};

          frame.clear();

          cnvs.SetCanvasSize(static_cast<unsigned>(static_cast<float>(windowSize.x) * 0.25f), static_cast<unsigned>(static_cast<float>(windowSize.y) * 0.5f));
          drawObj(pGraphs, 0., 0., "APL", drawLambdas[0]);
          drawObj(kBGraph, 0., 0.5, "APL", drawLambdas[1]);
          drawObj(speedH, 0.75, 0., "HIST", drawLambdas[2]);
          drawObj(mfpGraph, 0.75, 0.5, "APL", drawLambdas[3]);

          txtBox.setSize({static_cast<float>(windowSize.x) * 0.5f, static_cast<float>(windowSize.y) * 0.1f});
          txtBox.setPosition(static_cast<float>(windowSize.x) * 0.25f, static_cast<float>(windowSize.y) * 0.9f);
          VText.setPosition(static_cast<float>(windowSize.x) * (0.25f + 0.025f),
                            static_cast<float>(windowSize.y) * (0.9f + 0.025f));
          NText.setPosition(static_cast<float>(windowSize.x) * (0.25f + 0.175f),
                            static_cast<float>(windowSize.y) * (0.9f + 0.025f));
          TText.setPosition(static_cast<float>(windowSize.x) * (0.25f + 0.325f),
                            static_cast<float>(windowSize.y) * (0.9f + 0.025f));
          frame.draw(txtBox);
          frame.draw(VText);
          frame.draw(NText);
          frame.draw(TText);

          // insert paired renders or placeholders
          box.setPosition(static_cast<float>(windowSize.x) * 0.25f, 0.f);
          size_t rIndex{0};
          while (*fTime + gDeltaTL < statsL.front().getTime0()) {
            *fTime += gDeltaTL;
            timeText.setString("time = " + round2(*fTime));
            assert(isIntMultOf(*gTimeL - *fTime, gDeltaTL));
            if (rendersL.size() > rIndex &&
                isNegligible(*fTime - rendersL[rIndex].second, gDeltaTL)) {
              box.setScale(
                  static_cast<float>(gasSize.x) / static_cast<float>(rendersL[rIndex].first.getSize().x),
                  static_cast<float>(gasSize.y) / static_cast<float>(rendersL[rIndex].first.getSize().y));
              box.setTexture(rendersL[rIndex++].first);
              frame.draw(box);
              frame.draw(timeText);
              frame.display();
            } else {
              box.setScale(static_cast<float>(gasSize.x) / static_cast<float>(placeholder.getSize().x),
                           static_cast<float>(gasSize.y) / static_cast<float>(placeholder.getSize().y));
              box.setTexture(placeholder, true);
              frame.draw(box);
              frame.draw(timeText);
              frame.display();
            }
            frames.emplace_back(frame.getTexture());
          }
        }
        size_t rIndex{0};
        while (*fTime + gDeltaTL < statsL.back().getTime()) {
          assert(*fTime + gDeltaTL >= statsL.front().getTime0());
          auto s{std::upper_bound(statsL.begin(), statsL.end(), *fTime + gDeltaTL,
                                  [](double value, TdStats const& stat) {
                                    return value < stat.getTime0();
                                  }) -
                 1};
          assert(s >= statsL.begin());
          assert(s != statsL.end());
          TdStats const& stat{*s};
          // make the graphs picture
          for (size_t k{0}; k < 7; ++k) {
            TGraph* graph{dynamic_cast<TGraph*>(pGraphs.GetListOfGraphs()->At(static_cast<int>(k)))};
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
                               (static_cast<double>(stat.getNParticles()) * stat.getTemp()));
          kBGraph.AddPoint(stat.getTime(),
                           stat.getPressure() * stat.getVolume() /
                               (static_cast<double>(stat.getNParticles()) * stat.getTemp()));
          TH1D speedH;
          speedH = stat.getSpeedH();

          if (fitLambda) {
            fitLambda(speedH, opt);
          }

          frame.clear();

          cnvs.SetCanvasSize(static_cast<unsigned>(static_cast<float>(windowSize.x) * 0.25f), static_cast<unsigned>(static_cast<float>(windowSize.y) * 0.5f));
          drawObj(pGraphs, 0., 0., "APL", drawLambdas[0]);
          drawObj(kBGraph, 0., 0.5, "APL", drawLambdas[1]);
          drawObj(speedH, 0.75, 0., "HIST", drawLambdas[2]);
          drawObj(mfpGraph, 0.75, 0.5, "APL", drawLambdas[3]);

          txtBox.setSize({static_cast<float>(windowSize.x) * 0.5f, static_cast<float>(windowSize.y) * 0.1f});
          txtBox.setPosition(static_cast<float>(windowSize.x) * 0.25f, static_cast<float>(windowSize.y) * 0.9f);
          VText.setPosition(static_cast<float>(windowSize.x) * (0.25f + 0.025f),
                            static_cast<float>(windowSize.y) * (0.9f + 0.025f));
          NText.setPosition(static_cast<float>(windowSize.x) * (0.25f + 0.175f),
                            static_cast<float>(windowSize.y) * (0.9f + 0.025f));
          TText.setPosition(static_cast<float>(windowSize.x) * (0.25f + 0.325f),
                            static_cast<float>(windowSize.y) * (0.9f + 0.025f));
          frame.draw(txtBox);
          frame.draw(VText);
          frame.draw(NText);
          frame.draw(TText);

          box.setPosition(static_cast<float>(windowSize.x) * 0.25f, 0.f);
          while (*fTime + gDeltaTL < stat.getTime()) {
            *fTime += gDeltaTL;
            timeText.setString("time = " + round2(*fTime));
            assert(isIntMultOf(*gTimeL - *fTime, gDeltaTL));
            if (rendersL.size() > rIndex &&
                isNegligible(*fTime - rendersL[rIndex].second, gDeltaTL)) {
              box.setScale(
                  static_cast<float>(gasSize.x) / static_cast<float>(rendersL[rIndex].first.getSize().x),
                  static_cast<float>(gasSize.y) / static_cast<float>(rendersL[rIndex].first.getSize().y));
              box.setTexture(rendersL[(rIndex++)].first);
              frame.draw(box);
              frame.draw(timeText);
              frame.display();
            } else {
              box.setScale(static_cast<float>(gasSize.x) / static_cast<float>(placeholder.getSize().x),
                           static_cast<float>(gasSize.y) / static_cast<float>(placeholder.getSize().y));
              box.setTexture(placeholder, true);
              frame.draw(box);
              frame.draw(timeText);
              frame.display();
            }
            frames.emplace_back(frame.getTexture());
          }
        }  // while (fTime_ + gDeltaTL < statsL.back().getTime())
      } else if (rendersL.size()) {
        for (size_t i{0}; i < 7; ++i) {
          TGraph* graph{dynamic_cast<TGraph*>(pGraphs.GetListOfGraphs()->At(static_cast<int>(i)))};
          graph->AddPoint(*fTime, -1.);
          graph->AddPoint(*gTimeL, -1.);
        }

        mfpGraph.AddPoint(*fTime, -1.);
        mfpGraph.AddPoint(*gTimeL, -1.);
        kBGraph.AddPoint(*fTime, -1.);
        kBGraph.AddPoint(*gTimeL, -1.);
        TH1D speedH{};

        frame.clear();

        cnvs.SetCanvasSize(static_cast<unsigned>(static_cast<float>(windowSize.x) * 0.25f), static_cast<unsigned>(static_cast<float>(windowSize.y) * 0.5f));
        drawObj(pGraphs, 0., 0., "APL", drawLambdas[0]);
        drawObj(kBGraph, 0., 0.5, "APL", drawLambdas[1]);
        drawObj(speedH, 0.75, 0., "HIST", drawLambdas[2]);
        drawObj(mfpGraph, 0.75, 0.5, "APL", drawLambdas[3]);

        txtBox.setSize({static_cast<float>(windowSize.x) * 0.5f, static_cast<float>(windowSize.y) * 0.1f});
        txtBox.setPosition(static_cast<float>(windowSize.x) * 0.25f, static_cast<float>(windowSize.y) * 0.9f);
        VText.setPosition(static_cast<float>(windowSize.x) * (0.25f + 0.025f),
                          static_cast<float>(windowSize.y) * (0.9f + 0.025f));
        NText.setPosition(static_cast<float>(windowSize.x) * (0.25f + 0.175f),
                          static_cast<float>(windowSize.y) * (0.9f + 0.025f));
        TText.setPosition(static_cast<float>(windowSize.x) * (0.25f + 0.325f),
                          static_cast<float>(windowSize.y) * (0.9f + 0.025f));
        frame.draw(txtBox);
        frame.draw(VText);
        frame.draw(NText);
        frame.draw(TText);

        box.setPosition(static_cast<float>(windowSize.x) * 0.25f, 0.f);
        size_t rIndex{0};
        while (*fTime + gDeltaTL < gTimeL ||
               isNegligible(*fTime + gDeltaTL - *gTimeL, gDeltaTL)) {
          *fTime += gDeltaTL;
          timeText.setString("time = " + round2(*fTime));
          assert(isIntMultOf(*gTimeL - *fTime, gDeltaTL));
          if (rendersL.size() > rIndex &&
              isNegligible(*fTime - rendersL[rIndex].second, gDeltaTL)) {
            box.setScale(static_cast<float>(gasSize.x) / static_cast<float>(rendersL[rIndex].first.getSize().x),
                         static_cast<float>(gasSize.y) / static_cast<float>(rendersL[rIndex].first.getSize().y));
            box.setTexture(rendersL[rIndex++].first);
            frame.draw(box);
            frame.draw(timeText);
            frame.display();
          } else {
            box.setScale(static_cast<float>(gasSize.x) / static_cast<float>(placeholder.getSize().x),
                         static_cast<float>(gasSize.y) / static_cast<float>(placeholder.getSize().y));
            box.setTexture(placeholder, true);
            frame.draw(box);
            frame.draw(timeText);
            frame.display();
          }
          frames.emplace_back(frame.getTexture());
        }
      }  // else if renders.size()
      rendersL.clear();
      break;
    }  // case scope end
    default:
			delete trnsfrImg;
      throw std::invalid_argument("Invalid video option provided.");
  }

  delete trnsfrImg;
  return frames;
}

}  // namespace GS
