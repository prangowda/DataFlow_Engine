#include "../../include/algorithms/ProcessingAlgorithm.hpp"
#include "../../include/core/DataPacket.hpp"
#include <iostream>
#include <complex>
#include <cmath>

namespace dataflow {
namespace algorithms {

// Static factory registry
std::unordered_map<std::string, 
    std::function<std::shared_ptr<ProcessingAlgorithm>()>> AlgorithmFactory::creators_;

std::shared_ptr<ProcessingAlgorithm> AlgorithmFactory::create(const std::string& name) {
    // Check if we have a custom creator for this algorithm
    auto it = creators_.find(name);
    if (it != creators_.end()) {
        return it->second();
    }
    
    // If not, check built-in algorithms
    if (name == "FFT") {
        return std::make_shared<FFTAlgorithm>();
    } else if (name == "ImageProcessing") {
        return std::make_shared<ImageProcessingAlgorithm>();
    } else if (name == "MachineLearning") {
        return std::make_shared<MachineLearningAlgorithm>();
    }
    
    // Unknown algorithm
    throw std::invalid_argument("Unknown algorithm: " + name);
}

void AlgorithmFactory::registerAlgorithm(
    const std::string& name,
    std::function<std::shared_ptr<ProcessingAlgorithm>()> creator) {
    
    creators_[name] = std::move(creator);
}

// ----- FFTAlgorithm Implementation -----

FFTAlgorithm::FFTAlgorithm() 
    : windowSize_(1024), useHanning_(true) {}

std::shared_ptr<core::DataPacket> FFTAlgorithm::process(std::shared_ptr<core::DataPacket> input) {
    // Check for time domain signal
    auto timeSignal = input->getData<std::vector<double>>("time_signal");
    if (!timeSignal) {
        throw std::invalid_argument("FFT algorithm requires 'time_signal' data");
    }
    
    // Create output packet
    auto output = std::make_shared<core::DataPacket>();
    
    // Copy metadata from input
    for (const auto& key : input->getKeys()) {
        if (key != "time_signal" && key != "frequency_domain") {
            auto anyValue = input->getData<std::any>(key);
            if (anyValue) {
                output->setData(key, *anyValue);
            }
        }
    }
    
    // Resize input if needed
    std::vector<double> processedInput = *timeSignal;
    if (processedInput.size() > windowSize_) {
        processedInput.resize(windowSize_);
    } else if (processedInput.size() < windowSize_) {
        processedInput.resize(windowSize_, 0.0);
    }
    
    // Apply Hanning window if enabled
    if (useHanning_) {
        for (size_t i = 0; i < processedInput.size(); ++i) {
            double multiplier = 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (processedInput.size() - 1)));
            processedInput[i] *= multiplier;
        }
    }
    
    // Perform FFT
    std::vector<std::complex<double>> freqDomain;
    performFFT(processedInput, freqDomain);
    
    // Store result in output packet
    output->setData("frequency_domain", freqDomain);
    
    // Calculate magnitude spectrum
    std::vector<double> magnitudes(freqDomain.size() / 2);
    for (size_t i = 0; i < magnitudes.size(); ++i) {
        magnitudes[i] = std::abs(freqDomain[i]);
    }
    output->setData("magnitude_spectrum", magnitudes);
    
    return output;
}

void FFTAlgorithm::configure(const std::unordered_map<std::string, std::string>& params) {
    for (const auto& param : params) {
        if (param.first == "window_size") {
            windowSize_ = std::stoul(param.second);
        } else if (param.first == "use_hanning") {
            useHanning_ = (param.second == "true" || param.second == "1");
        }
    }
}

void FFTAlgorithm::performFFT(const std::vector<double>& input, std::vector<std::complex<double>>& output) {
    // Simplified FFT implementation (not efficient, would use a library in practice)
    const size_t N = input.size();
    output.resize(N);
    
    // Direct DFT implementation (O(n²) - would use FFT O(n log n) in practice)
    for (size_t k = 0; k < N; ++k) {
        output[k] = std::complex<double>(0, 0);
        for (size_t n = 0; n < N; ++n) {
            double angle = -2.0 * M_PI * k * n / N;
            output[k] += input[n] * std::complex<double>(std::cos(angle), std::sin(angle));
        }
    }
}

// ----- ImageProcessingAlgorithm Implementation -----

ImageProcessingAlgorithm::ImageProcessingAlgorithm() 
    : filterType_(FilterType::Blur), kernelSize_(3) {}

