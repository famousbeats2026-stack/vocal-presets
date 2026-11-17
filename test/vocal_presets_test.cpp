// Test harness: feed generated audio through the Famous Beats Vocal
// Presets chain and report measured results.
//
//   1. Noise reduction (cleaner section, noise-only input)
//   2. Tuner pitch snap (off-pitch sine -> nearest scale note)
//   3. Vocal-like signal through the Clean/Radio preset (level/NaN)
//   4. Silence (no output, no crash, no NaN)
//   5. Polyphonic input with the tuner on (no crash, no NaN)
//   6. All four presets on a vocal-like signal (no NaN, no clipping)
//
// Usage: vocal_presets_test
// Exit code 0 = all tests passed, 1 = any test failed.

#include "../Source/PluginProcessor.h"
#include <cstdio>
#include <cmath>
#include <vector>
#include <functional>
#include <random>

namespace
{
    constexpr double kSr = 44100.0;
    constexpr int kBlock = 512;

    double db(double rms) { return 20.0 * std::log10(rms + 1e-12); }

    void setParam(FamousBeatsVocalPresetsProcessor& p, const char* id, float value)
    {
        auto* param = p.getParametersState().getParameter(id);
        param->setValueNotifyingHost(param->convertTo0to1(value));
    }

    void setChainFlat(FamousBeatsVocalPresetsProcessor& p)
    {
        setParam(p, FamousBeatsVocalPresetsProcessor::kCleanAmountParam, 0.0f);
        setParam(p, FamousBeatsVocalPresetsProcessor::kEqAmountParam, 0.0f);
        setParam(p, FamousBeatsVocalPresetsProcessor::kCompMixParam, 0.0f);
        setParam(p, FamousBeatsVocalPresetsProcessor::kSatMixParam, 0.0f);
        setParam(p, FamousBeatsVocalPresetsProcessor::kTuneAmountParam, 0.0f);
        setParam(p, FamousBeatsVocalPresetsProcessor::kReverbMixParam, 0.0f);
        setParam(p, FamousBeatsVocalPresetsProcessor::kDelayMixParam, 0.0f);
        setParam(p, FamousBeatsVocalPresetsProcessor::kInputGainParam, 0.0f);
        setParam(p, FamousBeatsVocalPresetsProcessor::kOutputGainParam, 0.0f);
    }

    // Render `seconds` of audio through the processor in 512-sample
    // blocks. `gen(t, channel)` returns the input sample. Returns the
    // full stereo output.
    juce::AudioBuffer<float> render(FamousBeatsVocalPresetsProcessor& proc, double seconds,
                                    const std::function<float(double, int)>& gen)
    {
        const int total = (int)(seconds * kSr);
        juce::AudioBuffer<float> out(2, total);
        juce::MidiBuffer midi;
        juce::AudioBuffer<float> block(2, kBlock);
        int done = 0;
        while (done < total)
        {
            const int n = juce::jmin(kBlock, total - done);
            for (int ch = 0; ch < 2; ++ch)
            {
                auto* data = block.getWritePointer(ch);
                for (int i = 0; i < n; ++i)
                    data[i] = gen((done + i) / kSr, ch);
            }
            block.setSize(2, n, false, false, false);
            proc.processBlock(block, midi);
            for (int ch = 0; ch < 2; ++ch)
                out.copyFrom(ch, done, block, ch, 0, n);
            done += n;
            block.setSize(2, kBlock, false, false, false);
        }
        return out;
    }

    double regionRms(const juce::AudioBuffer<float>& buf, double t0, double t1)
    {
        const int s0 = (int)(t0 * kSr), s1 = (int)(t1 * kSr);
        double sum = 0.0;
        int n = 0;
        for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        {
            const auto* data = buf.getReadPointer(ch);
            for (int i = s0; i < s1 && i < buf.getNumSamples(); ++i)
            {
                sum += (double)data[i] * data[i];
                ++n;
            }
        }
        return n ? std::sqrt(sum / n) : 0.0;
    }

    float regionPeak(const juce::AudioBuffer<float>& buf, double t0, double t1)
    {
        const int s0 = (int)(t0 * kSr), s1 = (int)(t1 * kSr);
        float peak = 0.0f;
        for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        {
            const auto* data = buf.getReadPointer(ch);
            for (int i = s0; i < s1 && i < buf.getNumSamples(); ++i)
                peak = juce::jmax(peak, std::abs(data[i]));
        }
        return peak;
    }

    int countNan(const juce::AudioBuffer<float>& buf)
    {
        int n = 0;
        for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        {
            const auto* data = buf.getReadPointer(ch);
            for (int i = 0; i < buf.getNumSamples(); ++i)
                if (!std::isfinite(data[i]))
                    ++n;
        }
        return n;
    }

