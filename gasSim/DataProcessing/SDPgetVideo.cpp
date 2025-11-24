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
#include <execution>
#include <iomanip>
#include <sstream>

#include "SimDataPipeline.hpp"

// Hi, my name's Liam, I was the person who wrote this, I just wanted to say
// that I'm sorry for everything

namespace GS {

bool isNegligible(double epsilon, double x);  // implemented in TdStats.cpp

bool isIntMultOf(double x, double dt) {
  return isNegligible(x - dt * std::round(x / dt), dt);
}

/*
void RGBA32ToRGBA8(const UInt_t* rgbaBffr, unsigned w, unsigned h,
std::vector<sf::Uint8>& pixels) { const size_t total = static_cast<size_t>(w) *
h; pixels.resize(total * 4);

    std::vector<size_t> indices(total);
    std::iota(indices.begin(), indices.end(), 0);

    std::for_each(std::execution::par, indices.begin(), indices.end(),
                  [&](size_t idx) {
        UInt_t rgba = rgbaBffr[idx];
        size_t base = idx * 4;
        pixels[base + 0] = static_cast<sf::Uint8>((rgba >> 24) & 0xFF); // R
        pixels[base + 1] = static_cast<sf::Uint8>((rgba >> 16) & 0xFF); // G
        pixels[base + 2] = static_cast<sf::Uint8>((rgba >> 8) & 0xFF);  // B
        pixels[base + 3] = static_cast<sf::Uint8>((rgba >> 0) & 0xFF); 	// A
    });
}
*/

// hopefully chatGPT cooked
void RGBA32toRGBA8(UInt_t const* rgbaBffr, size_t w, size_t h,
                   std::vector<sf::Uint8>& pixels) {
  size_t total{w * h};
  pixels.resize(total * 4);

  std::for_each(std::execution::par, size_t{0}, total, [&](size_t idx) {
    UInt_t rgba = rgbaBffr[idx];
    size_t base = idx * 4;

    pixels[base + 0] = (rgba >> 24) & 0xFF;  // R
    pixels[base + 1] = (rgba >> 16) & 0xFF;  // G
    pixels[base + 2] = (rgba >> 8) & 0xFF;   // B
    pixels[base + 3] = (rgba >> 0) & 0xFF;   // A
  });
}

std::vector<sf::Texture> GS::SimDataPipeline::getVideo(
    VideoOpts opt, sf::Vector2i windowSize, sf::Texture placeholder,
    TList& outputGraphs, bool emptyStats,
    std::function<void(TH1D&, VideoOpts)> fitLambda,
    std::array<std::function<void()>, 4> drawLambdas) {
  // static int nExec {0};
  // std::cerr << std::endl << "Started getVideo, call number " << ++nExec <<
  // std::endl;

  if ((opt == VideoOpts::justGas && windowSize.x < 400 && windowSize.y < 400) ||
      (opt == VideoOpts::justStats && windowSize.x < 600 &&
       windowSize.y < 600) ||
      (windowSize.x < 800 && windowSize.y < 600)) {
    throw std::invalid_argument("Provided window size is too small.");
  }

  if (!(outputGraphs.At(0)->IsA() == TMultiGraph::Class() &&
        outputGraphs.At(1)->IsA() == TGraph::Class() &&
        outputGraphs.At(2)->IsA() == TGraph::Class())) {
    throw std::invalid_argument("Provided graphs list with wrong object types");
  }

  if (((TMultiGraph*)outputGraphs.At(0))->GetListOfGraphs()->GetSize() != 7) {
    throw std::invalid_argument("Provided multigraph number of graphs != 7");
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
          "Tried to get render time for output with no time set.");
    }
  };

  // std::cerr << "Got to data extraction phase." << std::endl;
  //	// extraction of renders and/or stats clusters compatible with
  // processing algorithm
  // fTime_ initialization, either through gTime_ or through stats[0]
  switch (opt) {
    case VideoOpts::justGas: {  // case scope
      {                         // lock scope
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
      break;
    }  // end of case scope
    case VideoOpts::justStats: {  // lock/case scope
      std::lock_guard<std::mutex> sGuard{statsMtx};
      {  // locks scope 2
        std::lock_guard<std::mutex> gTimeGuard{gTimeMtx};
        gTime = this->gTime;
      }  // end of locks scope 2
      gDeltaT = this->gDeltaT.load();
      if (this->stats.size()) {
        // setting fTime_
        if (!fTime.has_value()) {
          if (gTime.has_value()) {
            fTime =
                *gTime +
                gDeltaT * std::floor((stats[0].getTime0() - *gTime) / gDeltaT);
          } else {
            fTime = this->stats.front().getTime0() - gDeltaT;
          }
        }
        if (this->stats.back().getTime() >= *fTime + gDeltaT) {
          auto sStartI{std::upper_bound(this->stats.begin(), this->stats.end(),
                                        *fTime + gDeltaT,
                                        [](double value, TdStats const& s) {
                                          return value <= s.getTime();
                                        })};
          if (emptyStats) {
            stats.insert(stats.end(), std::make_move_iterator(sStartI),
                         std::make_move_iterator(this->stats.end()));
            this->stats.clear();
          } else {
            stats = std::vector<TdStats>{sStartI, this->stats.end()};
          }
        }
      } else {
        // std::cout << "Found empty stats_, returning empty vector." <<
        // std::endl;
        return {};
      }
      break;
    }  // end of lock/case scope
    case VideoOpts::all:
    case VideoOpts::gasPlusCoords: {  // locks/case scope
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
      /*
      std::cerr <<
                                      "Initialized gTime with value " <<
                                      (gTime.has_value()?
      std::to_string(*gTime): "empty") <<
                                      ", gDeltaT with value " << gDeltaT <<
                                      std::endl;
      */
      if (!gTime.has_value()) {
        return {};
      }
      if (this->renders.size() || this->stats.size()) {
        // std::cerr << "Found sizes: renders_ = " << renders_.size()
        //	<< ", stats_ = " << stats_.size() << std::endl;
        //  dealing with fTime_
        if (!fTime.has_value() || !isIntMultOf(*gTime - *fTime, gDeltaT)) {
          // std::cerr << "Setting fTime_. Because ";
          if (fTime.has_value()) {
            // std::cerr << "isIntMultOf returned " << isIntMultOf(*gTime -
            // *fTime_, gDeltaT) << " for gTime - fTime_ = " << *gTime - *fTime_
            //<< ", gDeltaT = " << gDeltaT;
          } else {
            // std::cerr << "fTime_.has_value returned " << fTime_.has_value();
          }
          // std::cerr << std::endl;
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
              /*
              std::cerr << "Found getRendersT0 = " << getRendersT0(renders_)
                                              << ", stats beginning = " <<
              stats_[0].getTime0()
                                              << "; set fTime_ to " << *fTime_
              << std::endl;
              */
            } else {
              fTime = getRendersT0(this->renders) - gDeltaT;
              /*
              std::cerr << "Found getRendersT0 = " << getRendersT0(renders_)
              << ", stats beginning = " << stats_[0].getTime0()
              << "; set fTime_ to " << *fTime_ << std::endl;
              */
            }
          }
          // std::cerr << "Set fTime_. Now fTime_.has_value() returns " <<
          // fTime_.has_value() << " with value = " << fTime_.value() <<
          // std::endl;
        }

        // std::cerr << "Starting vector segments extraction." << std::endl;
        //  extracting algorithm-compatible vector segments
        if (this->stats.size()) {
          auto sStartI{std::upper_bound(this->stats.begin(), this->stats.end(),
                                        *fTime + gDeltaT,
                                        [](double value, TdStats const& s) {
                                          // std::cerr << "Value = " << value;
                                          // value <= s.getTime()?
                                          // std::cerr << " <= s.getTime() = "
                                          // << s.getTime(): std::cerr << " >
                                          // s.getTime() = " << s.getTime();
                                          // std::cerr << std::endl;
                                          return value <= s.getTime();
                                        })

          };
          // std::cerr << "There's nothing to fear but BIG SCARY MONSTERS
          // AAAAAHHHHH!!!!!!!!\n"; std::cerr << "Found sStartI = " << sStartI -
          // stats_.begin() << std::endl; std::cerr << "fTime_ value = " <<
          // *fTime_ << " with stats_ front Time0 = " <<
          // stats_.front().getTime0() << " and Time = " <<
          // stats_.front().getTime() << std::endl;
          if (sStartI != this->stats.end()) {
            auto gStartI{std::lower_bound(this->renders.begin(),
                                          this->renders.end(), *fTime,
                                          [](auto const& render, double value) {
                                            return render.second < value;
                                          })};
            assert(isIntMultOf(*gTime - *fTime, gDeltaT));
            auto gEndI{std::upper_bound(this->renders.begin(),
                                        this->renders.end(),
                                        this->stats.back().getTime(),
                                        [](double value, auto const& render) {
                                          return value < render.second;
                                        })};
            // std::cerr << "Found gStartI = " << gStartI -
            // this->renders.begin()
            //	<< ", gEndI = " << gEndI - renders_.begin() << ", with gSize = "
            //<< renders_.size() << std::endl;
            if (emptyStats) {
              stats.insert(stats.end(), std::make_move_iterator(sStartI),
                           std::make_move_iterator(this->stats.end()));
              this->stats.clear();
            } else {
              stats = std::vector(sStartI, this->stats.end());
            }
            assert(gStartI <=
                   gEndI);  // fTime_ should be always <= stats.back().getTime,
                            // if it is available
            // used to have a check for gStartI != renders_.end(), if fuckup
            // check here first
            renders.insert(renders.begin(), std::make_move_iterator(gStartI),
                           std::make_move_iterator(gEndI));
            this->renders.clear();
          }
          // std::cerr << "Extracted vectors. Result sizes: stats size = " <<
          // stats.size() << ", renders size = " << renders.size() << std::endl;
          if (!stats.size() && !renders.size()) {
            return {};
          }
        } else {
          auto gStartI{std::lower_bound(this->renders.begin(),
                                        this->renders.end(), *fTime,
                                        [](auto const& render, double value) {
                                          return render.second < value;
                                        })};
          // same as above
          renders.insert(renders.begin(), std::make_move_iterator(gStartI),
                         std::make_move_iterator(this->renders.end()));
          this->renders.clear();
        }
      } else {
        // std::cerr << "Found empty stats and renders, returning empty vector."
        // << std::endl;
        return {};
      }
      addedResults.store(false);
      resultsLock.unlock();
      break;
    }  // end of locks/case scope
    default:
      throw std::invalid_argument("Invalid video option provided.");
  }

  // std::cerr << "Got to second phase." << std::endl;

  // std::cout << "Started variables setup." << std::endl;
  // recurrent variables setup
  // declarations
  // text
  sf::Font font;
  font.loadFromFile("assets/JetBrains-Mono-Nerd-Font-Complete.ttf");
  std::ostringstream Temp{};
  sf::Text TText;
  std::ostringstream Vol{};
  sf::Text VText;
  std::ostringstream Num{};
  sf::Text NText;
  sf::RectangleShape txtBox;
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
    cnvs.Update();
    trnsfrImg->FromPad(&cnvs);
    RGBA32toRGBA8(trnsfrImg->GetRgbaArray(), trnsfrImg->GetWidth(),
                  trnsfrImg->GetHeight(), pixels);
    auxTxtr.update(pixels.data());
    // auxTxtr.update(auxImg);
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
        Vol << std::fixed << std::setprecision(2) << stats[0].getVolume();
        VText = {"V = " + Vol.str() + "m\u00B3", font, 18};
        VText.setFont(font);
        VText.setFillColor(sf::Color::Black);
        Num << std::fixed << std::setprecision(2) << stats[0].getNParticles();
        NText = {"N = " + Num.str(), font, 18};
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
  // std::cerr << "Got to phase 3." << std::endl;
  frame.create(windowSize.x, windowSize.y);
  switch (opt) {
    case VideoOpts::justGas: {
      // std::cerr << "JustGas case, operating on vector of size: " <<
      // renders.size() << std::endl;
      if (renders.size()) {
        assert(isIntMultOf(*fTime - *gTime, gDeltaT));
        assert(gTime == renders.back().second);
        assert(fTime <= getRendersT0(renders));
        box.setPosition(0, 0);
        size_t rIndex{0};
        // std::cerr << "Inserting frames:" << std::endl;
        while (*fTime + gDeltaT < gTime ||
               isNegligible(*fTime + gDeltaT - *gTime, gDeltaT)) {
          *fTime += gDeltaT;
          assert(isIntMultOf(*gTime - *fTime, gDeltaT));
          if (renders.size() > rIndex &&
              isNegligible(*fTime - renders[rIndex].second, gDeltaT)) {
            // std::cerr << "rIndex = " << rIndex << std::endl;
            box.setScale(
                windowSize.x / (float)renders[rIndex].first.getSize().x,
                windowSize.y / (float)renders[rIndex].first.getSize().y);
            box.setTexture(renders[rIndex++].first);
            frame.draw(box);
            frame.display();
          } else {
            // std::cout << "Inserting placeholder.";
            box.setScale(windowSize.x / (float)placeholder.getSize().x,
                         windowSize.y / (float)placeholder.getSize().y);
            box.setTexture(placeholder);
            frame.draw(box);
            frame.display();
          }
          frames.emplace_back(frame.getTexture());
        }
        renders.clear();
      }
      break;
    }
    case VideoOpts::justStats:
      // std::cerr << "Working with stats of size: " << stats.size() <<
      // std::endl;
      if (stats.size()) {
        if (*fTime + gDeltaT <
            stats.front()
                .getTime0()) {  // insert empty data and use placeholder render
          // std::cerr << "Inserting placeholder." << std::endl;
          auto& stat{stats.front()};
          for (size_t i{0}; i < 7; ++i) {
            TGraph* graph{(TGraph*)pGraphs.GetListOfGraphs()->At(i)};
            graph->AddPoint(*fTime, NAN);
            graph->AddPoint(stat.getTime0(), NAN);
          }

          mfpGraph.AddPoint(*fTime, NAN);
          mfpGraph.AddPoint(stat.getTime0(), NAN);
          kBGraph.AddPoint(*fTime, NAN);
          kBGraph.AddPoint(stat.getTime0(), NAN);

          frame.clear();

          cnvs.SetCanvasSize(windowSize.x * 0.5, windowSize.y * 0.45);
          ;
          auxTxtr.create(windowSize.x * 0.5, windowSize.y * 0.45);
          // auxImg.create(cnvs.GetWindowWidth(), cnvs.GetWindowHeight());

          drawObj(pGraphs, 0., 0., "APL", drawLambdas[0]);
          drawObj(kBGraph, 0., windowSize.y * 0.55, "APL", drawLambdas[1]);

          cnvs.SetCanvasSize(windowSize.x * 0.5, windowSize.y * 0.5);
          auxTxtr.create(windowSize.x * 0.5, windowSize.y * 0.5);
          // auxImg.create(cnvs.GetWindowWidth(), cnvs.GetWindowHeight());

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
          box.setTexture(placeholder);
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
              graph->AddPoint(stat.getTime(), stat.getPressure(Wall(i)));
            } else {
              graph->AddPoint(stat.getTime(), stat.getPressure());
            }
          }

          mfpGraph.AddPoint(stat.getTime(), stat.getMeanFreePath());
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
        break;
      }
    case VideoOpts::gasPlusCoords:
      assert(gTime == renders.back().second);
      assert(fTime <= getRendersT0(renders));
      {  // case scope
        sf::Vector2u gasSize{static_cast<unsigned>(windowSize.x * 0.5),
                             static_cast<unsigned>(windowSize.y * 0.9)};
        if (stats.size()) {
          auxTxtr.create(windowSize.x * 0.5, windowSize.y * 0.5);
          if (*fTime + gDeltaT < stats.front().getTime0()) {
            for (size_t i{0}; i < 7; ++i) {
              TGraph* graph{(TGraph*)pGraphs.GetListOfGraphs()->At(i)};
              graph->AddPoint(*fTime, NAN);
              graph->AddPoint(stats.front().getTime0(), NAN);
            }

            kBGraph.AddPoint(*fTime, NAN);
            kBGraph.AddPoint(stats.front().getTime0(), NAN);

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
            size_t rIndex{0};
            while (*fTime + gDeltaT < stats.front().getTime0()) {
              *fTime += gDeltaT;
              assert(isIntMultOf(*gTime - *fTime, gDeltaT));
              if (renders.size() > rIndex &&
                  isNegligible(*fTime - renders[rIndex].second, gDeltaT)) {
                box.setScale(
                    gasSize.x / (float)renders[rIndex].first.getSize().x,
                    gasSize.y / (float)renders[rIndex].first.getSize().y);
                box.setTexture(renders[rIndex++].first);
                frame.draw(box);
                frame.display();
              } else {
                box.setScale(gasSize.x / (float)placeholder.getSize().x,
                             gasSize.y / (float)placeholder.getSize().y);
                box.setTexture(placeholder);
                frame.draw(box);
                frame.display();
              }
              frames.emplace_back(frame.getTexture());
            }
          }
          assert(*fTime + gDeltaT >= stats.front().getTime0());
          size_t rIndex{0};
          while (*fTime + gDeltaT < stats.back().getTime()) {
            auto s{std::lower_bound(stats.begin(), stats.end(), *fTime,
                                    [](TdStats const& stat, double value) {
                                      return stat.getTime0() <= value;
                                    })};
            assert(s != stats.end());
            TdStats const& stat{*s};
            // make the graphs picture
            for (size_t i{0}; i < 7; ++i) {
              TGraph* graph{(TGraph*)pGraphs.GetListOfGraphs()->At(i)};
              if (i < 6) {
                graph->AddPoint(stat.getTime(), stat.getPressure(Wall(i)));
              } else {
                graph->AddPoint(stat.getTime(), stat.getPressure());
              }
            }
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
            box.setPosition(windowSize.x * 0.5, 0.);  // huh?
            while (*fTime + gDeltaT < stat.getTime()) {
              *fTime += gDeltaT;
              assert(isIntMultOf(*gTime - *fTime, gDeltaT));
              if (renders.size() > rIndex &&
                  isNegligible(*fTime - renders[rIndex].second, gDeltaT)) {
                box.setScale(
                    gasSize.x / (float)renders[rIndex].first.getSize().x,
                    gasSize.y / (float)renders[rIndex].first.getSize().y);
                box.setTexture(renders[rIndex++].first);
                frame.draw(box);
                frame.display();
              } else {
                box.setScale(gasSize.x / (float)placeholder.getSize().x,
                             gasSize.y / (float)placeholder.getSize().y);
                box.setTexture(placeholder);
                frame.draw(box);
                frame.display();
              }
              frames.emplace_back(frame.getTexture());
            }
          }  // while (fTime_ + gDeltaT_ < stats.back().getTime())
          trnsfrImg->Delete();
        } else if (renders.size()) {
          assert(renders.back().second == gTime);
          if (*fTime + gDeltaT < gTime ||
              isNegligible(*fTime + gDeltaT - *gTime, gDeltaT)) {
            for (size_t i{0}; i < 7; ++i) {
              TGraph* graph{(TGraph*)pGraphs.GetListOfGraphs()->At(i)};
              graph->AddPoint(*fTime, NAN);
              graph->AddPoint(*gTime, NAN);
            }

            kBGraph.AddPoint(*fTime, NAN);
            kBGraph.AddPoint(*gTime, NAN);

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
            box.setPosition(windowSize.x * 0.5,
                            0.);  // huh, again, seems like I was thinking
                                  // bottom left corner
            size_t rIndex{0};
            while (*fTime + gDeltaT < gTime ||
                   isNegligible(*fTime + gDeltaT - *gTime, gDeltaT)) {
              *fTime += gDeltaT;
              assert(isIntMultOf(*gTime - *fTime, gDeltaT));
              if (renders.size() > rIndex &&
                  isNegligible(*fTime - renders[rIndex].second, gDeltaT)) {
                box.setScale(
                    gasSize.x / (float)renders[rIndex].first.getSize().x,
                    gasSize.y / (float)renders[rIndex].first.getSize().y);
                box.setTexture(renders[rIndex++].first);
                frame.draw(box);
                frame.display();
              } else {
                box.setScale(gasSize.x / (float)placeholder.getSize().x,
                             gasSize.y / (float)placeholder.getSize().y);
                box.setTexture(placeholder);
                frame.draw(box);
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
      assert(fTime <= getRendersT0(renders));
      assert(gTime == renders.back().second);
      sf::Vector2u gasSize{static_cast<unsigned>(windowSize.x * 0.5),
                           static_cast<unsigned>(windowSize.y * 0.9)};
      if (stats.size()) {
        auxTxtr.create(windowSize.x * 0.25, windowSize.y * 0.5);
        if (*fTime + gDeltaT < stats.front().getTime0()) {
          for (size_t i{0}; i < 7; ++i) {
            TGraph* graph{(TGraph*)pGraphs.GetListOfGraphs()->At(i)};
            graph->AddPoint(*fTime, NAN);
            graph->AddPoint(stats.front().getTime0(), NAN);
          }

          mfpGraph.AddPoint(*fTime, NAN);
          mfpGraph.AddPoint(stats.front().getTime0(), NAN);
          kBGraph.AddPoint(*fTime, NAN);
          kBGraph.AddPoint(stats.front().getTime0(), NAN);
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
            assert(isIntMultOf(*gTime - *fTime, gDeltaT));
            if (renders.size() > rIndex &&
                isNegligible(*fTime - renders[rIndex].second, gDeltaT)) {
              box.setScale(
                  gasSize.x / (float)renders[rIndex].first.getSize().x,
                  gasSize.y / (float)renders[rIndex].first.getSize().y);
              box.setTexture(renders[rIndex++].first);
              frame.draw(box);
              frame.display();
            } else {
              box.setScale(gasSize.x / (float)placeholder.getSize().x,
                           gasSize.y / (float)placeholder.getSize().y);
              box.setTexture(placeholder);
              frame.draw(box);
              frame.display();
            }
            frames.emplace_back(frame.getTexture());
          }
        }
        size_t rIndex{0};
        while (*fTime + gDeltaT < stats.back().getTime()) {
          assert(*fTime + gDeltaT >= stats.front().getTime0());
          auto s{std::lower_bound(stats.begin(), stats.end(), *fTime,
                                  [](TdStats const& stat, double value) {
                                    return stat.getTime0() <= value;
                                  })};
          // std::cerr << "Found stat index = " << s - stats.begin() << " for
          // fTime_ = " << *fTime_ + gDeltaT << std::endl;
          assert(s != stats.end());
          TdStats const& stat{*s};
          // make the graphs picture
          for (size_t k{0}; k < 7; ++k) {
            TGraph* graph{(TGraph*)pGraphs.GetListOfGraphs()->At(k)};
            if (k < 6) {
              graph->AddPoint(stat.getTime(), stat.getPressure(Wall(k)));
            } else {
              graph->AddPoint(stat.getTime(), stat.getPressure());
            }
          }
          mfpGraph.AddPoint(stat.getTime(), stat.getMeanFreePath());
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
          // auxImg.create(windowSize.x * 0.25, windowSize.y * 0.5);
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
            assert(isIntMultOf(*gTime - *fTime, gDeltaT));
            if (renders.size() > rIndex &&
                isNegligible(*fTime - renders[rIndex].second, gDeltaT)) {
              box.setScale(
                  gasSize.x / (float)renders[rIndex].first.getSize().x,
                  gasSize.y / (float)renders[rIndex].first.getSize().y);
              box.setTexture(renders[(rIndex++)].first);
              frame.draw(box);
              frame.display();
            } else {
              box.setTexture(placeholder);
              box.setScale(gasSize.x / (float)placeholder.getSize().x,
                           gasSize.y / (float)placeholder.getSize().y);
              frame.draw(box);
              frame.display();
            }
            frames.emplace_back(frame.getTexture());
          }
        }  // while (fTime_ + gDeltaT_ < stats.back().getTime())
      } else if (renders.size()) {
        for (size_t i{0}; i < 7; ++i) {
          TGraph* graph{(TGraph*)pGraphs.GetListOfGraphs()->At(i)};
          graph->AddPoint(*fTime, NAN);
          graph->AddPoint(*gTime, NAN);
        }

        mfpGraph.AddPoint(*fTime, NAN);
        mfpGraph.AddPoint(*gTime, NAN);
        kBGraph.AddPoint(*fTime, NAN);
        kBGraph.AddPoint(*gTime, NAN);
        TH1D speedH{};

        frame.clear();

        cnvs.SetCanvasSize(windowSize.x * 0.25, windowSize.y * 0.5);
        // auxImg.create(windowSize.x * 0.25, windowSize.y * 0.5);
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
          assert(isIntMultOf(*gTime - *fTime, gDeltaT));
          if (renders.size() > rIndex &&
              isNegligible(*fTime - renders[rIndex].second, gDeltaT)) {
            box.setScale(gasSize.x / (float)renders[rIndex].first.getSize().x,
                         gasSize.y / (float)renders[rIndex].first.getSize().y);
            box.setTexture(renders[rIndex++].first);
            frame.draw(box);
            frame.display();
          } else {
            box.setScale(gasSize.x / (float)placeholder.getSize().x,
                         gasSize.y / (float)placeholder.getSize().y);
            box.setTexture(placeholder);
            frame.draw(box);
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

  return frames;
}

}  // namespace GS
