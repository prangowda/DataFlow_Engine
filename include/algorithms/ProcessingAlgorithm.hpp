#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <functional>

#include "../core/DataPacket.hpp"

namespace dataflow {
namespace algorithms {

/**
 * @class ProcessingAlgorithm
 * @brief Base class for all data processing algorithms
 */
class ProcessingAlgorithm {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~ProcessingAlgorithm() = default;
    
    /**
     * @brief Process a data packet
     * @param input The input data packet
     * @return The processed output data packet
     */
    virtual std::shared_ptr<core::DataPacket> process(std::shared_ptr<core::DataPacket> input) = 0;
    
    /**
     * @brief Get the name of this algorithm
     * @return The algorithm name
     */
    virtual std::string getName() const = 0;
    
    /**
     * @brief Configure the algorithm with parameters
     * @param params Configuration parameters
     */
    virtual void configure(const std::unordered_map<std::string, std::string>& params) = 0;
};

/**
 * @class FFTAlgorithm
 * @brief Fast Fourier Transform implementation
 */
class FFTAlgorithm : public ProcessingAlgorithm {
public:
    FFTAlgorithm();
    ~FFTAlgorithm() override = default;
    
    std::shared_ptr<core::DataPacket> process(std::shared_ptr<core::DataPacket> input) override;
    std::string getName() const override { return "FFT"; }
    void configure(const std::unordered_map<std::string, std::string>& params) override;

private:
    size_t windowSize_;
    bool useHanning_;
    
    // FFT implementation
    void performFFT(const std::vector<double>& input, std::vector<std::complex<double>>& output);
};

/**
 * @class ImageProcessingAlgorithm
 * @brief Image processing implementation
 */
class ImageProcessingAlgorithm : public ProcessingAlgorithm {
public:
    ImageProcessingAlgorithm();
    ~ImageProcessingAlgorithm() override = default;
    
    std::shared_ptr<core::DataPacket> process(std::shared_ptr<core::DataPacket> input) override;
    std::string getName() const override { return "ImageProcessing"; }
    void configure(const std::unordered_map<std::string, std::string>& params) override;

private:
    enum class FilterType {
        Blur,
        Sharpen,
        EdgeDetection
    };
    
    FilterType filterType_;
    int kernelSize_;
    
    // Image processing methods
    void applyFilter(const std::vector<uint8_t>& input, std::vector<uint8_t>& output,
                   int width, int height, int channels);
};

/**
 * @class MachineLearningAlgorithm
 * @brief Machine learning inference implementation
 */
class MachineLearningAlgorithm : public ProcessingAlgorithm {
public:
    MachineLearningAlgorithm();
    ~MachineLearningAlgorithm() override = default;
    
    std::shared_ptr<core::DataPacket> process(std::shared_ptr<core::DataPacket> input) override;
    std::string getName() const override { return "MachineLearning"; }
    void configure(const std::unordered_map<std::string, std::string>& params) override;

private:
    std::string modelPath_;
    float confidenceThreshold_;
    
    // Would typically contain or link to ML framework
    void loadModel(const std::string& path);
    std::vector<float> runInference(const std::vector<float>& input);
};

/**
 * @class AlgorithmFactory
 * @brief Factory for creating algorithm instances
 */
class AlgorithmFactory {
public:
    /**
     * @brief Create an algorithm by name
     * @param name The name of the algorithm to create
     * @return A shared pointer to the created algorithm
     */
    static std::shared_ptr<ProcessingAlgorithm> create(const std::string& name);
    
    /**
     * @brief Register a custom algorithm creator function
     * @param name Algorithm name
     * @param creator Function that creates an instance of the algorithm
     */
    static void registerAlgorithm(
        const std::string& name,
        std::function<std::shared_ptr<ProcessingAlgorithm>()> creator);

private:
    static std::unordered_map<std::string, 
        std::function<std::shared_ptr<ProcessingAlgorithm>()>> creators_;
};

} // namespace algorithms
} // namespace dataflow
