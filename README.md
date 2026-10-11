# Lukija

A Sailfish OS app that reads PDF files aloud with offline text-to-speech, made for ebooks and academic articles. See [PLAN.md](PLAN.md) for the design and roadmap.

**Status:** Phase 2. The app opens PDFs and reads them aloud on the Jolla Phone (Sailfish OS 5.2), resuming where you stopped. Article clean-up (headers, footnotes, references) is Phase 3.

## Building on desktop

Arch / CachyOS:

```sh
sudo pacman -S --needed cmake ninja qt5-base qt5-multimedia poppler-qt5
cmake -S . -B build -G Ninja
cmake --build build
```

The first build downloads [libpiper](https://github.com/OHF-Voice/piper1-gpl) (Piper TTS), espeak-ng and a prebuilt onnxruntime, with pinned checksums, and builds libpiper with a small patch (`3rdparty/patch-libpiper.cmake`). Builds without network access can point `LUKIJA_PIPER_URL`, `LUKIJA_ESPEAK_URL`, `LUKIJA_SONIC_URL` and `LUKIJA_ONNXRUNTIME_URL` at local copies. Configure with `-DLUKIJA_WITH_PIPER=OFF` to build without Piper.

Keep the build directory's path short, under about 60 characters: espeak-ng's build can't handle long paths. CMake stops with a message if the path is too long.

`core/` must stay compatible with Qt 5.6, the Qt version on Sailfish OS, even though desktop builds use Qt 5.15. The Sailfish build runs the tests on Qt 5.6 and catches newer APIs.

## Building for Sailfish OS

With the [Sailfish SDK](https://docs.sailfishos.org/Tools/Sailfish_SDK/) installed (more setup notes in [docs/spikes.md](docs/spikes.md)):

```sh
mkdir build-sfos && cd build-sfos
sfdk -c target=SailfishOS-5.1.0.11-aarch64 build ..
```

This builds `RPMS/harbour-lukija-*.aarch64.rpm` (the app, with libpiper and onnxruntime bundled) and `harbour-lukija-cli` (the command-line tool), and runs the tests under qemu. To install over USB with developer mode on:

```sh
scp RPMS/*.rpm defaultuser@192.168.2.15:/tmp/
ssh defaultuser@192.168.2.15 'devel-su pkcon -y install-local /tmp/harbour-lukija-*.rpm'
```

Voices aren't downloaded by the app yet: fetch them on the PC with `tools/fetch-voice.sh` and copy them to `~/.local/share/io.github.l1pi/harbour-lukija/voices/` on the phone.

## Voices

```sh
tools/fetch-voice.sh                        # fi_FI-harri-medium and en_US-lessac-medium
tools/fetch-voice.sh en_GB-alba-medium      # any id from huggingface.co/rhasspy/piper-voices
```

Voices go to `~/.local/share/io.github.l1pi/harbour-lukija/voices/`.

## lukija-cli

```sh
build/tools/lukija-cli/lukija-cli dump paper.pdf        # the text as it will be spoken
build/tools/lukija-cli/lukija-cli annotate paper.pdf    # layout blocks and their classification
build/tools/lukija-cli/lukija-cli speak paper.pdf       # read aloud; voice chosen by language
build/tools/lukija-cli/lukija-cli speak book.pdf --page 12 --speed 1.2
build/tools/lukija-cli/lukija-cli speak paper.pdf --count 20 -o sample.wav
build/tools/lukija-cli/lukija-cli voices
```

`speak` prints each sentence as it's spoken and, at the end, the synthesis real-time factor. `LUKIJA_PIPER_THREADS` sets the number of synthesis threads (default 2).

## Tests

```sh
tests/corpus/fetch.sh          # download the test corpus (once)
ctest --test-dir build --output-on-failure
```

- `tests/unit/`: unit tests per pipeline stage, plus pipeline tests on PDFs generated during the test run.
- `tests/corpus/`: real PDFs (CC BY papers in English and Finnish, a public-domain ebook) listed in `manifest.tsv`. Each test compares `lukija-cli dump` output with `golden/<id>.txt`. Corpus tests are skipped until the PDFs are fetched.

When a pipeline change alters the output on purpose, regenerate the golden files and review the diff before committing:

```sh
LUKIJA_UPDATE_GOLDEN=1 ctest --test-dir build -L corpus
git diff tests/corpus/golden
```

To add a document, append a line to `manifest.tsv` (use `-` as the checksum, run `fetch.sh`, then fill in the printed checksum), re-run CMake, and generate its golden file.

## Layout

```
core/      PDF text extraction and the text pipeline (plain Qt + poppler-qt5)
tts/       TTS engine interface, Piper engine, synthesis queue, voice lookup
audio/     PCM helpers and AudioSink (QAudioOutput)
app/       playback controller, library database, QML models; main.cpp is the Sailfish app
qml/       Silica UI
rpm/       RPM spec for sfdk and OBS
tools/     lukija-cli, fetch-voice.sh
tests/     unit tests and the golden-file corpus
3rdparty/  libpiper build (espeak-ng, onnxruntime)
docs/      Phase 0 spike results
```

## Licence

GPL-3.0. See [LICENSE](LICENSE).
