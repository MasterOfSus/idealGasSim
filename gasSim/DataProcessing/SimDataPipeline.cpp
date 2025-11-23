#include "SimDataPipeline.hpp"

#include <SFML/Graphics/RenderTexture.hpp>
#include <execution>
#include <iterator>
#include <thread>
/*
std::deque<GasData> SimOutput::getData() {
  std::lock_guard<std::mutex> rawDataGuard {rawDataMtx_};
        return rawData_;
}
*/

void GS::SimDataPipeline::setFramerate(double framerate) {
  if (framerate <= 0) {
    throw std::invalid_argument("Non-positive framerate provided.");
  } else {
    gDeltaT.store(1. / framerate);
  }
}

GS::SimDataPipeline::SimDataPipeline(size_t statSize, double framerate,
                                     const TH1D& speedsHTemplate)
    : statSize(statSize), speedsHTemplate(speedsHTemplate) {
  setFramerate(framerate);
  if (speedsHTemplate.IsZombie()) {
    throw std::invalid_argument("Zombie histogram template provided");
  }
  if (speedsHTemplate.GetEntries() != 0) {
    throw std::invalid_argument("Non-empty histogram template provided");
  }
  assert(speedsHTemplate.GetNbinsX() != 0);
}

size_t GS::SimDataPipeline::getRawDataSize() {
  std::lock_guard<std::mutex> dataGuard(rawDataMtx);
  return rawData.size();
}

bool isNegligible(double epsilon, double x);  // implemented in TdStats.cpp

void GS::SimDataPipeline::addData(std::vector<GasData>&& data) {
  if (data.size()) {
    dataDone.store(false);
    double prevDTime;
    bool firstD{true};
    for (const GasData& d : data) {
      if (!nParticles.has_value()) {
        nParticles = d.getParticles().size();
        assert(nParticles.value());
      } else {
        if (d.getParticles().size() != nParticles) {
          throw std::invalid_argument("Non-matching particle numbers.");
        }
      }
      if (!firstD &&
          !isNegligible(d.getT0() - prevDTime, d.getTime() - d.getT0())) {
        throw std::invalid_argument("Non sequential data vector provided.");
      }
      prevDTime = d.getTime();
      firstD = false;
    }
    {
      std::lock_guard<std::mutex> rawDataGuard{rawDataMtx};
      if (rawDataBackTime.has_value()) {
        if (!isNegligible(data.front().getT0() - rawDataBackTime.value(),
                          data.front().getTime() - data.front().getT0())) {
          throw std::invalid_argument(
              "Argument time less than latest raw data piece time");
        }
      }
      rawData.insert(rawData.end(), std::make_move_iterator(data.begin()),
                     std::make_move_iterator(data.end()));
      rawDataBackTime = rawData.back().getTime();
    }
    rawDataCv.notify_all();
  }
}

void GS::SimDataPipeline::processData(bool mfpMemory) {
  processing.store(true);
  std::vector<GasData> data{};
  std::unique_lock<std::mutex> rawDataLock(rawDataMtx, std::defer_lock);
  while (true) {
    rawDataLock.lock();
    rawDataCv.wait_for(rawDataLock, std::chrono::milliseconds(100), [this] {
      return rawData.size() > statSize.load() || dataDone.load();
    });
    size_t itStatSize{statSize.load()};  // stat size for this iteration
    if (dataDone && rawData.size() < itStatSize) {
      break;
    }
    size_t nStats{rawData.size() / itStatSize};
    {  // chunkSize nspc
      size_t chunkSize = statChunkSize.load();
      if (chunkSize) {
        nStats = nStats > chunkSize ? chunkSize : nStats;
      }
    }  // chunkSize nspcEnd
    // don't stop make it drop dj blow my speakers up
    // std::cout << "nStats for this iteration is " << rawData_.size() << "/" <<
    // statSize_ << " = " << nStats << std::endl;

    if (nStats) {
      {  // guard scope begin
        std::lock_guard<std::mutex> outputGuard{outputMtx};
        data.insert(
            data.end(), std::make_move_iterator(rawData.begin()),
            std::make_move_iterator(rawData.begin() + nStats * itStatSize));
        assert(data.size());
        // std::cout << "GasData count = " << data.size() << std::endl;
        rawData.erase(rawData.begin(), rawData.begin() + nStats * itStatSize);
        rawDataLock.unlock();

        // std::cout << "Data size: " << data.size() << "; ";
        processStats(data, mfpMemory);
        /*{
        std::lock_guard<std::mutex> guard {statsMtx_};
        //std::cout << "Stats size: " << stats_.size() << std::endl;
        }*/
      }  // guard scope end
      addedResults.store(true);
      outputCv.notify_all();
    } else {
      rawDataLock.unlock();
    }
    data.clear();
  }
  processing.store(false);
}

