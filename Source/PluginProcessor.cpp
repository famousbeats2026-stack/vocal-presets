#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"
#include <cmath>

namespace
{
    // Scale pitch-class sets, relative to the key (0 = key note).
    constexpr int kScaleDegrees[5][12] = {
        { 0, 2, 4, 5, 7, 9, 11, -1, -1, -1, -1, -1 }, // Major
        { 0, 2, 3, 5, 7, 8, 10, -1, -1, -1, -1, -1 }, // Natural minor
        { 0, 2, 4, 7, 9, -1, -1, -1, -1, -1, -1, -1 }, // Major pentatonic
        { 0, 3, 5, 7, 10, -1, -1, -1, -1, -1, -1, -1 }, // Minor pentatonic
        { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 },       // Chromatic
    };
    constexpr int kScaleLengths[5] = { 7, 7, 5, 5, 12 };

    bool pitchClassInScale(int pc, int scale)
    {
        for (int i = 0; i < kScaleLengths[scale]; ++i)
            if (kScaleDegrees[scale][i] == pc)
                return true;
        return false;
    }

    float midiToFreq(int midi) { return 440.0f * std::pow(2.0f, (midi - 69) / 12.0f); }
}

//==============================================================================
FamousBeatsVocalPresetsProcessor::FamousBeatsVocalPresetsProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "Parameters",
                 {
                     std::make_unique<juce::AudioParameterBool>(
                         kBypassParam, "Bypass", false),
                     std::make_unique<juce::AudioParameterFloat>(
                         kInputGainParam, "Input Gain",
                         juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f,
                         juce::AudioParameterFloatAttributes().withLabel("dB")),
                     std::make_unique<juce::AudioParameterFloat>(
                         kOutputGainParam, "Output Gain",
                         juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f,
                         juce::AudioParameterFloatAttributes().withLabel("dB")),

                     std::make_unique<juce::AudioParameterFloat>(
                         kCleanAmountParam, "Cleaner Amount",
                         juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 80.0f,
                         juce::AudioParameterFloatAttributes().withLabel("%")),

                     std::make_unique<juce::AudioParameterFloat>(
                         kEqAmountParam, "EQ Amount",
                         juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f,
                         juce::AudioParameterFloatAttributes().withLabel("%")),
                     std::make_unique<juce::AudioParameterFloat>(
                         kHpFreqParam, "High Pass",
                         juce::NormalisableRange<float>(40.0f, 200.0f, 1.0f), 90.0f,
                         juce::AudioParameterFloatAttributes().withLabel("Hz")),
                     std::make_unique<juce::AudioParameterFloat>(
                         kMudGainParam, "Mud Cut",
                         juce::NormalisableRange<float>(-12.0f, 6.0f, 0.1f), -3.5f,
                         juce::AudioParameterFloatAttributes().withLabel("dB")),
                     std::make_unique<juce::AudioParameterFloat>(
                         kMudFreqParam, "Mud Freq",
                         juce::NormalisableRange<float>(150.0f, 600.0f, 1.0f), 320.0f,
                         juce::AudioParameterFloatAttributes().withLabel("Hz")),
                     std::make_unique<juce::AudioParameterFloat>(
                         kPresGainParam, "Presence",
                         juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 3.0f,
                         juce::AudioParameterFloatAttributes().withLabel("dB")),
                     std::make_unique<juce::AudioParameterFloat>(
                         kPresFreqParam, "Presence Freq",
                         juce::NormalisableRange<float>(2000.0f, 6000.0f, 10.0f), 4500.0f,
                         juce::AudioParameterFloatAttributes().withLabel("Hz")),
                     std::make_unique<juce::AudioParameterFloat>(
                         kAirGainParam, "Air",
                         juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 3.0f,
                         juce::AudioParameterFloatAttributes().withLabel("dB")),
                     std::make_unique<juce::AudioParameterFloat>(
                         kAirFreqParam, "Air Freq",
                         juce::NormalisableRange<float>(8000.0f, 14000.0f, 10.0f), 10000.0f,
                         juce::AudioParameterFloatAttributes().withLabel("Hz")),

                     std::make_unique<juce::AudioParameterFloat>(
                         kCompThreshParam, "Comp Threshold",
                         juce::NormalisableRange<float>(-40.0f, 0.0f, 0.5f), -18.0f,
                         juce::AudioParameterFloatAttributes().withLabel("dB")),
                     std::make_unique<juce::AudioParameterFloat>(
                         kCompRatioParam, "Comp Ratio",
                         juce::NormalisableRange<float>(1.0f, 10.0f, 0.1f), 3.5f,
                         juce::AudioParameterFloatAttributes().withLabel(":1")),
                     std::make_unique<juce::AudioParameterFloat>(
                         kCompAttackParam, "Comp Attack",
                         juce::NormalisableRange<float>(0.1f, 100.0f, 0.1f), 8.0f,
                         juce::AudioParameterFloatAttributes().withLabel("ms")),
                     std::make_unique<juce::AudioParameterFloat>(
                         kCompReleaseParam, "Comp Release",
                         juce::NormalisableRange<float>(20.0f, 500.0f, 1.0f), 120.0f,
                         juce::AudioParameterFloatAttributes().withLabel("ms")),
                     std::make_unique<juce::AudioParameterFloat>(
                         kCompMixParam, "Comp Mix",
                         juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f,
                         juce::AudioParameterFloatAttributes().withLabel("%")),

                     std::make_unique<juce::AudioParameterFloat>(
                         kSatDriveParam, "Saturation Drive",
                         juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 10.0f,
                         juce::AudioParameterFloatAttributes().withLabel("%")),
                     std::make_unique<juce::AudioParameterFloat>(
                         kSatMixParam, "Saturation Mix",
                         juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 60.0f,
                         juce::AudioParameterFloatAttributes().withLabel("%")),

                     std::make_unique<juce::AudioParameterFloat>(
                         kTuneAmountParam, "Tune Amount",
                         juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 40.0f,
                         juce::AudioParameterFloatAttributes().withLabel("%")),
                     std::make_unique<juce::AudioParameterFloat>(
                         kTuneSpeedParam, "Retune Speed",
                         juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 60.0f,
                         juce::AudioParameterFloatAttributes().withLabel("%")),
                     std::make_unique<juce::AudioParameterChoice>(
                         kTuneKeyParam, "Tune Key",
                         juce::StringArray({ "C", "C#", "D", "D#", "E", "F",
                                             "F#", "G", "G#", "A", "A#", "B" }),
                         0),
                     std::make_unique<juce::AudioParameterChoice>(
                         kTuneScaleParam, "Tune Scale",
                         juce::StringArray({ "Major", "Minor", "Major Pent",
                                             "Minor Pent", "Chromatic" }),
                         0),

                     std::make_unique<juce::AudioParameterFloat>(
                         kReverbMixParam, "Reverb Mix",
                         juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 18.0f,
                         juce::AudioParameterFloatAttributes().withLabel("%")),
                     std::make_unique<juce::AudioParameterFloat>(
                         kReverbSizeParam, "Reverb Size",
                         juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 40.0f,
                         juce::AudioParameterFloatAttributes().withLabel("%")),

                     std::make_unique<juce::AudioParameterFloat>(
                         kDelayTimeParam, "Delay Time",
                         juce::NormalisableRange<float>(0.0f, 500.0f, 1.0f), 220.0f,
                         juce::AudioParameterFloatAttributes().withLabel("ms")),
                     std::make_unique<juce::AudioParameterFloat>(
                         kDelayFeedbackParam, "Delay Feedback",
                         juce::NormalisableRange<float>(0.0f, 85.0f, 0.1f), 30.0f,
                         juce::AudioParameterFloatAttributes().withLabel("%")),
                     std::make_unique<juce::AudioParameterFloat>(
                         kDelayMixParam, "Delay Mix",
                         juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 12.0f,
                         juce::AudioParameterFloatAttributes().withLabel("%")),
                 })
{
    bypassParam = parameters.getRawParameterValue(kBypassParam);
    inputGainParam = parameters.getRawParameterValue(kInputGainParam);
    outputGainParam = parameters.getRawParameterValue(kOutputGainParam);
    cleanAmountParam = parameters.getRawParameterValue(kCleanAmountParam);
    eqAmountParam = parameters.getRawParameterValue(kEqAmountParam);
    hpFreqParam = parameters.getRawParameterValue(kHpFreqParam);
    mudGainParam = parameters.getRawParameterValue(kMudGainParam);
    mudFreqParam = parameters.getRawParameterValue(kMudFreqParam);
    presGainParam = parameters.getRawParameterValue(kPresGainParam);
    presFreqParam = parameters.getRawParameterValue(kPresFreqParam);
    airGainParam = parameters.getRawParameterValue(kAirGainParam);
    airFreqParam = parameters.getRawParameterValue(kAirFreqParam);
    compThreshParam = parameters.getRawParameterValue(kCompThreshParam);
    compRatioParam = parameters.getRawParameterValue(kCompRatioParam);
    compAttackParam = parameters.getRawParameterValue(kCompAttackParam);
    compReleaseParam = parameters.getRawParameterValue(kCompReleaseParam);
    compMixParam = parameters.getRawParameterValue(kCompMixParam);
    satDriveParam = parameters.getRawParameterValue(kSatDriveParam);
    satMixParam = parameters.getRawParameterValue(kSatMixParam);
    tuneAmountParam = parameters.getRawParameterValue(kTuneAmountParam);
    tuneSpeedParam = parameters.getRawParameterValue(kTuneSpeedParam);
    tuneKeyParam = parameters.getRawParameterValue(kTuneKeyParam);
    tuneScaleParam = parameters.getRawParameterValue(kTuneScaleParam);
    reverbMixParam = parameters.getRawParameterValue(kReverbMixParam);
    reverbSizeParam = parameters.getRawParameterValue(kReverbSizeParam);
    delayTimeParam = parameters.getRawParameterValue(kDelayTimeParam);
    delayFeedbackParam = parameters.getRawParameterValue(kDelayFeedbackParam);
    delayMixParam = parameters.getRawParameterValue(kDelayMixParam);
}

