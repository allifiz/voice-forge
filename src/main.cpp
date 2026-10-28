// VoiceForge Lite v0.1.2: experimental CPU-only voice effect.
// This is DSP pitch/formant processing, not AI voice conversion.
// NOMINMAX must be defined before miniaudio/Windows includes.
#if defined(_WIN32) && !defined(NOMINMAX)
#define NOMINMAX
#endif
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "signalsmith-stretch.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {
constexpr int kSampleRate = 48000;
constexpr int kProcessingChunk = 480; // 10 ms processing block, not total latency.

struct Settings {
    bool listDevices = false;
    bool benchmark = false;
    int inputIndex = -1;
    int outputIndex = -1;
    float pitchSemitones = 5.0f;
    float formantFactor = 1.16f;
    float gain = 0.85f;
};

void usage() {
    std::cout << R"(VoiceForge Lite v0.1.2 | CPU-only experiment

Usage:
  voiceforge --list-devices
  voiceforge --benchmark [--pitch 5] [--formant 1.16]
  voiceforge [--input N] [--output N] [--pitch N] [--formant N] [--gain N]

Options:
  --input N       Capture device index (default: OS default microphone)
  --output N      Playback device index (default: OS default speaker)
  --pitch N       Pitch semitones, -6..+10 (default: +5)
  --formant N     Formant scale, 0.8..1.5 (default: 1.16)
  --gain N        Output gain, 0..2 (default: 0.85)
  --list-devices  Display input/output device indexes
  --benchmark     Estimate processing load without a soundcard
  --help          Show help

For Discord, send audio to VB-CABLE's playback device "CABLE Input",
then choose recording device "CABLE Output" as Discord's microphone.
Use headphones. Press Enter to stop the real-time stream.
)";
}

int asIndex(const std::string& text) {
    std::size_t used = 0;
    const long n = std::stol(text, &used);
    if (used != text.size() || n < 0 || n > std::numeric_limits<int>::max()) {
        throw std::invalid_argument("invalid device index");
    }
    return static_cast<int>(n);
}

float asFloat(const std::string& text) {
    std::size_t used = 0;
    const float v = std::stof(text, &used);
    if (used != text.size() || !std::isfinite(v)) {
        throw std::invalid_argument("invalid number");
    }
    return v;
}

Settings parseArgs(int argc, char** argv) {
    Settings s;
    for (int i = 1; i < argc; ++i) {
        const std::string option(argv[i]);
        if (option == "--help" || option == "-h") {
            usage();
            std::exit(0);
        }
        if (option == "--list-devices") { s.listDevices = true; continue; }
        if (option == "--benchmark") { s.benchmark = true; continue; }
        if (option != "--input" && option != "--output" &&
            option != "--pitch" && option != "--formant" &&
            option != "--gain") {
            throw std::invalid_argument("unknown option: " + option);
        }
        if (i + 1 >= argc) throw std::invalid_argument("missing value for " + option);
        const std::string value(argv[++i]);
        if (option == "--input") s.inputIndex = asIndex(value);
        else if (option == "--output") s.outputIndex = asIndex(value);
        else if (option == "--pitch") s.pitchSemitones = asFloat(value);
        else if (option == "--formant") s.formantFactor = asFloat(value);
        else if (option == "--gain") s.gain = asFloat(value);
    }
    if (s.pitchSemitones < -6 || s.pitchSemitones > 10 ||
        s.formantFactor < 0.8f || s.formantFactor > 1.5f ||
        s.gain < 0 || s.gain > 2) {
        throw std::invalid_argument("pitch, formant or gain outside supported range");
    }
    return s;
}

class VoiceEngine {
public:
    explicit VoiceEngine(const Settings& settings) : gain_(settings.gain) {
        // 40 ms analysis window and 10 ms interval; latency not guaranteed.
        stretch_.configure(1, 1920, 480, true);
        stretch_.setTransposeSemitones(settings.pitchSemitones);
        stretch_.setFormantFactor(settings.formantFactor, true);
        stretch_.setFormantBase(130.0f / kSampleRate);
    }

    // Only call from one thread; no console I/O or heap allocations here.
    void process(const float* input, float* output, int count) {
        std::array<float, kProcessingChunk> scratchIn{};
        std::array<float, kProcessingChunk> scratchOut{};
        for (int offset = 0; offset < count; offset += kProcessingChunk) {
            const int n = std::min(kProcessingChunk, count - offset);
            for (int j = 0; j < n; ++j) {
                scratchIn[j] = input ? input[offset + j] : 0.0f;
            }
            float* inChannels[1] = {scratchIn.data()};
            float* outChannels[1] = {scratchOut.data()};
            stretch_.process(inChannels, n, outChannels, n);
            for (int j = 0; j < n; ++j) {
                const float x = scratchOut[j] * gain_;
                output[offset + j] = std::clamp(x, -0.98f, 0.98f);
            }
        }
    }

    int algorithmDelaySamples() const {
        return stretch_.inputLatency() + stretch_.outputLatency();
    }

private:
    signalsmith::stretch::SignalsmithStretch<float> stretch_;
    float gain_;
};

void dataCallback(ma_device* device, void* output, const void* input, ma_uint32 frameCount) {
    auto* engine = static_cast<VoiceEngine*>(device->pUserData);
    if (!engine || !output) return;
    // Requested format is mono float32 at 48 kHz.
    engine->process(static_cast<const float*>(input),
                    static_cast<float*>(output),
                    static_cast<int>(frameCount));
}

