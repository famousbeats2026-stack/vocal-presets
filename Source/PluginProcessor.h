#pragma once

// Explicit module includes (rather than the generated JuceHeader.h) so
// the test harness can build the processor without the plugin client's
// generated headers.
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <deque>
#include <vector>
#include <atomic>

// RNNoise C API (sources in Source/RNNoise)
extern "C" {
#include "rnnoise.h"
}

//==============================================================================
// Famous Beats Vocal Presets — a one-click vocal chain in a single plugin:
//
//   Preamp (Avalon-style) -> Vocal Cleaner (RNNoise) -> EQ -> Compressor
//   -> Saturation -> Tune (pitch correction) -> Reverb -> Delay -> Output
//
// Four artist presets (Drake / Travis Scott / Don Toliver / Clean) set
// every section at once; each section still has its own knobs to tweak.
//
// The pitch corrector ("Tune") is a from-scratch design, NOT a clone of
// Antares Auto-Tune: autocorrelation pitch detection + a phase-vocoder
// pitch shifter. Fast retune + high amount gives the hard-tune
// Travis Scott sound; slow/gentle gives transparent correction.
class FamousBeatsVocalPresetsProcessor : public juce::AudioProcessor
{
public:
    FamousBeatsVocalPresetsProcessor();
    ~FamousBeatsVocalPresetsProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Famous Beats Vocal Presets"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.5; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getParametersState() { return parameters; }

    // One-click presets. Sets every section's parameters.
    // 0 = Drake, 1 = Travis Scott, 2 = Don Toliver, 3 = Clean / Radio,
    // 4 = Rap Singing
    void applyPreset(int presetIndex);
    static const char* getPresetName(int presetIndex);
    int getCurrentPreset() const { return currentPreset; }

    // Live tuner readouts (for the editor / tests).
    float getDetectedPitchHz() const { return detectedPitchHz.load(); }
    float getCurrentShiftRatio() const { return shiftRatioSmoothed.load(); }

    // Effective delay time in seconds (tempo-synced divisions when Sync
    // is on, otherwise the ms control). Uses the host BPM when the
    // playhead provides one, else the manual BPM parameter.
    double getDelayTimeSeconds() const;
    // Beats per delay division index (0 = 1/2 ... 6 = 1/64).
    static double divisionToBeats(int divisionIndex);
    // Combined EQ magnitude response in dB at a frequency, computed
    // from the current EQ parameters (drives the editor's EQ curve).
    float getEqResponseDb(float freqHz) const;
    // Same, for many frequencies at once (coefficients computed once).
    void getEqResponseCurve(const float* freqs, float* dbs, int numPoints) const;
    // Current host tempo in BPM, or 0 when the host provides none.
    double getHostBpm() const { return hostBpm.load(); }

    // ---- Parameter IDs ----
    static constexpr const char* kBypassParam    = "bypass";
    static constexpr const char* kInputGainParam = "inputGain";   // dB
    static constexpr const char* kOutputGainParam = "outputGain"; // dB

    // Preamp (Avalon-style): warm tube-style gain stage at the front.
    static constexpr const char* kPreampGainParam  = "preampGain";  // dB, -12..+24
    static constexpr const char* kPreampDriveParam = "preampDrive"; // 0..100 %
    static constexpr const char* kPreampAirParam   = "preampAir";   // bool, Air button

    static constexpr const char* kCleanAmountParam = "cleanAmount"; // 0..100 %

    static constexpr const char* kEqAmountParam = "eqAmount"; // 0..100 %
    static constexpr const char* kHpFreqParam   = "hpFreq";    // Hz
    static constexpr const char* kMudGainParam  = "mudGain";   // dB
    static constexpr const char* kMudFreqParam  = "mudFreq";   // Hz
    static constexpr const char* kPresGainParam = "presGain";  // dB
    static constexpr const char* kPresFreqParam = "presFreq";  // Hz
    static constexpr const char* kAirGainParam  = "airGain";   // dB
    static constexpr const char* kAirFreqParam  = "airFreq";   // Hz

