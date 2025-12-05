#include <SFML/Graphics/RenderTexture.hpp>
#include <iterator>
#include <stdexcept>
#include <thread>
#include <iostream>

#include <SFML/Window/Context.hpp>

#include "SimDataPipeline.hpp"

namespace GS {

void SimDataPipeline::setFramerate(double framerate) {
  if (framerate <= 0) {
    throw std::invalid_argument(
        "setFramerate error: provided non-positive framerate");
  } else {
    gDeltaT.store(1. / framerate);
  }
}

SimDataPipeline::SimDataPipeline(size_t statSize, double framerate,
                                 TH1D const& speedsHTemplate)
    : statSize(statSize), speedsHTemplate(speedsHTemplate) {
  if (!statSize) {
    throw std::invalid_argument(
        "SDP constructor error: provided null statSize");
  }
  setFramerate(framerate);
  if (speedsHTemplate.IsZombie()) {
    throw std::invalid_argument(
        "SDP constructor error: provided zombie histogram template");
  }
  if (speedsHTemplate.GetEntries() != 0) {
    throw std::invalid_argument(
        "SDP constructor error: provided non-empty histogram template");
  }
  assert(speedsHTemplate.GetNbinsX() != 0);
}

size_t SimDataPipeline::getRawDataSize() {
  std::lock_guard<std::mutex> dataGuard(rawDataMtx);
  return rawData.size();
}

bool isNegligible(double epsilon, double x);  // implemented in TdStats.cpp

void SimDataPipeline::addData(std::vector<GasData>&& data) {
  if (data.size()) {
    dataDone.store(false);
    double prevDTime;
    bool firstD{true};
    for (GasData const& d : data) {
      if (!nParticles.has_value()) {
        nParticles = d.getParticles().size();
        assert(nParticles.value());
      } else {
        if (d.getParticles().size() != nParticles) {
          throw std::invalid_argument(
              "SDP addData error: non-matching particle numbers");
        }
      }
      if (!firstD &&
          !isNegligible(d.getT0() - prevDTime, d.getTime() - d.getT0())) {
        throw std::invalid_argument(
            "SDP addData error: provided non sequential data vector");
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
              "SDP addData error: data time is smaller than latest raw data "
              "piece time");
        }
      }
      rawData.insert(rawData.end(), std::make_move_iterator(data.begin()),
                     std::make_move_iterator(data.end()));
      rawDataBackTime = rawData.back().getTime();
    }
    rawDataCv.notify_all();
  }
}

void SimDataPipeline::processData(bool mfpMemory) {
  processing.store(true);
  std::vector<GasData> data{};
  std::unique_lock<std::mutex> rawDataLock(rawDataMtx, std::defer_lock);
  while (true) {
    rawDataLock.lock();
    rawDataCv.wait_for(rawDataLock, std::chrono::milliseconds(100), [this] {
      return rawData.size() > statSize.load() || dataDone.load();
    });
    size_t statSize{this->statSize.load()};  // stat size for this iteration
    if (dataDone.load() && rawData.size() < statSize) {
      break;
    }
    size_t nStats{rawData.size() / statSize};
    {  // chunkSize nspc
      size_t chunkSize = statChunkSize.load();
      if (chunkSize) {
        nStats = nStats > chunkSize ? chunkSize : nStats;
      }
    }  // chunkSize nspcEnd

    if (nStats) {
      {  // guard scope begin
        data.insert(
            data.end(), std::make_move_iterator(rawData.begin()),
            std::make_move_iterator(rawData.begin() + nStats * statSize));
        assert(data.size());
        rawData.erase(rawData.begin(), rawData.begin() + nStats * statSize);
        rawDataLock.unlock();
				std::vector<TdStats> tempStats;

        processStats(data, mfpMemory, tempStats);
        std::lock_guard<std::mutex> outputGuard{outputMtx};
				std::lock_guard<std::mutex> statsGuard {statsMtx};
				stats.insert(stats.end(),
						std::make_move_iterator(tempStats.begin()),
						std::make_move_iterator(tempStats.end())
						);
      }  // guards scope end
      addedResults.store(true);
      outputCv.notify_all();
    } else {
      rawDataLock.unlock();
    }
    data.clear();
  }
  processing.store(false);
}

