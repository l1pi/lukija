# Lukija

A Sailfish OS app that reads PDF files aloud with offline text-to-speech, made for ebooks and academic articles. See [PLAN.md](PLAN.md) for the design and roadmap.

**Status:** Phase 1. The text pipeline and a desktop command-line tool work; the Sailfish app doesn't exist yet.

## Building on desktop

Arch / CachyOS:

```sh
sudo pacman -S --needed cmake ninja qt5-base qt5-multimedia poppler-qt5
cmake -S . -B build -G Ninja
cmake --build build
```

The first build downloads and builds [libpiper](https://github.com/OHF-Voice/piper1-gpl) (Piper TTS), which fetches onnxruntime and espeak-ng. Configure with `-DLUKIJA_WITH_PIPER=OFF` to build without it.

Keep the build directory's path short, under about 60 characters: espeak-ng's build can't handle long paths. CMake stops with a message if the path is too long.

`core/` must stay compatible with Qt 5.6, the Qt version on Sailfish OS, even though desktop builds use Qt 5.15.

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

`speak` prints each sentence as it's spoken and, at the end, the synthesis real-time factor.

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
tts/       TTS engine interface, Piper engine, voice lookup
audio/     PCM helpers and AudioSink (QAudioOutput)
tools/     lukija-cli, fetch-voice.sh
tests/     unit tests and the golden-file corpus
3rdparty/  libpiper build
```

## Licence

GPL-3.0. See [LICENSE](LICENSE).