void GS::SimDataPipeline::processData(const Camera& camera,
                                      const RenderStyle& style,
                                      bool mfpMemory) {
  processing.store(true);
  // std::cout << "Started processing data.\n";
  std::vector<GasData> data{};
  std::unique_lock<std::mutex> rawDataLock{rawDataMtx, std::defer_lock};
  while (true) {
    rawDataLock.lock();
    rawDataCv.wait_for(rawDataLock, std::chrono::milliseconds(100), [this] {
      return rawData.size() > statSize.load() || dataDone;
    });
    // std::cout << "Done status: " << done_ << ". RawData emptiness: " <<
    // rawData_.empty() << std::endl;
    size_t itStatSize{statSize.load()};  // stat size for this iteration
    if (dataDone.load() && rawData.size() < itStatSize) {
      break;
    }

    size_t nStats{rawData.size() / itStatSize};
    size_t chunkSize{statChunkSize.load()};
    if (chunkSize) {
      nStats = nStats > chunkSize ? chunkSize : nStats;
    }

    if (nStats) {
      data.insert(
          data.end(), std::make_move_iterator(rawData.begin()),
          std::make_move_iterator(rawData.begin() + nStats * itStatSize));
      assert(data.size());
      rawData.erase(rawData.begin(), rawData.begin() + nStats * itStatSize);
      rawDataLock.unlock();

      std::thread sThread;
      std::thread gThread;

      {  // guard scope begin
        std::lock_guard<std::mutex> outputGuard{outputMtx};

        sThread = std::thread([this, &data, mfpMemory]() {
          try {
            processStats(data, mfpMemory);
          } catch (const std::exception& e) {
            // std::cerr << "Error in stats thread: " << e.what() << std::endl;
            std::terminate();
          }
        });

        gThread = std::thread([this, &data, &camera, &style]() {
          try {
            processGraphics(data, camera, style);
          } catch (const std::exception& e) {
            // std::cerr << "Error in graphics thread: " << e.what() <<
            // std::endl;
            std::terminate();
          }
        });

        if (sThread.joinable()) sThread.join();
        if (gThread.joinable()) gThread.join();
      }  // guard scope end
      addedResults.store(true);
      outputCv.notify_all();
      data.clear();
    } else {
      rawDataLock.unlock();
    }
  }
  processing.store(false);
}

// read VERY carefully during reread, weird naming changes might hvae fucked up
// everything
void GS::SimDataPipeline::processGraphics(const std::vector<GasData>& data,
                                          const Camera& camera,
                                          const RenderStyle& style) {
  // std::cout << "Processing graphics... ";
  // std::cout.flush();
  std::vector<std::pair<sf::Texture, double>> tempRenders{};

  // setting local time variables
  double gTimeL;
  double gDeltaTL{gDeltaT.load()};
  {  // guard scope
    std::lock_guard<std::mutex> gTimeGuard{gTimeMtx};
    if (!gTime.has_value()) {
      gTime = data[0].getT0() - gDeltaT;
    }
    assert(gDeltaTL > 0.);
    if (data.size()) {
      assert(gTime <= data[0].getT0());
    }
    gTimeL = *gTime;
  }  // guard scope
  sf::RenderTexture picture;

  tempRenders.reserve(
      (data.back().getTime() - data.front().getTime()) / gDeltaTL + 1);

  while (gTimeL + gDeltaTL < data[0].getT0()) {
    gTimeL += gDeltaTL;
  }

  for (const GasData& dat : data) {
    while (gTimeL + gDeltaTL <= dat.getTime()) {
      // std::cout << "Drawing gas at time " << *gTime_ + gDeltaT_ << std::endl;
      gTimeL += gDeltaTL;
      drawGas(dat, camera, picture, style, gTimeL - dat.getTime());
      renders.emplace_back(picture.getTexture(), gTimeL);
    }
  }

  std::lock_guard<std::mutex> rendersGuard{rendersMtx};
  renders.insert(renders.end(), std::make_move_iterator(tempRenders.begin()),
                 std::make_move_iterator(tempRenders.end()));
  std::lock_guard<std::mutex> gTimeGuard{gTimeMtx};
  gTime = gTimeL;
  // std::cout << "done!\n";
}