void listDevices(const ma_device_info* playbacks, ma_uint32 playbackCount,
                 const ma_device_info* captures, ma_uint32 captureCount) {
    std::cout << "INPUT DEVICES (for --input):\n";
    for (ma_uint32 i = 0; i < captureCount; ++i) {
        std::cout << "  " << i << ": " << captures[i].name
                  << (captures[i].isDefault ? " [default]" : "") << '\n';
    }
    std::cout << "OUTPUT DEVICES (for --output):\n";
    for (ma_uint32 i = 0; i < playbackCount; ++i) {
        std::cout << "  " << i << ": " << playbacks[i].name
                  << (playbacks[i].isDefault ? " [default]" : "") << '\n';
    }
}

void benchmark(const Settings& settings) {
    VoiceEngine engine(settings);
    std::array<float, kProcessingChunk> input{}, output{};
    constexpr int kBlocks = 1000; // 10 seconds of synthesized tones
    double meanMs = 0.0;
    double maxMs = 0.0;
    const auto start = std::chrono::steady_clock::now();
    for (int block = 0; block < kBlocks; ++block) {
        for (int i = 0; i < kProcessingChunk; ++i) {
            const float t = (block * kProcessingChunk + i) / static_cast<float>(kSampleRate);
            input[i] = 0.2f * std::sin(2.0f * 3.14159265f * 125.0f * t) +
                       0.1f * std::sin(2.0f * 3.14159265f * 250.0f * t);
        }
        const auto begin = std::chrono::steady_clock::now();
        engine.process(input.data(), output.data(), kProcessingChunk);
        const auto end = std::chrono::steady_clock::now();
        const double elapsed = std::chrono::duration<double, std::milli>(end - begin).count();
        meanMs += elapsed;
        maxMs = std::max(maxMs, elapsed);
    }
    const double wallMs = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - start).count();
    std::cout << std::fixed << std::setprecision(2)
              << "Synthetic audio: 10 seconds (48kHz mono)\n"
              << "Mean compute time for 10ms block: " << meanMs / kBlocks << " ms\n"
              << "Max compute time for 10ms block:  " << maxMs << " ms\n"
              << "Total wall time:                  " << wallMs << " ms\n"
              << "DSP latency estimate:            "
              << (1000.0 * engine.algorithmDelaySamples() / kSampleRate) << " ms\n"
              << "Driver, microphone and virtual-cable latency NOT included.\n";
}

int run(const Settings& settings) {
    if (settings.benchmark) {
        benchmark(settings);
        return 0;
    }

    ma_context context{};
    if (ma_context_init(nullptr, 0, nullptr, &context) != MA_SUCCESS) {
        throw std::runtime_error("could not initialize audio backend");
    }

    ma_device_info* playbacks = nullptr;
    ma_device_info* captures = nullptr;
    ma_uint32 playbackCount = 0;
    ma_uint32 captureCount = 0;
    const ma_result enumeration = ma_context_get_devices(
        &context, &playbacks, &playbackCount, &captures, &captureCount);

    if (enumeration != MA_SUCCESS) {
        ma_context_uninit(&context);
        throw std::runtime_error("could not enumerate audio devices");
    }
    if (settings.listDevices) {
        listDevices(playbacks, playbackCount, captures, captureCount);
        ma_context_uninit(&context);
        return 0;
    }
    if ((settings.inputIndex >= 0 &&
         static_cast<ma_uint32>(settings.inputIndex) >= captureCount) ||
        (settings.outputIndex >= 0 &&
         static_cast<ma_uint32>(settings.outputIndex) >= playbackCount)) {
        ma_context_uninit(&context);
        throw std::runtime_error("device index out of range; run --list-devices");
    }

    VoiceEngine engine(settings);
    ma_device_config config = ma_device_config_init(ma_device_type_duplex);
    config.sampleRate = kSampleRate;
    config.periodSizeInFrames = kProcessingChunk;
    config.capture.format = ma_format_f32;
    config.capture.channels = 1;
    config.playback.format = ma_format_f32;
    config.playback.channels = 1;
    if (settings.inputIndex >= 0) {
        config.capture.pDeviceID = &captures[settings.inputIndex].id;
    }
    if (settings.outputIndex >= 0) {
        config.playback.pDeviceID = &playbacks[settings.outputIndex].id;
    }
    config.dataCallback = dataCallback;
    config.pUserData = &engine;

    ma_device device{};
    if (ma_device_init(&context, &config, &device) != MA_SUCCESS) {
        ma_context_uninit(&context);
        throw std::runtime_error("could not open microphone/output device");
    }
    if (ma_device_start(&device) != MA_SUCCESS) {
        ma_device_uninit(&device);
        ma_context_uninit(&context);
        throw std::runtime_error("failed to start audio stream");
    }

    std::cout << "VoiceForge active; pitch " << settings.pitchSemitones
              << " semitones, formant " << settings.formantFactor << "x\n"
              << "DSP latency estimate: "
              << (1000.0 * engine.algorithmDelaySamples() / kSampleRate)
              << " ms (not end-to-end)\n"
              << "Press Enter to stop. Use headphones to prevent feedback.\n";
    std::string line;
    std::getline(std::cin, line);

    ma_device_uninit(&device);
    ma_context_uninit(&context);
    return 0;
}
} // namespace

int main(int argc, char** argv) {
    try {
        return run(parseArgs(argc, argv));
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        usage();
        return 1;
    }
}