void SimDataPipeline::processData(Camera camera,
                                  RenderStyle style, bool mfpMemory) {
  processing.store(true);
  std::unique_lock<std::mutex> rawDataLock{rawDataMtx, std::defer_lock};
  while (true) {
    rawDataLock.lock();
    rawDataCv.wait_for(rawDataLock, std::chrono::milliseconds(100), [this] {
      return rawData.size() > statSize.load() || dataDone.load();
    });
    size_t statSize{this->statSize.load()};  // stat size for this iteration
    if (dataDone.load() && rawData.size() < statSize) {
      break;
    }

    size_t nStats{rawData.size() / statSize};
    size_t chunkSize{statChunkSize.load()};
    if (chunkSize) {
      nStats = nStats > chunkSize ? chunkSize : nStats;
    }

    if (nStats) {
			auto data{std::make_shared<std::vector<GasData>>()};
			std::vector<TdStats> tempStats {};
			std::vector<std::pair<sf::Texture, double>> tempRenders {};
      data->insert(data->end(), std::make_move_iterator(rawData.begin()),
                  std::make_move_iterator(rawData.begin() + nStats * statSize));
      assert(data->size());
      rawData.erase(rawData.begin(), rawData.begin() + nStats * statSize);
      rawDataLock.unlock();

			std::thread sThread {[this, mfpMemory, data, &tempStats]() {
				try {
					processStats(*data, mfpMemory, tempStats);
				} catch (std::exception const& e) {
					std::cerr << "What..." << std::endl;
					std::terminate();
				}
			}};

			std::thread gThread {[camera, style, this, data, &tempRenders]() {
				try {
					processGraphics(*data, camera, style, tempRenders);
				} catch (std::exception const& e) {
					std::cerr << "What (graphicz)..." << std::endl;
					std::terminate();
				}
			}};

			if (sThread.joinable()) {
				sThread.join();
			}
			if (gThread.joinable()) {
				gThread.join();
			}
      {  // output guard scope begin
        std::lock_guard<std::mutex> outputGuard{outputMtx};
				{ // stats guard scope begin
				std::lock_guard<std::mutex> statsGuard {statsMtx};
				stats.insert(stats.end(),
						std::make_move_iterator(tempStats.begin()), 
						std::make_move_iterator(tempStats.end())
				);
				}	// stats guard scope end
				{ // renders guard scope begin
				std::lock_guard<std::mutex> gTimeGuard {gTimeMtx};
				std::lock_guard<std::mutex> rendersGuard {rendersMtx};
				renders.insert(renders.end(),
						std::make_move_iterator(tempRenders.begin()),
						std::make_move_iterator(tempRenders.end())
				);
				gTime = renders.back().second;
				} // renders guard scope end
      }  // output guard scope end
      addedResults.store(true);
      outputCv.notify_all();
      data->clear();
			tempStats.clear();
			tempRenders.clear();
    } else {
      rawDataLock.unlock();
    }
  }
  processing.store(false);
}

void SimDataPipeline::processGraphics(std::vector<GasData> const& data,
                                      Camera const& camera,
                                      RenderStyle const& style,
																			std::vector<std::pair<sf::Texture, double>>& tempRenders) {
  sf::RenderTexture picture;
	picture.setActive();

  // setting local time variables
  double gTime;
  double gDeltaT{this->gDeltaT.load()};
	{
	std::lock_guard<std::mutex> gTimeGuard{gTimeMtx};
	assert(gDeltaT > 0.);
	if (!this->gTime.has_value()) {
		this->gTime = data[0].getT0() - gDeltaT;
	}
	assert(this->gTime <= data[0].getT0());
	gTime = *this->gTime;
	}

  tempRenders.reserve(
      (data.back().getTime() - data.front().getTime()) / gDeltaT + 1);

  while (gTime + gDeltaT < data[0].getT0()) {
    gTime += gDeltaT;
  }

  for (GasData const& dat : data) {
    while (gTime + gDeltaT <= dat.getTime()) {
      gTime += gDeltaT;
      drawGas(dat, camera, picture, style, gTime - dat.getTime());
      tempRenders.emplace_back(picture.getTexture(), gTime);
    }
  }
}

