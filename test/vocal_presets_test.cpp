// Test harness: feed generated audio through the Famous Beats Vocal
// Presets chain and report measured results.
//
//   1. Noise reduction (cleaner section, noise-only input)
//   2. Tuner pitch snap (off-pitch sine -> nearest scale note)
//   3. Vocal-like signal through the Clean/Radio preset (level/NaN)
//   4. Silence (no output, no crash, no NaN)
//   5. Polyphonic input with the tuner on (no crash, no NaN)
//   6. All five presets on a vocal-like signal (no NaN, no clipping)
//   7. Preamp: gain raises level without clipping, AIR lifts the top
//      end, full drive does not clip
//   8. Reverb types: each type produces a tail, tails differ
//   9. Delay divisions: computed ms at 120 BPM, plus an impulse echo
//  10. EQ curve: response follows the parameters
//  11. CPU measurement (informational, never fails) + per-section profile
//
// Usage: vocal_presets_test
// Exit code 0 = all tests passed, 1 = any test failed.

#include "../Source/PluginProcessor.h"
#include <cstdio>
#include <cmath>
#include <chrono>
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

    // ---- Test 6: all five presets ------------------------------------
    {
        printf("Test 6: all five presets on a vocal-like signal\n");
        double phase = 0.0;
        for (int p = 0; p < FamousBeatsVocalPresetsProcessor::kNumPresets; ++p)
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

    // ---- CPU benchmark ------------------------------------------------
    // Informational only (never fails): render 10 s of vocal-like audio
    // through the heaviest preset (Travis Scott: tune 100, sat 100,
    // reverb 35, delay 30) in 512-sample blocks and report wall time
    // as a percentage of real-time on one core. Three passes, median.
    // The signal generator itself is included in the timing in both the
    // before and after measurements, so the delta is what matters.
    {
        printf("\nCPU benchmark: 10 s vocal-like audio, Travis Scott preset\n");
        double pcts[3] = { 0.0, 0.0, 0.0 };
        for (int pass = 0; pass < 3; ++pass)
        {
            FamousBeatsVocalPresetsProcessor proc;
            proc.prepareToPlay(kSr, kBlock);
            proc.applyPreset(1); // Travis Scott — heaviest
            double phase = 0.0;
            const auto t0 = std::chrono::steady_clock::now();
            auto out = render(proc, 10.0, [&](double t, int)
            {
                const double f0 = 220.0 * std::pow(2.0, (0.5 / 12.0) * std::sin(2.0 * juce::MathConstants<double>::pi * 5.0 * t));
                phase += 2.0 * juce::MathConstants<double>::pi * f0 / kSr;
                double s = 0.0;
                for (int k = 1; k <= 6; ++k)
                    s += (1.0 / k) * std::sin(k * phase);
                return (float)(0.25 * s);
            });
            const auto t1 = std::chrono::steady_clock::now();
            const double secs = std::chrono::duration<double>(t1 - t0).count();
            pcts[pass] = secs / 10.0 * 100.0;
            printf("  pass %d: %.2f s wall for 10 s audio = %.1f%% of one core\n",
                   pass + 1, secs, pcts[pass]);
            (void)out;
        }
        double median = pcts[1];
        if ((pcts[0] <= pcts[1] && pcts[1] <= pcts[2]) || (pcts[2] <= pcts[1] && pcts[1] <= pcts[0]))
            median = pcts[1];
        else if ((pcts[1] <= pcts[0] && pcts[0] <= pcts[2]) || (pcts[2] <= pcts[0] && pcts[0] <= pcts[1]))
            median = pcts[0];
        else
            median = pcts[2];
        printf("  median: %.1f%% of one core (target < 25%%)\n", median);
    }

    // ---- Test 7: preamp gain, drive, and AIR -------------------------
    {
        printf("\nTest 7: preamp gain / drive / AIR button\n");
        // 7a: +12 dB gain should raise the level ~4x without clipping.
        {
            FamousBeatsVocalPresetsProcessor proc;
            proc.prepareToPlay(kSr, kBlock);
            setChainFlat(proc);
            setParam(proc, FamousBeatsVocalPresetsProcessor::kPreampGainParam, 12.0f);
            double phase = 0.0;
            auto out = render(proc, 1.0, [&](double t, int)
            {
                const double f0 = 220.0;
                phase += 2.0 * juce::MathConstants<double>::pi * f0 / kSr;
                return (float)(0.2 * std::sin(phase));
            });
            const double rms = regionRms(out, 0.2, 1.0);
            const float peak = regionPeak(out, 0.0, 1.0);
            printf("  +12 dB gain: output RMS %.3f (expect ~0.56), peak %.2f\n", rms, peak);
            check(rms > 0.45 && rms < 0.65, "preamp +12 dB raises level ~4x");
            check(peak <= 1.0f, "preamp gain does not clip");
            check(countNan(out) == 0, "no NaN in output");
        }
        // 7b: AIR on should lift the top end measurably. A +4 dB high
        // shelf only reaches about half its gain AT its corner, so the
        // probe is at 18 kHz, well above the 14 kHz corner, where the
        // shelf is at (nearly) full gain.
        {
            auto airRms = [&](bool airOn)
            {
                FamousBeatsVocalPresetsProcessor proc;
                proc.prepareToPlay(kSr, kBlock);
                setChainFlat(proc);
                setParam(proc, FamousBeatsVocalPresetsProcessor::kPreampAirParam, airOn ? 1.0f : 0.0f);
                auto out = render(proc, 1.0, [&](double t, int)
                {
                    return (float)(0.2 * std::sin(2.0 * juce::MathConstants<double>::pi * 18000.0 * t));
                });
                return regionRms(out, 0.2, 1.0);
            };
            const double off = airRms(false);
            const double on = airRms(true);
            printf("  18 kHz sine RMS: AIR off %.4f, AIR on %.4f (ratio %.2f, expect ~1.58)\n",
                   off, on, on / off);
            check(on / off > 1.35 && on / off < 1.75, "AIR button lifts the top end ~+4 dB");
        }
        // 7c: full drive on a loud sine must not clip.
        {
            FamousBeatsVocalPresetsProcessor proc;
            proc.prepareToPlay(kSr, kBlock);
            setChainFlat(proc);
            setParam(proc, FamousBeatsVocalPresetsProcessor::kPreampDriveParam, 100.0f);
            auto out = render(proc, 1.0, [&](double t, int)
            {
                return (float)(0.5 * std::sin(2.0 * juce::MathConstants<double>::pi * 220.0 * t));
            });
            const float peak = regionPeak(out, 0.0, 1.0);
            printf("  drive 100%% on 0.5 sine: peak %.2f\n", peak);
            check(peak <= 1.0f, "preamp full drive does not clip");
            check(countNan(out) == 0, "no NaN in output");
        }
    }

    // ---- Test 8: reverb types -----------------------------------------
    // 1 s of sine, then silence: measure the tail RMS after the input
    // stops, where only the reverb is audible. Types must differ.
    {
        printf("\nTest 8: reverb types (Hall / Plate / Room / Chamber)\n");
        double rmsByType[4] = { 0, 0, 0, 0 };
        for (int t = 0; t < 4; ++t)
        {
            FamousBeatsVocalPresetsProcessor proc;
            proc.prepareToPlay(kSr, kBlock);
            setChainFlat(proc);
            setParam(proc, FamousBeatsVocalPresetsProcessor::kReverbTypeParam, (float)t);
            setParam(proc, FamousBeatsVocalPresetsProcessor::kReverbMixParam, 50.0f);
            setParam(proc, FamousBeatsVocalPresetsProcessor::kReverbSizeParam, 50.0f);
            double phase = 0.0;
            auto out = render(proc, 2.0, [&](double t2, int)
            {
                if (t2 >= 1.0)
                    return 0.0f;
                phase += 2.0 * juce::MathConstants<double>::pi * 220.0 / kSr;
                return (float)(0.25 * std::sin(phase));
            });
            const double tailRms = regionRms(out, 1.2, 2.0);
            rmsByType[t] = tailRms;
            printf("  %-8s tail RMS %.4f, NaN %d\n",
                   FamousBeatsVocalPresetsProcessor::kReverbTypeNames[t], tailRms, countNan(out));
            char name[64];
            std::snprintf(name, sizeof(name), "reverb %s produces a tail, no NaN",
                          FamousBeatsVocalPresetsProcessor::kReverbTypeNames[t]);
            check(tailRms > 0.001 && countNan(out) == 0, name);
        }
        double lo = rmsByType[0], hi = rmsByType[0];
        for (int t = 1; t < 4; ++t)
        {
            lo = juce::jmin(lo, rmsByType[t]);
            hi = juce::jmax(hi, rmsByType[t]);
        }
        printf("  tail RMS spread: %.4f .. %.4f\n", lo, hi);
        check((hi - lo) / ((hi + lo) * 0.5) > 0.2, "reverb types decay differently");
    }

    // ---- Test 9: delay divisions --------------------------------------
    {
        printf("\nTest 9: tempo-synced delay divisions at 120 BPM\n");
        const double expectedSecs[7] = { 1.0, 0.5, 0.25, 1.0 / 6.0, 0.125, 0.0625, 0.03125 };
        FamousBeatsVocalPresetsProcessor proc;
        proc.prepareToPlay(kSr, kBlock);
        setParam(proc, FamousBeatsVocalPresetsProcessor::kDelaySyncParam, 1.0f);
        setParam(proc, FamousBeatsVocalPresetsProcessor::kDelayBpmParam, 120.0f);
        for (int d = 0; d < 7; ++d)
        {
            setParam(proc, FamousBeatsVocalPresetsProcessor::kDelayDivisionParam, (float)d);
            const double secs = proc.getDelayTimeSeconds();
            printf("  %-5s -> %.4f s (expect %.4f s)\n",
                   FamousBeatsVocalPresetsProcessor::kDelayDivisionNames[d], secs, expectedSecs[d]);
            char name[64];
            std::snprintf(name, sizeof(name), "division %s = %.0f ms at 120 BPM",
                          FamousBeatsVocalPresetsProcessor::kDelayDivisionNames[d],
                          expectedSecs[d] * 1000.0);
            check(std::abs(secs - expectedSecs[d]) < 0.001, name);
        }
        // Sync off falls back to the ms control.
        setParam(proc, FamousBeatsVocalPresetsProcessor::kDelaySyncParam, 0.0f);
        setParam(proc, FamousBeatsVocalPresetsProcessor::kDelayTimeParam, 220.0f);
        check(std::abs(proc.getDelayTimeSeconds() - 0.22) < 0.001,
              "sync off uses the ms control (220 ms)");

        // Functional check: an impulse echoes one division later.
        {
            FamousBeatsVocalPresetsProcessor proc2;
            proc2.prepareToPlay(kSr, kBlock);
            setChainFlat(proc2);
            setParam(proc2, FamousBeatsVocalPresetsProcessor::kDelaySyncParam, 1.0f);
            setParam(proc2, FamousBeatsVocalPresetsProcessor::kDelayDivisionParam, 1.0f); // 1/4 = 500 ms
            setParam(proc2, FamousBeatsVocalPresetsProcessor::kDelayBpmParam, 120.0f);
            setParam(proc2, FamousBeatsVocalPresetsProcessor::kDelayMixParam, 100.0f);
            setParam(proc2, FamousBeatsVocalPresetsProcessor::kDelayFeedbackParam, 0.0f);
            auto out = render(proc2, 1.5, [&](double t, int)
            {
                return (std::abs(t - 0.1) < 0.5 / kSr) ? 0.8f : 0.0f;
            });
            // The tuner dry-delay adds 2048 samples (~46 ms) even when
            // the tune amount is 0, so the impulse lands at ~0.146 s
            // and its echo at ~0.646 s.
            const float echoPeak = regionPeak(out, 0.60, 0.70);
            const float quietPeak = regionPeak(out, 0.25, 0.45);
            printf("  impulse echo: peak %.2f in [0.60, 0.70] s, %.3f in [0.25, 0.45] s\n",
                   echoPeak, quietPeak);
            check(echoPeak > 0.3f, "echo arrives one division later");
            check(quietPeak < 0.05f, "no echo where none is due");
        }
    }

    // ---- Test 10: EQ curve math ---------------------------------------
    {
        printf("\nTest 10: EQ curve responds to parameters\n");
        FamousBeatsVocalPresetsProcessor proc;
        proc.prepareToPlay(kSr, kBlock);
        setChainFlat(proc);
        setParam(proc, FamousBeatsVocalPresetsProcessor::kEqAmountParam, 100.0f);
        setParam(proc, FamousBeatsVocalPresetsProcessor::kHpFreqParam, 90.0f);
        setParam(proc, FamousBeatsVocalPresetsProcessor::kMudGainParam, -6.0f);
        setParam(proc, FamousBeatsVocalPresetsProcessor::kMudFreqParam, 320.0f);
        setParam(proc, FamousBeatsVocalPresetsProcessor::kPresGainParam, 6.0f);
        setParam(proc, FamousBeatsVocalPresetsProcessor::kPresFreqParam, 4500.0f);
        setParam(proc, FamousBeatsVocalPresetsProcessor::kAirGainParam, 6.0f);
        setParam(proc, FamousBeatsVocalPresetsProcessor::kAirFreqParam, 10000.0f);

        const float atPres = proc.getEqResponseDb(4500.0f);
        const float atMud = proc.getEqResponseDb(320.0f);
        const float atAir = proc.getEqResponseDb(10000.0f);
        const float atAirHigh = proc.getEqResponseDb(18000.0f);
        const float atLow = proc.getEqResponseDb(40.0f);
        printf("  curve: 40 Hz %.1f dB, 320 Hz %.1f dB, 4.5 kHz %.1f dB, 10 kHz %.1f dB, 18 kHz %.1f dB\n",
               atLow, atMud, atPres, atAir, atAirHigh);
        check(atPres > 4.5f && atPres < 8.0f, "presence boost shows ~+6 dB at 4.5 kHz");
        check(atMud > -7.5f && atMud < -4.5f, "mud cut shows ~-6 dB at 320 Hz");
        // A +6 dB shelf is only ~half gain at its 10 kHz corner (measured
        // 4.1 dB) and reaches full gain well above it (18 kHz).
        check(atAir > 3.0f && atAir < 5.5f, "air shelf shows ~half gain at its 10 kHz corner");
        check(atAirHigh > 5.0f && atAirHigh < 7.0f, "air shelf reaches ~+6 dB at 18 kHz");
        check(atLow < -8.0f, "high-pass cuts hard at 40 Hz");

        // Bulk curve matches the single-point values.
        const float freqs[5] = { 40.0f, 320.0f, 4500.0f, 10000.0f, 18000.0f };
        float dbs[5] = { 0, 0, 0, 0, 0 };
        proc.getEqResponseCurve(freqs, dbs, 5);
        check(std::abs(dbs[0] - atLow) < 0.01f && std::abs(dbs[1] - atMud) < 0.01f
              && std::abs(dbs[2] - atPres) < 0.01f && std::abs(dbs[3] - atAir) < 0.01f
              && std::abs(dbs[4] - atAirHigh) < 0.01f,
              "bulk curve matches single-point values");

        // The curve moves when a parameter moves.
        setParam(proc, FamousBeatsVocalPresetsProcessor::kPresGainParam, 0.0f);
        const float atPresFlat = proc.getEqResponseDb(4500.0f);
        printf("  presence to 0 dB: 4.5 kHz now %.1f dB\n", atPresFlat);
        check(std::abs(atPresFlat) < 1.5f, "curve follows the presence gain");
    }

    // ---- CPU profile: where the CPU goes -------------------------------
    // Single 10 s pass per configuration, Travis Scott preset. Shows
    // what the per-section bypass actually saves.
    {
        printf("\nCPU profile: 10 s vocal-like audio, single pass each\n");
        auto benchOne = [&](const char* label,
                            const std::function<void(FamousBeatsVocalPresetsProcessor&)>& setup)
        {
            FamousBeatsVocalPresetsProcessor proc;
            proc.prepareToPlay(kSr, kBlock);
            proc.applyPreset(1); // Travis Scott
            setup(proc);
            double phase = 0.0;
            const auto t0 = std::chrono::steady_clock::now();
            auto out = render(proc, 10.0, [&](double t, int)
            {
                const double f0 = 220.0 * std::pow(2.0, (0.5 / 12.0) * std::sin(2.0 * juce::MathConstants<double>::pi * 5.0 * t));
                phase += 2.0 * juce::MathConstants<double>::pi * f0 / kSr;
                double s = 0.0;
                for (int k = 1; k <= 6; ++k)
                    s += (1.0 / k) * std::sin(k * phase);
                return (float)(0.25 * s);
            });
            const auto t1 = std::chrono::steady_clock::now();
            const double pct = std::chrono::duration<double>(t1 - t0).count() / 10.0 * 100.0;
            printf("  %-32s %.1f%% of one core\n", label, pct);
            (void)out;
        };
        benchOne("Travis Scott (full)", [](FamousBeatsVocalPresetsProcessor&) {});
        benchOne("Travis Scott, Tune 0", [](FamousBeatsVocalPresetsProcessor& p)
        {
            setParam(p, FamousBeatsVocalPresetsProcessor::kTuneAmountParam, 0.0f);
        });
        benchOne("Travis Scott, Clean 0", [](FamousBeatsVocalPresetsProcessor& p)
        {
            setParam(p, FamousBeatsVocalPresetsProcessor::kCleanAmountParam, 0.0f);
        });
        benchOne("Travis Scott, Verb+Delay 0", [](FamousBeatsVocalPresetsProcessor& p)
        {
            setParam(p, FamousBeatsVocalPresetsProcessor::kReverbMixParam, 0.0f);
            setParam(p, FamousBeatsVocalPresetsProcessor::kDelayMixParam, 0.0f);
        });
        benchOne("Travis Scott, Preamp 0", [](FamousBeatsVocalPresetsProcessor& p)
        {
            setParam(p, FamousBeatsVocalPresetsProcessor::kPreampGainParam, 0.0f);
            setParam(p, FamousBeatsVocalPresetsProcessor::kPreampDriveParam, 0.0f);
            setParam(p, FamousBeatsVocalPresetsProcessor::kPreampAirParam, 0.0f);
        });
    }

    printf("\n%s: %d failure(s)\n", failures == 0 ? "ALL TESTS PASSED" : "FAILURES", failures);
    return failures == 0 ? 0 : 1;
}
