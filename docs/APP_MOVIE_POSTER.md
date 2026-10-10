# App movie first-frame preview

The resource browser now has a movie preview page and a main-menu entry. Enter a
relative movie path under the selected resource root and click Load first frame.
Clear removes the image. Preview does not play the movie or start audio.

The App calls the shared `AppMedia::moviePoster(relative)` implementation, then
uses the existing `ImagePresenter` contract. ResourceRoot rejects empty, missing,
absolute, traversal and escaping symlink paths. MovieReader extracts frame zero;
the adapter copies each BGRA row into a tight 64x48 image, discards row padding
and produces opaque pixels. Its local Reader closes before return, including
exception and invalid-frame paths. Failed replacements and image conversion
clear the preceding image.

The accepted fixture is a self-generated 8-second, 64x48 H264/LPCM MOV, with
30fps video and 48kHz stereo PCM. The existing Reader checks 8-second duration,
dimensions and the presence of one video and one audio track at open. This
preview checks only the first frame's PTS and pixel layout; it does not inspect
later cadence or audio packets. These tests do not establish general movie,
original-asset or Bink compatibility.

Build provenance and the portable runner include `stream_lab/movie_io.mm` and
`video_lab/stream.cpp`, their headers, and the required system frameworks. The
default source export still needs no city prototype or game data. The generated
MOV bytes are captured in the integration input evidence before decoding.

Validation uses the production control and synthetic files, plus a fake image
presenter and fake audio factory. It checks first-frame colours against the
generator's independent colour contract, packed pixels, invalid replacements
and cleared images. The App is compiled and packaged for signature and system
link checks; it is never launched by these commands:

```sh
python3 -I -B /absolute/export/scripts/independent_runner.py integration --mode release --output /absolute/results
python3 -I -B /absolute/export/scripts/independent_runner.py integration --mode undefined --output /absolute/results
python3 -I -B /absolute/export/scripts/independent_runner.py app --mode release --output /absolute/results
python3 -I -B /absolute/export/scripts/independent_runner.py verify --report /absolute/results/RUN_ID
```

Each process has a 60-second limit. Fixture generation needs the system
AVFoundation media services; a failed writer fails the suite. GUI conversion,
human-visible output, sound and original game behaviour remain unverified.