std::shared_ptr<core::DataPacket> ImageProcessingAlgorithm::process(std::shared_ptr<core::DataPacket> input) {
    // Check for image data
    auto imageData = input->getData<std::vector<uint8_t>>("image_data");
    auto width = input->getData<int>("image_width");
    auto height = input->getData<int>("image_height");
    auto channels = input->getData<int>("image_channels");
    
    if (!imageData || !width || !height || !channels) {
        throw std::invalid_argument("Image processing requires image data, width, height, and channels");
    }
    
    // Create output packet
    auto output = std::make_shared<core::DataPacket>();
    
    // Copy metadata
    for (const auto& key : input->getKeys()) {
        if (key != "image_data" && key != "processed_image") {
            auto anyValue = input->getData<std::any>(key);
            if (anyValue) {
                output->setData(key, *anyValue);
            }
        }
    }
    
    // Process the image
    std::vector<uint8_t> processedImage;
    applyFilter(*imageData, processedImage, *width, *height, *channels);
    
    // Store result
    output->setData("processed_image", processedImage);
    output->setData("filter_type", static_cast<int>(filterType_));
    output->setData("kernel_size", kernelSize_);
    
    return output;
}

void ImageProcessingAlgorithm::configure(const std::unordered_map<std::string, std::string>& params) {
    for (const auto& param : params) {
        if (param.first == "filter_type") {
            int type = std::stoi(param.second);
            switch (type) {
                case 0: filterType_ = FilterType::Blur; break;
                case 1: filterType_ = FilterType::Sharpen; break;
                case 2: filterType_ = FilterType::EdgeDetection; break;
                default: throw std::invalid_argument("Invalid filter type");
            }
        } else if (param.first == "kernel_size") {
            kernelSize_ = std::stoi(param.second);
            // Ensure odd kernel size
            if (kernelSize_ % 2 == 0) {
                ++kernelSize_;
            }
        }
    }
}

void ImageProcessingAlgorithm::applyFilter(const std::vector<uint8_t>& input, std::vector<uint8_t>& output,
                                        int width, int height, int channels) {
    // Simple implementation (would use optimized libraries in practice)
    output = input; // Start with a copy
    
    // In a real implementation, we would use different kernels based on filterType_
    // and apply convolution with appropriate border handling
    // This is a simplified placeholder
    
    std::cout << "Applying " << (filterType_ == FilterType::Blur ? "blur" : 
                              filterType_ == FilterType::Sharpen ? "sharpen" : "edge detection")
              << " filter with kernel size " << kernelSize_ << std::endl;
}

// ----- MachineLearningAlgorithm Implementation -----

MachineLearningAlgorithm::MachineLearningAlgorithm() 
    : confidenceThreshold_(0.5) {}

std::shared_ptr<core::DataPacket> MachineLearningAlgorithm::process(std::shared_ptr<core::DataPacket> input) {
    // Check for feature data
    auto features = input->getData<std::vector<float>>("features");
    if (!features) {
        throw std::invalid_argument("ML algorithm requires 'features' data");
    }
    
    // Create output packet
    auto output = std::make_shared<core::DataPacket>();
    
    // Copy metadata
    for (const auto& key : input->getKeys()) {
        if (key != "features" && key != "predictions") {
            auto anyValue = input->getData<std::any>(key);
            if (anyValue) {
                output->setData(key, *anyValue);
            }
        }
    }
    
    // Run inference
    auto predictions = runInference(*features);
    
    // Store results
    output->setData("predictions", predictions);
    output->setData("model_path", modelPath_);
    output->setData("confidence_threshold", confidenceThreshold_);
    
    return output;
}

void MachineLearningAlgorithm::configure(const std::unordered_map<std::string, std::string>& params) {
    for (const auto& param : params) {
        if (param.first == "model_path") {
            modelPath_ = param.second;
            loadModel(modelPath_);
        } else if (param.first == "confidence_threshold") {
            confidenceThreshold_ = std::stof(param.second);
        }
    }
}

void MachineLearningAlgorithm::loadModel(const std::string& path) {
    // In a real implementation, this would load a model from a file
    // using a machine learning framework like TensorFlow, PyTorch, or ONNX Runtime
    std::cout << "Loading ML model from: " << path << std::endl;
}

std::vector<float> MachineLearningAlgorithm::runInference(const std::vector<float>& input) {
    // In a real implementation, this would run inference using the loaded model
    // This is a simplified placeholder that returns random values
    std::vector<float> results(10); // Assume 10 output classes
    
    for (auto& val : results) {
        val = static_cast<float>(rand()) / RAND_MAX;
    }
    
    return results;
}

} // namespace algorithms
} // namespace dataflow