FamousBeatsVocalPresetsProcessor::~FamousBeatsVocalPresetsProcessor()
{
    freeDenoisers();
    if (sharedModel != nullptr)
    {
        rnnoise_model_free(sharedModel);
        sharedModel = nullptr;
    }
}

//==============================================================================
// RNNoise model: the embedded "sh" model (sh.rnnn) is stronger on hiss than
// the library's built-in 2017 model. rnnoise_model_from_file() needs a
// FILE*, so the embedded bytes are written to a temp file once. Any failure
// falls back to the built-in model.
RNNModel* FamousBeatsVocalPresetsProcessor::loadEmbeddedModel()
{
    auto tempFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                        .getChildFile("fbs_vocalpresets_sh.rnnn");

    if (!tempFile.existsAsFile() || tempFile.getSize() != BinaryData::sh_rnnnSize)
    {
        if (!tempFile.replaceWithData(BinaryData::sh_rnnn, (size_t)BinaryData::sh_rnnnSize))
            return nullptr;
    }

    FILE* f = fopen(tempFile.getFullPathName().toRawUTF8(), "r");
    if (f == nullptr)
        return nullptr;

    RNNModel* model = rnnoise_model_from_file(f);
    fclose(f);
    return model; // may be nullptr if the file was unreadable
}

void FamousBeatsVocalPresetsProcessor::initDenoisers()
{
    freeDenoisers();

    if (!modelTried)
    {
        sharedModel = loadEmbeddedModel();
        modelTried = true;
    }

    const int numCh = (int)cleanerChannels.size();
    for (int ch = 0; ch < numCh; ++ch)
        cleanerChannels[(size_t)ch].denoiser = rnnoise_create(sharedModel);
}