    static constexpr const char* kCompThreshParam = "compThresh"; // dB
    static constexpr const char* kCompRatioParam  = "compRatio";  // :1
    static constexpr const char* kCompAttackParam = "compAttack"; // ms
    static constexpr const char* kCompReleaseParam = "compRelease"; // ms
    static constexpr const char* kCompMixParam    = "compMix";    // 0..100 %

    static constexpr const char* kSatDriveParam = "satDrive"; // 0..100 %
    static constexpr const char* kSatMixParam   = "satMix";   // 0..100 %

    static constexpr const char* kTuneAmountParam = "tuneAmount"; // 0..100 %
    static constexpr const char* kTuneSpeedParam  = "tuneSpeed";  // 0..100 %
    static constexpr const char* kTuneKeyParam    = "tuneKey";    // 0..11 (C..B)
    static constexpr const char* kTuneScaleParam  = "tuneScale";  // 0..4

    static constexpr const char* kReverbMixParam  = "reverbMix";  // 0..100 %
    static constexpr const char* kReverbSizeParam = "reverbSize"; // 0..100 %
    static constexpr const char* kReverbTypeParam = "reverbType"; // 0..3 (Hall/Plate/Room/Chamber)

    static constexpr const char* kDelayTimeParam     = "delayTime";     // ms
    static constexpr const char* kDelayFeedbackParam = "delayFeedback"; // 0..85 %
    static constexpr const char* kDelayMixParam      = "delayMix";      // 0..100 %
    static constexpr const char* kDelaySyncParam     = "delaySync";     // bool, tempo sync
    static constexpr const char* kDelayDivisionParam = "delayDivision"; // 0..6 (1/2..1/64)
    static constexpr const char* kDelayBpmParam      = "delayBpm";      // manual BPM, 40..240

    static constexpr int kNumPresets = 5;
    static constexpr const char* kReverbTypeNames[4] = {
        "Hall", "Plate", "Room", "Chamber"
    };
    static constexpr const char* kDelayDivisionNames[7] = {
        "1/2", "1/4", "1/8", "1/12", "1/16", "1/32", "1/64"
    };
    static constexpr const char* kScaleNames[5] = {
        "Major", "Minor", "Major Pent", "Minor Pent", "Chromatic"
    };
    static constexpr const char* kKeyNames[12] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };

