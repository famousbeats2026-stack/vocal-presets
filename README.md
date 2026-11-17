# Famous Beats Vocal Presets — one-click vocal chain for Mac

A full vocal chain in one plugin. Put it on a vocal track, pick a
preset, and the vocal is cleaned, EQ'd, compressed, saturated, tuned,
and given space — ready to sit in the mix.

**What you get here is the full plugin project — not the plugin file
itself.** A Mac plugin file (.vst3 / .component) can only be built on a
Mac (or on GitHub's Mac computers). Pick one of the two ways below to
make the installable file. The easy one is Option A — it needs no Xcode
on your Mac.

---

## Option A — Let GitHub build it for you (easiest, no Xcode needed)

You need a free GitHub account. If you don't have one yet, make one at
https://github.com/signup — it takes about 5 minutes, just an email
and a password.

1. On GitHub, press the **+** at the top right, then **New repository**.
   Name it `vocal-presets`, leave it Public, and press **Create repository**.
2. On the next page, press **"uploading an existing file"**.
3. Drag this whole folder (everything in it) into the upload area, then
   press **Commit changes**.
4. Click the **Actions** tab at the top. You'll see a run called
   "Build macOS plugin" — wait for the green checkmark (a few minutes).
   If it hasn't started, press **Run workflow** on the right.
5. When it's done, click the finished run, scroll to the bottom, and
   download **FamousBeatsVocalPresets-macOS**. Unzip it — inside are:
   - `Famous Beats Vocal Presets.vst3`
   - `Famous Beats Vocal Presets.component`

## Option B — Build it on your own Mac (needs Xcode)

1. Install Xcode free from the App Store (it's a big download, about
   12 GB — let it finish). Then open Terminal once and run:
   `xcode-select --install`
2. In Terminal, get this project (replace the link with your own if you
   put it on GitHub, or just copy this folder to your Mac):
   ```
   git clone https://github.com/YOURNAME/vocal-presets.git
   cd vocal-presets
   ```
3. Build it:
   ```
   cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15
   cmake --build build --config Release
   ```
4. The finished files are in:
   - `build/FamousBeatsVocalPresets_artefacts/Release/VST3/Famous Beats Vocal Presets.vst3`
   - `build/FamousBeatsVocalPresets_artefacts/Release/AU/Famous Beats Vocal Presets.component`

---

## Installing the plugin

1. Copy the files into your plugin folders:
   - `.vst3` → `~/Library/Audio/Plug-Ins/VST3/`
   - `.component` → `~/Library/Audio/Plug-Ins/Components/`

   (To get there in Finder: press Cmd+Shift+G and paste the path.)
2. This plugin isn't signed with an Apple developer certificate, so
   macOS may block it the first time. If your DAW says it can't load
   it, run this in Terminal (it just clears the block flag):
   ```
   xattr -cr ~/Library/Audio/Plug-Ins/VST3/"Famous Beats Vocal Presets.vst3"
   xattr -cr ~/Library/Audio/Plug-Ins/Components/"Famous Beats Vocal Presets.component"
   ```
   Then restart your DAW and rescan plugins.
3. Works on macOS 10.15 (Catalina) and newer, Intel and Apple Silicon
   (universal binary — both chips in one file).

## How to use it

- Put it on a **vocal track** — one instance per vocal. It is a vocal
  chain, not a mix-bus plugin: don't put it on a finished mix.
- **Pick a preset first** — Drake, Travis Scott, Don Toliver, Clean /
  Radio, or Rap Singing. That sets the whole chain at once. Then turn
  any knob to taste.
- **Key / Scale** (in the Tune section) tells the pitch corrector what
  key your song is in. Default is C major. Set it to your song's key or
  the tuner will pull notes to the wrong scale.
- **Tune Amount** — how hard the correction pulls. 100% with a fast
  Retune Speed is the hard-tune Travis Scott sound; lower amounts with
  a slow speed are transparent.
- **EQ curve** — the gold curve at the top shows the EQ as it really
  is. Drag a node left/right to move its frequency, up/down to change
  its gain. HP moves in frequency only.
- **Preamp** — the first thing your vocal hits. Gain and Drive warm it
  up like a tube channel strip; the **AIR** button opens the top end
  with a high air boost.
- **Delay Sync** — turn Sync on and pick a note division (1/2, 1/4,
  1/8, 1/12, 1/16, 1/32, 1/64) and the delay follows your song's tempo
  (from the DAW, or the BPM knob when the DAW gives no tempo). Turn it
  off to set the time in ms.
- **Reverb Type** — Hall, Plate, Room, or Chamber. Hall is big and
  lush, Plate is bright and dense, Room is small and tight, Chamber is
  warm and medium-large.
- Use it on **dry vocals**. The cleaner can't tell music from noise,
  so don't put it on a vocal with music behind it.
- It adds about 45 ms of latency (noise cleaner + pitch shifter), but
  your DAW compensates for that automatically, so the vocal stays in
  time. It is a mixing plugin — don't track live through it.
- **CPU** — sections you turn down to zero are skipped entirely, so
  turning Tune or the cleaner off frees real CPU. If it still feels
  heavy, turn the Tune Amount down first: the pitch shifter is the
  hungriest part of the chain.

## The chain (in order)

1. **Preamp (Avalon-style)** — warm tube-style gain stage with Gain
   and Drive, plus the **AIR** button: a high air boost that opens the
   top end. (Avalon-style character — it is not the actual Avalon
   VT-737SP hardware.)
2. **Vocal Cleaner** — RNNoise neural denoiser (the tech behind
   Discord/Zoom voice cleanup). Removes room tone, fan, hum and hiss.
3. **EQ** — industry-standard vocal curve: high-pass (~85 Hz), mud cut
   (~300 Hz), presence boost (~4 kHz), air boost (~11 kHz). The curve
   at the top of the window shows it and is draggable.
4. **Compressor** — smooths the dynamics so the vocal sits steady.
   Threshold, Ratio, Attack, Release and Mix are all on the panel.
5. **Saturation** — tanh warmth/grit. A little adds body; a lot adds
   edge.
6. **Tune** — pitch correction. Detects the sung pitch and snaps it to
   the nearest note of your key/scale. See the honesty note below.
7. **Reverb** — Hall, Plate, Room, or Chamber space.
8. **Delay** — stereo delay with darkened repeats, for slap and throws.
   Tempo-synced note divisions or plain ms.
9. **Output gain**.

## The presets

- **Drake** — clean, warm, upfront. Gentle cleaning, subtle tune,
  light compression, warm saturation, a small room, and a short slap
  delay.
- **Travis Scott** — hard tune (fast retune, full amount), big spacey
  reverb, long delay throws, more saturation, airy top end.
- **Don Toliver** — melodic and airy. Medium tune, bright airy EQ,
  medium reverb, warm delay.
- **Clean / Radio** — polished modern radio vocal. Balanced full chain.
- **Rap Singing** — punchy and upfront for melodic rap and sung rap.
  Tight fast compression, light natural tune, small tight room reverb,
  and a tempo-synced 1/8 delay, with the preamp and AIR on.

## About the tuner — please read

The built-in pitch corrector is written from scratch for this plugin.
It is **not Antares Auto-Tune**, and it does not sound identical to
it. It detects the sung pitch with autocorrelation and shifts it with
a phase vocoder. On a clean, monophonic vocal (one voice,
no music behind it) it snaps off-pitch notes onto the scale reliably,
and at full amount with fast retune it gives the hard-tune effect.
Its limits: it is monophonic, so it gets confused by harmonies,
doubles, or music bleeding into the mic; very fast runs and vibrato
can sound slightly phasey at extreme settings; and it cannot fix a
vocal that is wildly out of tune the way a manual editor can. If you
need that level of control, record the vocal through this chain and
fine-tune problem notes in your DAW's pitch editor afterwards.

## What it does, technically

- RNNoise neural denoiser (xiph/rnnoise, BSD licence), with the
  "sh" model from GregorR/rnnoise-models, embedded in the plugin.
  If that model ever fails to load, it falls back to RNNoise's
  built-in model.
- Runs the cleaner at 48 kHz internally and resamples to and from
  your session's sample rate, so it works at 44.1 kHz, 48 kHz,
  96 kHz, and anything else your DAW uses. Stereo: one denoiser
  per channel.
- Pitch detection by normalised autocorrelation (65–1050 Hz), now
  running every 2048 samples; pitch shifting by a phase vocoder
  (2048-point STFT, hop 1024, 50% overlap, with Laroche phase locking
  so the shift stays accurate at the wider hop); ratio slewed by the
  Retune Speed control; correction gated off on silence, noise, and
  low-confidence detection so it never chases room tone. The vocoder
  adds one FFT frame (2048 samples) of latency, which the host
  compensates. When Tune Amount is 0 the vocoder is skipped entirely
  (only a cheap delay line keeps the timing true), and every section
  whose amount is 0 is skipped the same way — that is most of the CPU
  saving. Full chain (Travis Scott preset, the heaviest) measured
  15.5% of one core on the build machine, down from 22.1% before
  these optimisations; with Tune off it drops to 8.4%.
- The EQ knobs are smoothed over 50 ms per block (a pre-existing bug
  had the smoothers advancing one sample-step per block, so the EQ
  took ~25 s to respond to a knob turn — fixed).
- Tested on Linux before shipping (all chain tests pass): steady
  white noise at -20 dBFS dropped to about -60 dBFS (roughly 40 dB
  reduction — 39.5 to 40.9 dB across runs, the test noise is random);
  an off-pitch 228 Hz test note was pulled to 220.5 Hz, 4 cents off
  A3 (220 Hz); silence stays silent; a two-note chord does not crash
  or clip; all five presets run clean with no NaN and peaks at or
  below 1.0; the preamp, Air button, reverb types, delay divisions
  and EQ curve are all covered by tests. Real results depend on the
  recording.

## Files in this project

- `CMakeLists.txt` — the build file (JUCE is downloaded automatically)
- `Source/PluginProcessor.h` / `.cpp` — the whole vocal chain
- `Source/PluginEditor.h` / `.cpp` — the black-and-gold interface
- `Source/RNNoise/` — the RNNoise C sources + the `sh.rnnn` model
- `test/vocal_presets_test.cpp` — a test program that feeds generated
  audio through the chain and reports measured results
- `.github/workflows/build-macos.yml` — the cloud build

## Credits

- RNNoise by Jean-Marc Valin / Mozilla / Xiph.Org (BSD licence)
- "sh" model by Gregor Richards (rnnoise-models, BSD licence)
- Built with JUCE 8 (GPLv3 / JUCE licence — see JUCE's licence terms)
- Famous Beats Studio