void FamousBeatsVocalPresetsProcessor::freeDenoisers()
{
    for (auto& ch : cleanerChannels)
    {
        if (ch.denoiser != nullptr)
        {
            rnnoise_destroy(ch.denoiser);
            ch.denoiser = nullptr;
        }
    }
}

void FamousBeatsVocalPresetsProcessor::resetCleanerState()
{
    for (auto& ch : cleanerChannels)
    {
        ch.to48k.reset((double)kRnnoiseRate / hostRate);
        ch.from48k.reset(hostRate / (double)kRnnoiseRate);
        ch.in48k.clear();
        ch.out48k.clear();
        ch.pendingWet.clear();
        ch.dryDelay.assign((size_t)juce::jmax(1, cleanerLatency), 0.0f);
        ch.dryDelayPos = 0;
        std::fill(std::begin(ch.frameIn), std::end(ch.frameIn), 0.0f);
        std::fill(std::begin(ch.frameOut), std::end(ch.frameOut), 0.0f);
    }
}

//==============================================================================
void FamousBeatsVocalPresetsProcessor::LinearResampler::reset(double r)
{
    ratio = r;
    readPos = 0.0;
    history.clear();
}

void FamousBeatsVocalPresetsProcessor::LinearResampler::process(const float* input, int numSamples,
                                                                std::vector<float>& out)
{
    for (int i = 0; i < numSamples; ++i)
        history.push_back(input[i]);

    out.clear();
    const double step = 1.0 / ratio;
    while (true)
    {
        const int idx = (int)std::floor(readPos);
        const double frac = readPos - idx;
        if (idx + 1 >= (int)history.size())
            break;
        const float s0 = history[(size_t)idx];
        const float s1 = history[(size_t)idx + 1];
        out.push_back(s0 + (float)((s1 - s0) * frac));
        readPos += step;
    }

    const int drop = (int)std::floor(readPos);
    for (int i = 0; i < drop; ++i)
        history.pop_front();
    readPos -= drop;
}

//==============================================================================
// Pitch shifter: phase vocoder (STFT -> shift the spectrum -> resynthesise).
// A granular delay-line shifter was tried first and REJECTED: its
// grain-boundary phase jumps cancel the shift on average, so the output
// stayed at the input pitch (measured: 233 Hz in, 233 Hz out at ratio
// 1.0565). The phase vocoder is the proven approach. Analysis: 2048-point
// Hann window, hop 512. For each bin the true frequency is recovered from
// the phase deviation from the expected advance; the output spectrum is
// the input spectrum resampled by 1/ratio, and each output bin's phase is
// advanced by its (shifted) true frequency. Resynthesis is overlap-add
// with a Hann window; Hann^2 at 75% overlap sums to 1.5, so the output
// is unity gain (JUCE's inverse FFT already divides by N; the
// analysis Hann halves the amplitude and the synthesis Hann
// overlap-add at 75% overlap sums to 2.0).
// Output latency is one FFT frame (2048 samples); the dry path is
// delayed by the same amount so the tune mix stays in time.
void FamousBeatsVocalPresetsProcessor::PhaseVocoder::prepare(double)
{
    fft = std::make_unique<juce::dsp::FFT>(11); // 2^11 = 2048
    inRing.assign(8192, 0.0f);
    outRing.assign(8192, 0.0f);
    window.assign((size_t)kFftSize, 0.0f);
    for (int i = 0; i < kFftSize; ++i)
        window[(size_t)i] = 0.5f - 0.5f * std::cos(2.0f * juce::MathConstants<float>::pi * i / (kFftSize - 1));
    prevPhase.assign((size_t)kNumBins, 0.0f);
    synthPhase.assign((size_t)kNumBins, 0.0f);
    mag.assign((size_t)kNumBins, 0.0f);
    trueFreq.assign((size_t)kNumBins, 0.0f);
    fftBuf.assign((size_t)kFftSize, {});
    specBuf.assign((size_t)kFftSize, {});
    outSpec.assign((size_t)kFftSize, {});
    timeBuf.assign((size_t)kFftSize, {});
    reset();
}

void FamousBeatsVocalPresetsProcessor::PhaseVocoder::reset()
{
    std::fill(inRing.begin(), inRing.end(), 0.0f);
    std::fill(outRing.begin(), outRing.end(), 0.0f);
    std::fill(prevPhase.begin(), prevPhase.end(), 0.0f);
    std::fill(synthPhase.begin(), synthPhase.end(), 0.0f);
    inCount = 0;
    samplesSinceFrame = 0;
}

void FamousBeatsVocalPresetsProcessor::PhaseVocoder::process(float* data, int numSamples, float ratio)
{
    const int ringSize = (int)inRing.size();
    for (int n = 0; n < numSamples; ++n)
    {
        inRing[(size_t)(inCount % ringSize)] = data[n];
        ++inCount;
        ++samplesSinceFrame;
        if (samplesSinceFrame >= kHop)
        {
            samplesSinceFrame -= kHop;
            processFrame(ratio);
        }
        // Output is delayed by one frame: read the overlap-add ring at
        // (inCount - kFftSize) and clear it (it is an accumulator).
        const int outPos = ((inCount - kFftSize) % ringSize + ringSize) % ringSize;
        data[n] = outRing[(size_t)outPos];
        outRing[(size_t)outPos] = 0.0f;
    }
}