    // Dominant frequency of a mono window via normalised autocorrelation.
    double measureFreq(const float* x, int numSamples, double sr)
    {
        std::vector<float> w((size_t)numSamples);
        double mean = 0.0;
        for (int i = 0; i < numSamples; ++i)
            mean += x[i];
        mean /= numSamples;
        double energy = 0.0;
        for (int i = 0; i < numSamples; ++i)
        {
            const float hann = 0.5f - 0.5f * std::cos(2.0 * juce::MathConstants<double>::pi * i / (numSamples - 1));
            w[(size_t)i] = (float)((x[i] - mean) * hann);
            energy += (double)w[(size_t)i] * w[(size_t)i];
        }
        if (energy < 1e-10)
            return 0.0;

        const int minLag = (int)(sr / 1050.0);
        const int maxLag = (int)(sr / 65.0);
        double best = 0.0;
        int bestLag = 0;
        for (int lag = minLag; lag <= maxLag; ++lag)
        {
            double sum = 0.0;
            for (int i = 0; i < numSamples - lag; ++i)
                sum += (double)w[(size_t)i] * w[(size_t)(i + lag)];
            if (sum > best)
            {
                best = sum;
                bestLag = lag;
            }
        }
        if (bestLag == 0)
            return 0.0;
        return sr / bestLag;
    }

    double centsOff(double measured, double target)
    {
        return 1200.0 * std::log2(measured / target);
    }

    int failures = 0;
    void check(bool ok, const char* name)
    {
        printf("  [%s] %s\n", ok ? "PASS" : "FAIL", name);
        if (!ok)
            ++failures;
    }
}