private:
    // ---- Vocal Cleaner (RNNoise) ----
    // RNNoise runs on 480-sample frames at 48 kHz, and expects samples
    // at int16 scale (about +/-32767), NOT normalised +/-1.0 floats.
    // (The official rnnoise_demo.c passes raw short values as float.)
    static constexpr int kRnnoiseRate = 48000;
    static constexpr int kFrameSize = 480; // 10 ms at 48 kHz

    // Small streaming linear resampler (any rate <-> 48 kHz).
    struct LinearResampler
    {
        void reset(double ratio); // ratio = outputRate / inputRate
        void process(const float* input, int numSamples, std::vector<float>& out);

    private:
        double ratio = 1.0;
        double readPos = 0.0;
        std::deque<float> history;
    };

    struct CleanerChannelState
    {
        DenoiseState* denoiser = nullptr;
        LinearResampler to48k;
        LinearResampler from48k;
        std::deque<float> in48k;
        std::deque<float> out48k;
        std::deque<float> pendingWet;
        std::vector<float> dryDelay;
        int dryDelayPos = 0;
        float frameIn[kFrameSize] = {};
        float frameOut[kFrameSize] = {};
    };

    void initDenoisers();
    void freeDenoisers();
    void resetCleanerState();
    static RNNModel* loadEmbeddedModel();

    // ---- Pitch shifter: phase vocoder ----
    // A granular delay-line shifter was tried first and REJECTED: its
    // grain-boundary phase jumps cancel the shift on average, so the
    // output stayed at the input pitch. The phase vocoder is the proven
    // approach: STFT, shift the spectrum by the ratio, resynthesise
    // with phase propagation. It works on any input, and the ratio can
    // change smoothly. Latency is one FFT frame (2048 samples).
    struct PhaseVocoder
    {
        static constexpr int kFftSize = 2048;
        static constexpr int kHop = 1024; // 50% overlap: half the FFT
        // rate of the old 512 hop, same 2048-sample latency. Hann^2
        // overlap-add sums to 1.0 at 50% overlap (2.0 at 75%), so the
        // synthesis needs kSynthScale = 2.0 to stay unity gain.
        static constexpr int kNumBins = kFftSize / 2 + 1;

        void prepare(double sampleRate);
        void reset();
        // Process one channel of a block in place. ratio = shift factor
        // (1.0 = no shift). Output is delayed by kFftSize samples.
        void process(float* data, int numSamples, float ratio);

    private:
        void processFrame(float ratio);

        std::unique_ptr<juce::dsp::FFT> fft;
        std::vector<float> inRing;   // input ring buffer (8192)
        std::vector<float> outRing;  // overlap-add output ring (8192)
        std::vector<float> window;   // Hann window
        std::vector<float> prevPhase;
        std::vector<float> synthPhase;
        std::vector<float> mag;        // per-frame magnitude scratch
        std::vector<float> trueFreq;   // per-frame true-frequency scratch
        std::vector<juce::dsp::Complex<float>> fftBuf;  // time-domain frame
        std::vector<juce::dsp::Complex<float>> specBuf; // forward FFT output
        std::vector<juce::dsp::Complex<float>> outSpec; // shifted spectrum
        std::vector<juce::dsp::Complex<float>> timeBuf; // inverse FFT output
        int inCount = 0;             // total input samples ever written
        int samplesSinceFrame = 0;
    };

    // Pitch detection: normalised autocorrelation on a 2048-sample mono
    // window, 65..1050 Hz search range. Returns 0 when nothing confident.
    float detectPitch(const float* window, int numSamples, float& confidenceOut) const;
    // Snap a detected frequency to the nearest note of the selected
    // key/scale. Returns the target frequency (== input when chromatic).
    float quantiseToScale(float freqHz, int key, int scale) const;

    // ---- Chain stages ----
    void processPreamp(juce::AudioBuffer<float>& buffer);
    void processCleaner(juce::AudioBuffer<float>& buffer, float amount);
    void processEQ(juce::AudioBuffer<float>& buffer);
    void processCompressor(juce::AudioBuffer<float>& buffer);
    void processSaturation(juce::AudioBuffer<float>& buffer);
    void processTuner(juce::AudioBuffer<float>& buffer);
    void processReverb(juce::AudioBuffer<float>& buffer);
    void processDelay(juce::AudioBuffer<float>& buffer);

    // Tuner dry-delay only (no STFT): keeps the reported latency true
    // when the tuner is bypassed, at the cost of a ring-buffer copy.
    void processTunerDryDelayOnly(juce::AudioBuffer<float>& buffer);

    void setParamValue(const char* paramId, float value);

    juce::AudioProcessorValueTreeState parameters;
    int currentPreset = 3; // starts on Clean / Radio

    // Cached parameter pointers.
    std::atomic<float>* bypassParam = nullptr;
    std::atomic<float>* inputGainParam = nullptr;
    std::atomic<float>* outputGainParam = nullptr;
    std::atomic<float>* preampGainParam = nullptr;
    std::atomic<float>* preampDriveParam = nullptr;
    std::atomic<float>* preampAirParam = nullptr;
    std::atomic<float>* cleanAmountParam = nullptr;
    std::atomic<float>* eqAmountParam = nullptr;
    std::atomic<float>* hpFreqParam = nullptr;
    std::atomic<float>* mudGainParam = nullptr;
    std::atomic<float>* mudFreqParam = nullptr;
    std::atomic<float>* presGainParam = nullptr;
    std::atomic<float>* presFreqParam = nullptr;
    std::atomic<float>* airGainParam = nullptr;
    std::atomic<float>* airFreqParam = nullptr;
    std::atomic<float>* compThreshParam = nullptr;
    std::atomic<float>* compRatioParam = nullptr;
    std::atomic<float>* compAttackParam = nullptr;
    std::atomic<float>* compReleaseParam = nullptr;
    std::atomic<float>* compMixParam = nullptr;
    std::atomic<float>* satDriveParam = nullptr;
    std::atomic<float>* satMixParam = nullptr;
    std::atomic<float>* tuneAmountParam = nullptr;
    std::atomic<float>* tuneSpeedParam = nullptr;
    std::atomic<float>* tuneKeyParam = nullptr;
    std::atomic<float>* tuneScaleParam = nullptr;
    std::atomic<float>* reverbMixParam = nullptr;
    std::atomic<float>* reverbSizeParam = nullptr;
    std::atomic<float>* reverbTypeParam = nullptr;
    std::atomic<float>* delayTimeParam = nullptr;
    std::atomic<float>* delayFeedbackParam = nullptr;
    std::atomic<float>* delayMixParam = nullptr;
    std::atomic<float>* delaySyncParam = nullptr;
    std::atomic<float>* delayDivisionParam = nullptr;
    std::atomic<float>* delayBpmParam = nullptr;

    // Host tempo (BPM) from the playhead, 0 when the host gives none.
    std::atomic<double> hostBpm { 0.0 };

    // Preamp (Avalon-style): one Air high-shelf per channel.
    struct PreampChannel
    {
        juce::dsp::IIR::Filter<float> airShelf;
        bool airOn = false;
    };
    std::vector<std::unique_ptr<PreampChannel>> preampChannels;

    // Cleaner state.
    std::vector<CleanerChannelState> cleanerChannels;
    int cleanerLatency = 0;
    double hostRate = 48000.0;
    bool modelTried = false;
    RNNModel* sharedModel = nullptr;

    // EQ: one IIR filter per band, per channel. IIR::Filter is neither
    // copyable nor movable (it owns a HeapBlock), so channels are held
    // by unique_ptr.
    struct EqChannel
    {
        juce::dsp::IIR::Filter<float> hp, mud, presence, air;
    };
    std::vector<std::unique_ptr<EqChannel>> eqChannels;
    juce::SmoothedValue<float> mudGainS, presGainS, airGainS, eqAmountS, hpFreqS;

    // Compressor (JUCE dsp) + mix.
    juce::dsp::Compressor<float> compressor;
    juce::AudioBuffer<float> dryBuffer; // reused for comp/sat mixes

    // Tuner state.
    std::vector<std::unique_ptr<PhaseVocoder>> vocoders; // one per channel
    std::vector<float> tuneDryDelay;   // dry delay line, kFftSize, per channel
    std::vector<int> tuneDryPos;       // per-channel delay-line position
    std::vector<float> detectRing;     // mono ring buffer for pitch detection
    int detectRingPos = 0;
    int detectCountdown = 0;               // samples until next detection
    std::vector<float> detectWindow;
    float shiftRatio = 1.0f;               // smoothed, applied to wet shifter
    float tuneGate = 0.0f;                 // 0..1, fades wet in/out on confidence
    std::atomic<float> detectedPitchHz { 0.0f };
    std::atomic<float> shiftRatioSmoothed { 1.0f };

    // Reverb (JUCE dsp).
    juce::dsp::Reverb reverb;
    juce::dsp::Reverb::Parameters reverbParams;

    // Delay: per-channel circular buffer with darkened feedback.
    struct DelayChannel
    {
        std::vector<float> buffer;
        int writePos = 0;
        float feedbackLp = 0.0f;
    };
    std::vector<DelayChannel> delayChannels;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FamousBeatsVocalPresetsProcessor)
};