void FamousBeatsVocalPresetsProcessor::PhaseVocoder::processFrame(float ratio)
{
    const int ringSize = (int)inRing.size();

    // Gather the latest kFftSize input samples, Hann-windowed.
    for (int i = 0; i < kFftSize; ++i)
    {
        const int idx = ((inCount - kFftSize + i) % ringSize + ringSize) % ringSize;
        fftBuf[(size_t)i] = { inRing[(size_t)idx] * window[(size_t)i], 0.0f };
    }
    // NOTE: JUCE's FFT::perform does NOT support in-place operation
    // (same buffer for input and output corrupts the transform), so
    // the forward and inverse transforms each use separate buffers.
    fft->perform(fftBuf.data(), specBuf.data(), false);

    // Magnitude and true frequency (in bins) of bins 0..N/2.
    const float expectedAdvance = 2.0f * juce::MathConstants<float>::pi * (float)kHop / (float)kFftSize;
    const float binsPerRad = (float)kFftSize / (2.0f * juce::MathConstants<float>::pi * (float)kHop);
    for (int k = 0; k < kNumBins; ++k)
    {
        const auto c = specBuf[(size_t)k];
        mag[(size_t)k] = std::abs(c);
        const float phase = std::arg(c);
        float dev = phase - prevPhase[(size_t)k] - expectedAdvance * (float)k;
        dev = std::atan2(std::sin(dev), std::cos(dev)); // wrap to [-pi, pi]
        trueFreq[(size_t)k] = (float)k + dev * binsPerRad;
        prevPhase[(size_t)k] = phase;
    }

    // Build the shifted spectrum: output bin j takes magnitude and true
    // frequency from input bin j/ratio, and its phase advances by the
    // shifted true frequency.
    const float phaseStep = 2.0f * juce::MathConstants<float>::pi * (float)kHop / (float)kFftSize;
    for (int j = 0; j < kNumBins; ++j)
    {
        const float src = (float)j / ratio;
        if (src > (float)(kNumBins - 1))
        {
            outSpec[(size_t)j] = { 0.0f, 0.0f };
            if (j > 0 && j < kFftSize / 2)
                outSpec[(size_t)(kFftSize - j)] = { 0.0f, 0.0f };
            continue;
        }
        const int i0 = (int)std::floor(src);
        const int i1 = juce::jmin(i0 + 1, kNumBins - 1);
        const float frac = src - (float)i0;
        const float m = mag[(size_t)i0] * (1.0f - frac) + mag[(size_t)i1] * frac;
        const float tf = trueFreq[(size_t)i0] * (1.0f - frac) + trueFreq[(size_t)i1] * frac;
        synthPhase[(size_t)j] += phaseStep * tf * ratio;
        const float re = m * std::cos(synthPhase[(size_t)j]);
        const float im = m * std::sin(synthPhase[(size_t)j]);
        outSpec[(size_t)j] = { re, im };
        if (j > 0 && j < kFftSize / 2)
            outSpec[(size_t)(kFftSize - j)] = { re, -im };
    }
    // DC and Nyquist are real and do not shift.
    outSpec[0] = { mag[0], 0.0f };
    outSpec[(size_t)(kFftSize / 2)] = { mag[(size_t)(kNumBins - 1)], 0.0f };

    fft->perform(outSpec.data(), timeBuf.data(), true);

    // Overlap-add into the output ring at the frame's time position,
    // with the synthesis Hann window. No extra scaling: JUCE's inverse
    // FFT already divides by N, the analysis Hann halves the amplitude
    // (A/2), and the synthesis Hann overlap-add at 75% overlap sums to
    // 2.0, restoring unity gain.
    const int frameStart = inCount - kFftSize;
    for (int i = 0; i < kFftSize; ++i)
    {
        const int idx = ((frameStart + i) % ringSize + ringSize) % ringSize;
        outRing[(size_t)idx] += timeBuf[(size_t)i].real() * window[(size_t)i];
    }
}

//==============================================================================
// Pitch detection: normalised autocorrelation on a mono window.
// Returns the detected frequency in Hz, or 0 when nothing is confident
// (silence, noise, or polyphonic mush). confidenceOut is 0..1.
float FamousBeatsVocalPresetsProcessor::detectPitch(const float* windowIn, int numSamples,
                                                   float& confidenceOut) const
{
    confidenceOut = 0.0f;

    // Copy, remove DC, Hann-window, and measure energy.
    std::vector<float> x((size_t)numSamples);
    double mean = 0.0;
    for (int i = 0; i < numSamples; ++i)
        mean += windowIn[i];
    mean /= numSamples;

    double energy = 0.0;
    for (int i = 0; i < numSamples; ++i)
    {
        const float w = 0.5f - 0.5f * std::cos(2.0f * juce::MathConstants<float>::pi * i / (numSamples - 1));
        x[(size_t)i] = (float)((windowIn[i] - mean) * w);
        energy += (double)x[(size_t)i] * x[(size_t)i];
    }
    if (energy < 1e-8) // silence
        return 0.0f;

    const int minLag = (int)std::floor(hostRate / 1050.0);
    const int maxLag = (int)std::ceil(hostRate / 65.0);
    const int maxUsableLag = juce::jmin(maxLag, numSamples / 2);

    // Normalised autocorrelation; track the strongest peak.
    float bestCorr = 0.0f;
    int bestLag = 0;
    for (int lag = minLag; lag <= maxUsableLag; ++lag)
    {
        double sum = 0.0;
        for (int i = 0; i < numSamples - lag; ++i)
            sum += (double)x[(size_t)i] * x[(size_t)(i + lag)];
        const float corr = (float)(sum / energy);
        if (corr > bestCorr)
        {
            bestCorr = corr;
            bestLag = lag;
        }
    }

    if (bestLag == 0 || bestCorr < 0.5f)
        return 0.0f;

    // Parabolic interpolation around the peak for sub-sample accuracy.
    auto corrAt = [&](int lag) -> double
    {
        double sum = 0.0;
        for (int i = 0; i < numSamples - lag; ++i)
            sum += (double)x[(size_t)i] * x[(size_t)(i + lag)];
        return sum / energy;
    };
    double offset = 0.0;
    if (bestLag > minLag && bestLag < maxUsableLag)
    {
        const double a = corrAt(bestLag - 1);
        const double b = corrAt(bestLag);
        const double c = corrAt(bestLag + 1);
        const double denom = a - 2.0 * b + c;
        if (std::abs(denom) > 1e-9)
            offset = juce::jlimit(-0.5, 0.5, 0.5 * (a - c) / denom);
    }

    confidenceOut = juce::jlimit(0.0f, 1.0f, bestCorr);
    return (float)(hostRate / ((double)bestLag + offset));
}