int main()
{
    printf("Famous Beats Vocal Presets — chain tests (44.1 kHz, stereo)\n\n");

    // ---- Test 1: noise reduction -------------------------------------
    {
        printf("Test 1: cleaner noise reduction (white noise, -20 dBFS)\n");
        FamousBeatsVocalPresetsProcessor proc;
        proc.prepareToPlay(kSr, kBlock);
        setChainFlat(proc);
        setParam(proc, FamousBeatsVocalPresetsProcessor::kCleanAmountParam, 100.0f);

        std::mt19937 rng(12345);
        std::normal_distribution<float> noise(0.0f, 0.1f); // RMS 0.1
        auto out = render(proc, 3.0, [&](double, int) { return noise(rng); });
        // Input RMS measured from the generator alone would be -20 dB;
        // measure the output over the last second (pipeline primed).
        const double outRms = regionRms(out, 2.0, 3.0);
        printf("  input -20.0 dBFS, output %.1f dBFS (reduction %.1f dB)\n",
               db(outRms), -20.0 - db(outRms));
        check(outRms < 0.01, "noise reduced by >20 dB"); // output below -40 dBFS
        check(countNan(out) == 0, "no NaN in output");
    }

    // ---- Test 2: tuner pitch snap ------------------------------------
    // Input 228 Hz sits between A3 (220) and B3 (247) in C major, clearly
    // nearer A3, so the tuner must pull it down to 220 Hz. (An input
    // exactly on A#3 / 233.08 Hz is equidistant between A3 and B3, so it
    // is not a discriminating test.)
    {
        printf("Test 2: tuner snaps 228 Hz to A3 (220 Hz), C major\n");
        FamousBeatsVocalPresetsProcessor proc;
        proc.prepareToPlay(kSr, kBlock);
        setChainFlat(proc);
        setParam(proc, FamousBeatsVocalPresetsProcessor::kTuneAmountParam, 100.0f);
        setParam(proc, FamousBeatsVocalPresetsProcessor::kTuneSpeedParam, 100.0f);
        setParam(proc, FamousBeatsVocalPresetsProcessor::kTuneKeyParam, 0.0f);   // C
        setParam(proc, FamousBeatsVocalPresetsProcessor::kTuneScaleParam, 0.0f); // Major

        auto out = render(proc, 3.0, [&](double t, int)
        {
            return 0.5f * (float)std::sin(2.0 * juce::MathConstants<double>::pi * 228.0 * t);
        });
        const int s0 = (int)(2.0 * kSr), s1 = (int)(3.0 * kSr);
        const double measured = measureFreq(out.getReadPointer(0) + s0, s1 - s0, kSr);
        printf("  input 228.0 Hz, output %.1f Hz (%.0f cents off 220 Hz)\n",
               measured, centsOff(measured, 220.0));
        printf("  detected pitch (processor): %.1f Hz, ratio %.3f\n",
               proc.getDetectedPitchHz(), proc.getCurrentShiftRatio());
        check(measured > 0.0 && std::abs(centsOff(measured, 220.0)) < 30.0,
              "output within 30 cents of 220 Hz");
        check(countNan(out) == 0, "no NaN in output");
    }

    // ---- Test 3: vocal-like signal, Clean/Radio defaults -------------
    {
        printf("Test 3: vocal-like signal through default (Clean/Radio) chain\n");
        FamousBeatsVocalPresetsProcessor proc;
        proc.prepareToPlay(kSr, kBlock);
        // Defaults are the Clean/Radio preset; nothing to set.
        double phase = 0.0;
        auto out = render(proc, 3.0, [&](double t, int)
        {
            const double f0 = 220.0 * std::pow(2.0, (0.5 / 12.0) * std::sin(2.0 * juce::MathConstants<double>::pi * 5.0 * t));
            phase += 2.0 * juce::MathConstants<double>::pi * f0 / kSr;
            double s = 0.0;
            for (int k = 1; k <= 6; ++k)
                s += (1.0 / k) * std::sin(k * phase);
            return (float)(0.25 * s); // about -12 dBFS
        });
        const double rms = regionRms(out, 1.0, 3.0);
        const float peak = regionPeak(out, 0.0, 3.0);
        printf("  output RMS %.1f dBFS, peak %.2f, NaN count %d\n",
               db(rms), peak, countNan(out));
        check(rms > 0.005, "output present (RMS above -46 dBFS)");
        check(peak <= 1.0f, "no clipping (peak <= 1.0)");
        check(countNan(out) == 0, "no NaN in output");
    }

    // ---- Test 4: silence ---------------------------------------------
    {
        printf("Test 4: silence in -> silence out\n");
        FamousBeatsVocalPresetsProcessor proc;
        proc.prepareToPlay(kSr, kBlock);
        auto out = render(proc, 1.0, [](double, int) { return 0.0f; });
        const float peak = regionPeak(out, 0.0, 1.0);
        printf("  output peak %.2e, NaN count %d\n", peak, countNan(out));
        check(peak < 1e-6f, "output stays silent");
        check(countNan(out) == 0, "no NaN in output");
    }

    // ---- Test 5: polyphonic input with tuner on ----------------------
    {
        printf("Test 5: two-note chord (220 + 277 Hz) with tuner at 100%%\n");
        FamousBeatsVocalPresetsProcessor proc;
        proc.prepareToPlay(kSr, kBlock);
        setChainFlat(proc);
        setParam(proc, FamousBeatsVocalPresetsProcessor::kTuneAmountParam, 100.0f);
        setParam(proc, FamousBeatsVocalPresetsProcessor::kTuneSpeedParam, 100.0f);
        auto out = render(proc, 2.0, [&](double t, int)
        {
            return 0.3f * (float)(std::sin(2.0 * juce::MathConstants<double>::pi * 220.0 * t)
                                  + std::sin(2.0 * juce::MathConstants<double>::pi * 277.18 * t));
        });
        const float peak = regionPeak(out, 0.0, 2.0);
        printf("  output peak %.2f, NaN count %d (must not crash or blow up)\n",
               peak, countNan(out));
        check(peak <= 1.0f, "no clipping");
        check(countNan(out) == 0, "no NaN in output");
    }

    // ---- Test 6: all four presets ------------------------------------
    {
        printf("Test 6: all four presets on a vocal-like signal\n");
        double phase = 0.0;
        for (int p = 0; p < 4; ++p)
        {
            FamousBeatsVocalPresetsProcessor proc;
            proc.prepareToPlay(kSr, kBlock);
            proc.applyPreset(p);
            auto out = render(proc, 1.0, [&](double t, int)
            {
                const double f0 = 220.0 * std::pow(2.0, (0.3 / 12.0) * std::sin(2.0 * juce::MathConstants<double>::pi * 5.0 * t));
                phase += 2.0 * juce::MathConstants<double>::pi * f0 / kSr;
                double s = 0.0;
                for (int k = 1; k <= 6; ++k)
                    s += (1.0 / k) * std::sin(k * phase);
                return (float)(0.25 * s);
            });
            const float peak = regionPeak(out, 0.0, 1.0);
            printf("  %-14s peak %.2f, NaN %d\n",
                   FamousBeatsVocalPresetsProcessor::getPresetName(p), peak, countNan(out));
            char name[64];
            std::snprintf(name, sizeof(name), "preset %s: no clipping, no NaN",
                          FamousBeatsVocalPresetsProcessor::getPresetName(p));
            check(peak <= 1.0f && countNan(out) == 0, name);
        }
    }

    printf("\n%s: %d failure(s)\n", failures == 0 ? "ALL TESTS PASSED" : "FAILURES", failures);
    return failures == 0 ? 0 : 1;
}