void GS::SimDataPipeline::processStats(const std::vector<GasData>& data,
                                       bool mfpMemory) {
  // std::cout << "Started processing stats.\n";
  std::vector<TdStats> tempStats{};

  size_t statSizeL = statSize.load();  // local statSize for this call
  assert(!(data.size() % statSizeL));

  tempStats.reserve(data.size() / statSizeL);

  // std::cout << "Reached pre-loop\n";
  if (mfpMemory) {
    std::lock_guard<std::mutex> lastStatGuard{lastStatMtx};
    for (unsigned i{0}; i < data.size() / statSizeL; ++i) {
      TdStats stat{tempStats.size()
                       ? TdStats{data[i * statSizeL], TdStats(tempStats.back())}
                   : lastStat.has_value()
                       ? TdStats{data[i * statSizeL], std::move(*lastStat)}
                       : TdStats{data[i * statSizeL], speedsHTemplate}};
      // std::cerr << "Bin number for lastStat_ speedsH_ = " <<
      // lastStat_->getSpeedH().GetNbinsX() << std::endl; std::cerr << "Bin
      // number for speedsHTemplate_ = " << speedsHTemplate_.GetNbinsX() <<
      // std::endl;
      for (unsigned j{1}; j < statSizeL; ++j) {
        stat.addData(data[i * statSizeL + j]);
      }
      tempStats.emplace_back(std::move(stat));
    }
    lastStat = tempStats.back();
  } else {
    for (size_t i{0}; i < data.size() / statSizeL; ++i) {
      TdStats stat{data[i * statSizeL], speedsHTemplate};
      for (size_t j{1}; j < statSizeL; ++j) {
        stat.addData(data[i * statSizeL + j]);
      }
      tempStats.emplace_back(std::move(stat));
    }
  }
  // std::cout << "Done with loop\n";
  std::lock_guard<std::mutex> statsGuard{statsMtx};
  stats.insert(stats.end(), std::make_move_iterator(tempStats.begin()),
               std::make_move_iterator(tempStats.end()));
}

std::vector<GS::TdStats> GS::SimDataPipeline::getStats(bool emptyQueue) {
  if (emptyQueue) {
    std::deque<TdStats> tempStats;
    std::lock_guard<std::mutex> lastStatGuard{lastStatMtx};
    // hopefully locks stats for less time by swapping, then converting to
    // vector after releasing lock
    {  // lock scope
      std::lock_guard<std::mutex> statsGuard{statsMtx};
      lastStat = stats.back();
      tempStats = std::exchange(stats, {});
      stats.clear();
    }  // lock scope end
    return std::vector<TdStats>(std::make_move_iterator(tempStats.begin()),
                                std::make_move_iterator(tempStats.end()));
  } else {
    std::lock_guard<std::mutex> guard{statsMtx};
    return std::vector<TdStats>(stats.begin(), stats.end());
  }
}

std::vector<sf::Texture> GS::SimDataPipeline::getRenders(bool emptyQueue) {
  std::vector<sf::Texture> tempRenders{};
  if (emptyQueue) {
    std::lock_guard<std::mutex> rendersGuard{rendersMtx};
    tempRenders.reserve(renders.size());
    for (auto& r : renders) {
      tempRenders.emplace_back(std::move(r.first));
    }
    renders.clear();
  } else {
    std::lock_guard<std::mutex> guard(rendersMtx);
    tempRenders.reserve(renders.size());
    for (auto& r : renders) {
      tempRenders.emplace_back(r.first);
    }
  }
  return tempRenders;
}

void GS::SimDataPipeline::setStatChunkSize(size_t s) {
  if (s) {
    statChunkSize.store(s);
  } else {
    throw(std::invalid_argument("Provided null stat chunk size."));
  }
}

void GS::SimDataPipeline::setStatSize(size_t s) {
  if (s) {
    statSize.store(s);
  } else {
    throw(std::invalid_argument("Provided null stat size."));
  }
}