float FamousBeatsVocalPresetsProcessor::quantiseToScale(float freqHz, int key, int scale) const
{
    if (scale >= 4) // Chromatic: every note is in the scale.
        return freqHz;

    const float midiFloat = 69.0f + 12.0f * std::log2(freqHz / 440.0f);

    auto inScale = [&](int midi) -> bool
    {
        const int pc = ((midi - key) % 12 + 12) % 12;
        return pitchClassInScale(pc, scale);
    };

    // Nearest in-scale note by distance in semitones from the detected
    // pitch (not from the rounded chromatic note — rounding first and
    // then searching outward picks the wrong side when the pitch sits
    // just below a non-scale note, e.g. 228 Hz is nearer A3 than B3
    // but rounds to A#3, whose upward neighbour is B3).
    const int centre = (int)std::lrint(midiFloat);
    int bestMidi = centre;
    float bestDist = 1e9f;
    for (int midi = centre - 12; midi <= centre + 12; ++midi)
    {
        if (!inScale(midi))
            continue;
        const float d = std::abs((float)midi - midiFloat);
        if (d < bestDist)
        {
            bestDist = d;
            bestMidi = midi;
        }
    }
    return midiToFreq(bestMidi);
}

//==============================================================================
void FamousBeatsVocalPresetsProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    hostRate = sampleRate;

    // Latency: RNNoise's 10 ms frame delay + the phase vocoder's one
    // FFT frame (2048 samples). The dry path is delayed by the same
    // 2048 samples, so the tune mix stays in time; the host compensates.
    cleanerLatency = (int)std::ceil(0.010 * sampleRate) + 2;
    setLatencySamples(cleanerLatency + PhaseVocoder::kFftSize);

    const int numCh = juce::jmax(1, getTotalNumInputChannels());

    cleanerChannels.assign((size_t)numCh, CleanerChannelState());
    resetCleanerState();
    initDenoisers();

    eqChannels.clear();
    for (int i = 0; i < numCh; ++i)
        eqChannels.push_back(std::make_unique<EqChannel>());
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32)juce::jmax(1, samplesPerBlock),
                                  (juce::uint32)numCh };
    for (auto& eq : eqChannels)
    {
        eq->hp.prepare(spec);
        eq->mud.prepare(spec);
        eq->presence.prepare(spec);
        eq->air.prepare(spec);
    }
    mudGainS.reset(sampleRate, 0.05);
    presGainS.reset(sampleRate, 0.05);
    airGainS.reset(sampleRate, 0.05);
    eqAmountS.reset(sampleRate, 0.05);
    hpFreqS.reset(sampleRate, 0.05);
    mudGainS.setCurrentAndTargetValue(mudGainParam->load());
    presGainS.setCurrentAndTargetValue(presGainParam->load());
    airGainS.setCurrentAndTargetValue(airGainParam->load());
    eqAmountS.setCurrentAndTargetValue(eqAmountParam->load() * 0.01f);
    hpFreqS.setCurrentAndTargetValue(hpFreqParam->load());

    compressor.prepare(spec);
    compressor.setThreshold(compThreshParam->load());
    compressor.setRatio(compRatioParam->load());
    compressor.setAttack(compAttackParam->load());
    compressor.setRelease(compReleaseParam->load());

    vocoders.clear();
    for (int i = 0; i < numCh; ++i)
    {
        vocoders.push_back(std::make_unique<PhaseVocoder>());
        vocoders.back()->prepare(sampleRate);
    }
    tuneDryDelay.assign((size_t)(numCh * PhaseVocoder::kFftSize), 0.0f);
    tuneDryPos.assign((size_t)numCh, 0);
    shiftRatio = 1.0f;
    tuneGate = 0.0f;
    detectedPitchHz.store(0.0f);
    shiftRatioSmoothed.store(1.0f);
    detectRing.assign(4096, 0.0f);
    detectRingPos = 0;
    detectCountdown = 0;
    detectWindow.assign(2048, 0.0f);

    reverb.prepare(spec);
    reverbParams.roomSize = 0.4f;
    reverbParams.damping = 0.5f;
    reverbParams.wetLevel = 0.15f;
    reverbParams.dryLevel = 1.0f;
    reverbParams.width = 1.0f;
    reverbParams.freezeMode = 0.0f;
    reverb.setParameters(reverbParams);

    delayChannels.assign((size_t)numCh, DelayChannel());
    const int delayLen = (int)std::ceil(sampleRate * 2.0); // 2 seconds max
    for (auto& d : delayChannels)
    {
        d.buffer.assign((size_t)delayLen, 0.0f);
        d.writePos = 0;
        d.feedbackLp = 0.0f;
    }
}

void FamousBeatsVocalPresetsProcessor::releaseResources()
{
    freeDenoisers();
    cleanerChannels.clear();
}

