# Componenti di terzi e licenze

Il codice del gioco è sotto licenza MIT (`LICENSE`). Grafica, musiche, effetti, storie e personaggi
sono originali e generati dagli script di questo repository (`tools/art`, `tools/audio`, `data/voce/frasi.csv`):
nessun personaggio, logo, brano o immagine di terzi.

## Nel gioco (compilati nel core)

| Componente | Versione | Licenza | Uso |
| --- | --- | --- | --- |
| `libretro.h` (The RetroArch team) | API libretro 1 | MIT | l'interfaccia del core con RetroArch |
| `stb_image.h` (Sean Barrett) | 2.30 | pubblico dominio oppure MIT, a scelta | decodifica delle PNG (solo PNG) |
| `stb_vorbis.c` (Sean Barrett) | 1.22 | pubblico dominio oppure MIT, a scelta | decodifica delle voci e delle musiche OGG |

## Solo negli strumenti (non finiscono nel core né nei pacchetti RetroArch)

| Componente | Licenza | Uso |
| --- | --- | --- |
| `stb_image_write.h` 1.16 (Sean Barrett) | pubblico dominio oppure MIT | l'harness di test scrive le schermate |
| sherpa-onnx (esegue il modello Piper) | Apache-2.0 | `tools/voice/gen_voice.py` produce i file della voce |
| espeak-ng-data (fonemi, nel pacchetto del modello) | GPL-3.0 | letti da sherpa-onnx durante la sintesi; non sono inclusi nel gioco |
| ffmpeg con libvorbis | LGPL-2.1+ / BSD-3-Clause | codifica delle frasi in OGG |
| Whisper (modelli per sherpa-onnx) | MIT | `tools/voice/check_voice.py` controlla che le frasi si capiscano |
| Pillow, NumPy | MIT-CMU, BSD-3-Clause | generazione della grafica |
| Zig (compilatore C per aarch64 e x86_64) | MIT | build di rilascio |

## La voce

Le frasi (`data/deva_adventures/voce/*.ogg`) sono generate offline con il modello Piper
**it_IT-paola-medium**. Verificato il 2 ottobre 2026 sulle schede dei modelli:

| Che cosa | Fonte | Licenza |
| --- | --- | --- |
| dataset della voce «paola» | [paolapersico1/Voice-Dataset-Italian](https://huggingface.co/datasets/paolapersico1/Voice-Dataset-Italian) | CC0-1.0 |
| modello it_IT-paola-medium | [scheda del modello](https://huggingface.co/rhasspy/piper-voices/raw/main/it/it_IT/paola/medium/MODEL_CARD) | «addestrato a partire dalla voce inglese lessac (medium)» |
| modello di partenza en_US-lessac-medium | [scheda del modello](https://huggingface.co/rhasspy/piper-voices/raw/main/en/en_US/lessac/medium/MODEL_CARD) | dataset Lessac Blizzard 2013: [licenza](https://www.cstr.ed.ac.uk/projects/blizzard/2013/lessac_blizzard2013/license.html) solo per ricerca, niente uso commerciale né redistribuzione |

In pratica:

- **Uso in famiglia** (il gioco sulla console di casa): nessun problema.
- **Pubblicare il gioco** (un repository pubblico, un pacchetto scaricabile): il dataset di paola è
  libero, ma il modello deriva da uno addestrato su un corpus con licenza di sola ricerca. Che le frasi
  generate ne siano un'opera derivata è discutibile; per non doverlo stabilire, prima di pubblicare
  conviene sostituire la voce: registrare le frasi in `data/voce/registrate/<id>.wav` (lo stesso id di
  `data/voce/frasi.csv`, vincono sulla sintesi) oppure rigenerarle con una voce addestrata solo su dati
  con licenza libera, e controllarle con `tools/voice/check_voice.py`.

## Testi delle licenze

- MIT (questo gioco): `LICENSE`.
- `libretro.h`, `stb_image.h`, `stb_vorbis.c`: il testo della licenza è in fondo a ciascun file in
  `third_party/`.
