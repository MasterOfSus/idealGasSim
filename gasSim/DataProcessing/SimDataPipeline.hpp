#ifndef SIMDATAPIPELINE_HPP
#define SIMDATAPIPELINE_HPP

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <optional>

#include "../Graphics.hpp"
#include "TdStats.hpp"

namespace GS {

enum class VideoOpts { justGas, justStats, gasPlusCoords, all };

class SimDataPipeline {
 public:
  SimDataPipeline(size_t statSize, double framerate,
                  TH1D const& speedsHTemplate);

  void setStatChunkSize(size_t statChunkSize);
  size_t getStatChunkSize() const { return statChunkSize.load(); }
  void setFramerate(double framerate);

  void addData(std::vector<GasData>&& data);
  void processData(bool mfpMemory = true, std::function<bool()> stopper = [] { return false; });
  void processData(Camera camera, RenderStyle style,
                   bool mfpMemory = true, std::function<bool()> stopper = [] { return false; });
  std::vector<sf::Texture> getVideo(
      VideoOpts opt, sf::Vector2u windowSize, sf::Texture const& placeHolderT,
      TList& outputGraphs, bool emptyQueue = false,
      std::function<void(TH1D&, VideoOpts)> fitLambda = {},
      std::array<std::function<void()>, 4> drawLambdas = {});
  size_t getRawDataSize();
  double getFramerate() const { return 1. / gDeltaT.load(); }
  size_t getStatSize() const { return statSize.load(); }
  void setStatSize(size_t size);

	size_t getNStats();
  std::vector<TdStats> getStats(bool clearMem = false);
	size_t getNRenders();
  std::vector<sf::Texture> getRenders(bool clearMem = false);

  bool isProcessing() { return processing.load(); }
  bool isDone() { return dataDone.load(); }
  void setDone() { dataDone.store(true); }

  void setFont(sf::Font const& font);

 private:
  void processStats(std::vector<GasData> const& data, bool mfpMemory, std::vector<TdStats>& tempResults);
  void processGraphics(std::vector<GasData> const& data, Camera const& camera,
                       RenderStyle const& style,
											 std::vector<std::pair<sf::Texture, double>>& tempRenders);

  std::atomic<bool> dataDone{false};
  std::atomic<bool> processing{false};
  std::atomic<bool> addedResults{false};
  std::deque<GasData> rawData{};
  std::mutex rawDataMtx;
  std::condition_variable rawDataCv;
  std::optional<double> rawDataBackTime{};

  std::atomic<size_t> statSize;
  std::atomic<size_t> statChunkSize;
  std::deque<TdStats> stats{};
  std::mutex statsMtx;
  std::optional<TdStats> lastStat;
  std::mutex lastStatMtx;

  std::atomic<double> gDeltaT;
  std::optional<double> gTime;
  std::mutex gTimeMtx;
  std::deque<std::pair<sf::Texture, double>> renders;
  std::mutex rendersMtx;

  std::mutex outputMtx;
  std::condition_variable outputCv;

  std::optional<double> fTime;

  std::optional<size_t> nParticles;

  const TH1D speedsHTemplate;
  sf::Font font;
};
}  // namespace GS

#endif