//==============================================================================
// Stage 1 — Vocal Cleaner (RNNoise), copied from the Vocal Cleaner plugin:
// resample to 48 kHz, denoise 480-sample frames (int16 scale!), resample
// back, and mix with the latency-matched dry signal.
void FamousBeatsVocalPresetsProcessor::processCleaner(juce::AudioBuffer<float>& buffer, float amount)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    std::vector<float> resampled48k;
    std::vector<float> resampledHost;
    std::vector<float> frame48k;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto& state = cleanerChannels[(size_t)ch];
        float* channelData = buffer.getWritePointer(ch);

        state.to48k.process(channelData, numSamples, resampled48k);
        for (float s : resampled48k)
            state.in48k.push_back(s);

        while (state.in48k.size() >= (size_t)kFrameSize)
        {
            for (int i = 0; i < kFrameSize; ++i)
            {
                // RNNoise wants int16-scale floats (+/-32767), not +/-1.0.
                state.frameIn[i] = state.in48k.front() * 32768.0f;
                state.in48k.pop_front();
            }
            if (state.denoiser != nullptr)
                rnnoise_process_frame(state.denoiser, state.frameOut, state.frameIn);
            else
                std::copy(std::begin(state.frameIn), std::end(state.frameIn), std::begin(state.frameOut));

            for (int i = 0; i < kFrameSize; ++i)
                state.out48k.push_back(state.frameOut[i] * (1.0f / 32768.0f));
        }

        frame48k.assign(state.out48k.begin(), state.out48k.end());
        state.out48k.clear();
        state.from48k.process(frame48k.data(), (int)frame48k.size(), resampledHost);
        for (float s : resampledHost)
            state.pendingWet.push_back(s);

        const int delayLen = (int)state.dryDelay.size();
        for (int n = 0; n < numSamples; ++n)
        {
            const float dryIn = channelData[n];

            float dryDelayed = 0.0f;
            if (delayLen > 0)
            {
                dryDelayed = state.dryDelay[(size_t)state.dryDelayPos];
                state.dryDelay[(size_t)state.dryDelayPos] = dryIn;
                state.dryDelayPos = (state.dryDelayPos + 1) % delayLen;
            }

            float wet = 0.0f;
            if (!state.pendingWet.empty())
            {
                wet = state.pendingWet.front();
                state.pendingWet.pop_front();
            }

            channelData[n] = wet * amount + dryDelayed * (1.0f - amount);
        }
    }
}

// Stage 2 — EQ: high-pass, mud cut, presence boost, air boost.
// EQ Amount scales the band gains (and opens the high-pass) toward flat.
void FamousBeatsVocalPresetsProcessor::processEQ(juce::AudioBuffer<float>& buffer)
{
    const float amount = eqAmountS.getNextValue();
    const float hpFreq = hpFreqS.getNextValue();
    const float mudGain = mudGainS.getNextValue() * amount;
    const float presGain = presGainS.getNextValue() * amount;
    const float airGain = airGainS.getNextValue() * amount;

    mudGainS.setTargetValue(mudGainParam->load());
    presGainS.setTargetValue(presGainParam->load());
    airGainS.setTargetValue(airGainParam->load());
    eqAmountS.setTargetValue(eqAmountParam->load() * 0.01f);
    hpFreqS.setTargetValue(hpFreqParam->load());

    // At 0% the high-pass opens right down to 20 Hz (effectively off).
    const float hpFreqEff = 20.0f + (hpFreq - 20.0f) * amount;

    auto hpCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(hostRate, hpFreqEff);
    auto mudCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        hostRate, mudFreqParam->load(), 1.0f, juce::Decibels::decibelsToGain(mudGain));
    auto presCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        hostRate, presFreqParam->load(), 1.0f, juce::Decibels::decibelsToGain(presGain));
    auto airCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        hostRate, airFreqParam->load(), 0.7f, juce::Decibels::decibelsToGain(airGain));

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto& eq = eqChannels[(size_t)ch];
        *eq->hp.coefficients = *hpCoeffs;
        *eq->mud.coefficients = *mudCoeffs;
        *eq->presence.coefficients = *presCoeffs;
        *eq->air.coefficients = *airCoeffs;

        juce::dsp::AudioBlock<float> block(buffer);
        auto channelBlock = block.getSingleChannelBlock((size_t)ch);
        juce::dsp::ProcessContextReplacing<float> ctx(channelBlock);
        eq->hp.process(ctx);
        eq->mud.process(ctx);
        eq->presence.process(ctx);
        eq->air.process(ctx);
    }
}

// Stage 3 — Compressor (JUCE dsp) with parallel mix.
void FamousBeatsVocalPresetsProcessor::processCompressor(juce::AudioBuffer<float>& buffer)
{
    compressor.setThreshold(compThreshParam->load());
    compressor.setRatio(compRatioParam->load());
    compressor.setAttack(compAttackParam->load());
    compressor.setRelease(compReleaseParam->load());

    const float mix = juce::jlimit(0.0f, 1.0f, compMixParam->load() * 0.01f);
    if (mix <= 0.0f)
        return;

    dryBuffer.makeCopyOf(buffer, true);

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> ctx(block);
    compressor.process(ctx);

    if (mix < 1.0f)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* wet = buffer.getWritePointer(ch);
            const auto* dry = dryBuffer.getReadPointer(ch);
            for (int n = 0; n < buffer.getNumSamples(); ++n)
                wet[n] = dry[n] * (1.0f - mix) + wet[n] * mix;
        }
    }
}

// Stage 4 — Saturation: tanh waveshaper with drive + mix.
void FamousBeatsVocalPresetsProcessor::processSaturation(juce::AudioBuffer<float>& buffer)
{
    const float drive = juce::jlimit(0.0f, 1.0f, satDriveParam->load() * 0.01f);
    const float mix = juce::jlimit(0.0f, 1.0f, satMixParam->load() * 0.01f);
    if (mix <= 0.0f || drive <= 0.0f)
        return;

    const float g = 1.0f + drive * 5.0f;          // 1..6
    const float post = 1.0f / std::sqrt(g);        // keeps peaks under 1.0

    dryBuffer.makeCopyOf(buffer, true);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* wet = buffer.getWritePointer(ch);
        const auto* dry = dryBuffer.getReadPointer(ch);
        for (int n = 0; n < buffer.getNumSamples(); ++n)
        {
            const float shaped = std::tanh(g * wet[n]) * post;
            wet[n] = dry[n] * (1.0f - mix) + shaped * mix;
        }
    }
}