void SimDataPipeline::processStats(std::vector<GasData> const& data,
                                   bool mfpMemory,
																	 std::vector<GS::TdStats>& tempStats) {

  size_t statSize = this->statSize.load();  // local statSize for this call
  assert(!(data.size() % statSize));

  tempStats.reserve(data.size() / statSize);

  if (mfpMemory) {
    std::lock_guard<std::mutex> lastStatGuard{lastStatMtx};
    for (size_t i{0}; i < data.size() / statSize; ++i) {
      TdStats stat{tempStats.size()
                       ? TdStats{data[i * statSize], TdStats(tempStats.back())}
                   : lastStat.has_value()
                       ? TdStats{data[i * statSize], std::move(*lastStat)}
                       : TdStats{data[i * statSize], speedsHTemplate}};
      for (size_t j{1}; j < statSize; ++j) {
        stat.addData(data[i * statSize + j]);
      }
      tempStats.emplace_back(std::move(stat));
    }
    lastStat = tempStats.back();
  } else {
    for (size_t i{0}; i < data.size() / statSize; ++i) {
      TdStats stat{data[i * statSize], speedsHTemplate};
      for (size_t j{1}; j < statSize; ++j) {
        stat.addData(data[i * statSize + j]);
      }
      tempStats.emplace_back(std::move(stat));
    }
  }
}

std::vector<TdStats> SimDataPipeline::getStats(bool emptyQueue) {
  if (emptyQueue) {
    std::deque<TdStats> tempStats{};
    std::lock_guard<std::mutex> lastStatGuard{lastStatMtx};
    {  // lock scope
      std::lock_guard<std::mutex> statsGuard{statsMtx};
      if (stats.size()) {
        lastStat = stats.back();
        tempStats = std::move(stats);
        stats.clear();
      }
    }  // lock scope end
    return std::vector<TdStats>(std::make_move_iterator(tempStats.begin()),
                                std::make_move_iterator(tempStats.end()));
  } else {
    std::lock_guard<std::mutex> guard{statsMtx};
    return std::vector<TdStats>(stats.begin(), stats.end());
  }
}

std::vector<sf::Texture> SimDataPipeline::getRenders(bool emptyQueue) {
	sf::Context c;
  std::vector<sf::Texture> tempRenders{};
  if (emptyQueue) {
    std::lock_guard<std::mutex> rendersGuard{rendersMtx};
    tempRenders.reserve(renders.size());
    for (auto& r : renders) {
      tempRenders.emplace_back(std::move(r.first));
    }
    renders.clear();
  } else {
    std::lock_guard<std::mutex> rendersGuard {rendersMtx};
    tempRenders.reserve(renders.size());
    for (auto const& r : renders) {
      tempRenders.emplace_back(r.first);
    }
  }
  return tempRenders;
}

void SimDataPipeline::setStatChunkSize(size_t s) {
  if (s) {
    statChunkSize.store(s);
  } else {
    throw(std::invalid_argument(
        "setStatChunkSize error: provided null stat chunk size"));
  }
}

void SimDataPipeline::setStatSize(size_t s) {
  if (s) {
    statSize.store(s);
  } else {
    throw(std::invalid_argument("setStatSize error: provided null stat size"));
  }
}

void SimDataPipeline::setFont(sf::Font const& f) {
  if (f.getInfo().family.empty()) {
    throw std::invalid_argument("setFont error: provided empty font");
  } else {
    font = f;
  }
}

}  // namespace GS