// Stage 5 — Tune: detect pitch on the mono average, snap to the scale,
// slew the shift ratio at the retune speed, and crossfade between the
// latency-matched dry signal and the phase-vocoder output.
void FamousBeatsVocalPresetsProcessor::processTuner(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    const float amount = juce::jlimit(0.0f, 1.0f, tuneAmountParam->load() * 0.01f);
    const float speed = juce::jlimit(0.0f, 1.0f, tuneSpeedParam->load() * 0.01f);
    const int key = (int)tuneKeyParam->load();
    const int scale = (int)tuneScaleParam->load();

    // Feed the mono detection ring with the tune-section input.
    for (int n = 0; n < numSamples; ++n)
    {
        float mono = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            mono += buffer.getReadPointer(ch)[n];
        mono /= (float)numChannels;
        detectRing[(size_t)detectRingPos] = mono;
        detectRingPos = (detectRingPos + 1) % (int)detectRing.size();
    }

    // Retune speed: time constant 400 ms (gentle) down to 2 ms (hard tune).
    const double tauSeconds = 0.400 * std::pow(0.005, (double)speed);

    detectCountdown -= numSamples;
    if (detectCountdown <= 0)
    {
        detectCountdown += 1024; // detection hop

        // Copy the latest 2048 mono samples into the detection window.
        const int ringSize = (int)detectRing.size();
        for (int i = 0; i < 2048; ++i)
        {
            const int idx = (detectRingPos - 2048 + i + ringSize * 2) % ringSize;
            detectWindow[(size_t)i] = detectRing[(size_t)idx];
        }

        float confidence = 0.0f;
        const float detected = detectPitch(detectWindow.data(), 2048, confidence);

        // Gate: only correct when the detector is confident and the
        // signal is loud enough to be a voice, not room noise.
        double rms = 0.0;
        for (float s : detectWindow)
            rms += (double)s * s;
        rms = std::sqrt(rms / detectWindow.size());
        const bool voiced = detected > 0.0f && confidence > 0.75f
                            && rms > 0.0032; // about -50 dBFS

        const double hopSeconds = 1024.0 / hostRate;
        const double alphaRatio = 1.0 - std::exp(-hopSeconds / tauSeconds);

        if (voiced)
        {
            const float target = quantiseToScale(detected, key, scale);
            const float targetRatio = juce::jlimit(0.5f, 1.9f, target / detected);
            shiftRatio += (targetRatio - shiftRatio) * (float)alphaRatio;
            detectedPitchHz.store(detected);
            tuneGate += (1.0f - tuneGate) * (float)(1.0 - std::exp(-hopSeconds / 0.005));
        }
        else
        {
            tuneGate += (0.0f - tuneGate) * (float)(1.0 - std::exp(-hopSeconds / 0.200));
        }
        shiftRatioSmoothed.store(shiftRatio);
    }

    const float wetMix = amount * tuneGate;
    const int delayLen = PhaseVocoder::kFftSize;
    std::vector<float> dryIn((size_t)numSamples);
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        // Save the dry input (the vocoder runs in place, below).
        for (int n = 0; n < numSamples; ++n)
            dryIn[(size_t)n] = data[n];
        // Wet: phase-vocoder pitch shift (already delayed by one frame).
        vocoders[(size_t)ch]->process(data, numSamples, shiftRatio);
        // Dry: the same one-frame delay, then crossfade wet/dry.
        auto& dryPos = tuneDryPos[(size_t)ch];
        float* dryDelay = &tuneDryDelay[(size_t)(ch * delayLen)];
        for (int n = 0; n < numSamples; ++n)
        {
            const float wet = data[n];
            const float dry = dryDelay[dryPos];
            dryDelay[dryPos] = dryIn[(size_t)n];
            data[n] = dry * (1.0f - wetMix) + wet * wetMix;
            dryPos = (dryPos + 1) % delayLen;
        }
    }
}

// Stage 6 — Reverb (JUCE dsp). Dry stays at unity; mix adds the wet tail.
void FamousBeatsVocalPresetsProcessor::processReverb(juce::AudioBuffer<float>& buffer)
{
    const float mix = juce::jlimit(0.0f, 1.0f, reverbMixParam->load() * 0.01f);
    const float size = juce::jlimit(0.0f, 1.0f, reverbSizeParam->load() * 0.01f);

    reverbParams.roomSize = 0.15f + 0.75f * size;
    reverbParams.damping = 0.5f;
    reverbParams.wetLevel = mix;
    reverbParams.dryLevel = 1.0f;
    reverbParams.width = 1.0f;
    reverbParams.freezeMode = 0.0f;
    reverb.setParameters(reverbParams);

    if (mix <= 0.0f)
        return;

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> ctx(block);
    reverb.process(ctx);
}

// Stage 7 — Stereo delay with darkened feedback ("throw" feel).
void FamousBeatsVocalPresetsProcessor::processDelay(juce::AudioBuffer<float>& buffer)
{
    const float mix = juce::jlimit(0.0f, 1.0f, delayMixParam->load() * 0.01f);
    if (mix <= 0.0f)
        return;

    const float feedback = juce::jlimit(0.0f, 0.85f, delayFeedbackParam->load() * 0.01f);
    const int delaySamples = (int)juce::jlimit(1.0, (double)delayChannels[0].buffer.size() - 2,
                                               delayTimeParam->load() * 0.001 * hostRate);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto& d = delayChannels[(size_t)ch];
        auto* data = buffer.getWritePointer(ch);
        const int size = (int)d.buffer.size();
        int readPos = (d.writePos - delaySamples + size) % size;

        for (int n = 0; n < buffer.getNumSamples(); ++n)
        {
            const float in = data[n];
            const float delayed = d.buffer[(size_t)readPos];
            // One-pole low-pass in the feedback path: repeats get darker.
            d.feedbackLp += 0.25f * (delayed - d.feedbackLp);
            d.buffer[(size_t)d.writePos] = in + d.feedbackLp * feedback;
            data[n] = in + delayed * mix;

            d.writePos = (d.writePos + 1) % size;
            readPos = (readPos + 1) % size;
        }
    }
}

//==============================================================================
void FamousBeatsVocalPresetsProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                                    juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    if (numSamples <= 0 || numChannels <= 0)
        return;

    // True bypass: nothing processed, host delay compensation keeps time.
    if (bypassParam->load() > 0.5f)
        return;

    // Re-prepare if the host changed the channel count.
    if ((int)cleanerChannels.size() != numChannels)
        prepareToPlay(hostRate, juce::jmax(512, numSamples));

    // Input gain.
    const float inGain = juce::Decibels::decibelsToGain(inputGainParam->load());
    if (inGain != 1.0f)
        buffer.applyGain(inGain);

    // 1. Vocal Cleaner (RNNoise)
    const float cleanAmount = juce::jlimit(0.0f, 1.0f, cleanAmountParam->load() * 0.01f);
    if (cleanAmount > 0.0f)
        processCleaner(buffer, cleanAmount);

    // 2. EQ — 3. Compressor — 4. Saturation — 5. Tune
    processEQ(buffer);
    processCompressor(buffer);
    processSaturation(buffer);
    processTuner(buffer);

    // 6. Reverb — 7. Delay
    processReverb(buffer);
    processDelay(buffer);

    // Output gain + safety: never let a runaway tail clip the bus.
    const float outGain = juce::Decibels::decibelsToGain(outputGainParam->load());
    buffer.applyGain(outGain);
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        for (int n = 0; n < numSamples; ++n)
        {
            float s = data[n];
            if (!std::isfinite(s))
                s = 0.0f;
            data[n] = juce::jlimit(-1.0f, 1.0f, s);
        }
    }
}

//==============================================================================
void FamousBeatsVocalPresetsProcessor::setParamValue(const char* paramId, float value)
{
    auto* param = parameters.getParameter(paramId);
    if (param != nullptr)
        param->setValueNotifyingHost(param->convertTo0to1(value));
}

const char* FamousBeatsVocalPresetsProcessor::getPresetName(int presetIndex)
{
    switch (presetIndex)
    {
        case 0: return "Drake";
        case 1: return "Travis Scott";
        case 2: return "Don Toliver";
        default: return "Clean / Radio";
    }
}

// Presets set every section. Values are the full parameter set, so picking
// a preset and then turning one knob keeps the rest of the chain.
void FamousBeatsVocalPresetsProcessor::applyPreset(int presetIndex)
{
    currentPreset = juce::jlimit(0, kNumPresets - 1, presetIndex);

    // Clean / Radio is the reference; the others adjust from it.
    float clean = 80, eq = 100, hp = 90, mud = -3.5f, mudF = 320, pres = 3.0f, presF = 4500,
          air = 3.0f, airF = 10000;
    float cThresh = -18, cRatio = 3.5f, cAttack = 8, cRelease = 120, cMix = 100;
    float sat = 10, satMix = 60;
    float tune = 40, tuneSpeed = 60;
    float verb = 18, verbSize = 40;
    float dly = 220, dlyFb = 30, dlyMix = 12;
    float in = 0, out = 0;

    switch (currentPreset)
    {
        case 0: // Drake — clean, warm, upfront.
            clean = 60; hp = 80; mud = -2.5f; mudF = 280; pres = 2.0f; presF = 3500;
            air = 2.0f; airF = 11000;
            cThresh = -16; cRatio = 2.5f; cAttack = 12; cRelease = 140;
            sat = 15; satMix = 80;
            tune = 25; tuneSpeed = 70; // gentle, transparent correction
            verb = 12; verbSize = 30;  // small room
            dly = 180; dlyFb = 25; dlyMix = 10; // short slap
            break;

        case 1: // Travis Scott — hard tune, big space, dotted throws.
            clean = 70; hp = 90; mud = -3.0f; mudF = 300; pres = 3.0f; presF = 4000;
            air = 4.0f; airF = 12000;
            cThresh = -14; cRatio = 4.0f; cAttack = 5; cRelease = 100;
            sat = 35; satMix = 100;
            tune = 100; tuneSpeed = 5; // fast retune, full amount = hard tune
            verb = 35; verbSize = 70;  // big spacey room
            dly = 420; dlyFb = 45; dlyMix = 30; // long dotted-style throws
            out = 1;
            break;

        case 2: // Don Toliver — melodic, airy, warm.
            clean = 65; hp = 80; mud = -2.0f; mudF = 250; pres = 2.0f; presF = 3000;
            air = 4.5f; airF = 12000;
            cThresh = -16; cRatio = 3.0f; cAttack = 15; cRelease = 160;
            sat = 25; satMix = 90;
            tune = 55; tuneSpeed = 45;
            verb = 25; verbSize = 55;
            dly = 300; dlyFb = 40; dlyMix = 20;
            break;

        default: // Clean / Radio — balanced full chain.
            break;
    }

    setParamValue(kInputGainParam, in);
    setParamValue(kOutputGainParam, out);
    setParamValue(kCleanAmountParam, clean);
    setParamValue(kEqAmountParam, eq);
    setParamValue(kHpFreqParam, hp);
    setParamValue(kMudGainParam, mud);
    setParamValue(kMudFreqParam, mudF);
    setParamValue(kPresGainParam, pres);
    setParamValue(kPresFreqParam, presF);
    setParamValue(kAirGainParam, air);
    setParamValue(kAirFreqParam, airF);
    setParamValue(kCompThreshParam, cThresh);
    setParamValue(kCompRatioParam, cRatio);
    setParamValue(kCompAttackParam, cAttack);
    setParamValue(kCompReleaseParam, cRelease);
    setParamValue(kCompMixParam, cMix);
    setParamValue(kSatDriveParam, sat);
    setParamValue(kSatMixParam, satMix);
    setParamValue(kTuneAmountParam, tune);
    setParamValue(kTuneSpeedParam, tuneSpeed);
    setParamValue(kReverbMixParam, verb);
    setParamValue(kReverbSizeParam, verbSize);
    setParamValue(kDelayTimeParam, dly);
    setParamValue(kDelayFeedbackParam, dlyFb);
    setParamValue(kDelayMixParam, dlyMix);
}

//==============================================================================
juce::AudioProcessorEditor* FamousBeatsVocalPresetsProcessor::createEditor()
{
    return new PluginEditor(*this);
}

void FamousBeatsVocalPresetsProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void FamousBeatsVocalPresetsProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

//==============================================================================
// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FamousBeatsVocalPresetsProcessor();
}
