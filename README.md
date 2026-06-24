# Implementazione locale di rtk/headroom/caveman per ridurre i

## Descrizione
Questo progetto fornisce un'implementazione locale dei moduli **rtk**, **headroom** e **caveman** per la riduzione del token usage nei modelli LLM. È stato integrato nel sistema di build di `llama.cpp` per consentire l'applicazione di filtri di riduzione token direttamente durante la compilazione e l'esecuzione dei modelli.

## Architettura
- **rtk**: modulo di token‑re‑ranking basato su regole.
- **headroom**: gestisce il margine di token per evitare overflow di contesto.
- **caveman**: algoritmo di riduzione aggressiva con fallback sicuro.
- Integrazione con `llama.cpp` tramite CMake e header condivisi.

## Installazione
```bash
# Clona il repository (se non già presente)
git clone <repo‑url> rtkheadroomcaveman
cd rtkheadroomcaveman

# Aggiorna il build system di llama.cpp
cd path/to/llama.cpp
mkdir -p build && cd build
cmake .. -DRTK_HEADROOM=ON -DCAVEMAN=ON
make -j$(nproc)
```
> **Nota**: sostituire `path/to/llama.cpp` con il percorso relativo al progetto `llama.cpp`.

## Uso
Una volta compilato, il binario accetta i seguenti flag aggiuntivi:
```
--rtk-filter          Abilita il filtro rtk
--headroom <bytes>    Imposta il margine di token (es. 1024)
--caveman-level <n>   Livello di riduzione (0‑3)
```
Esempio:
```bash
./main -m modello.gguf --rtk-filter --headroom 1024 --caveman-level 2 -p "Prompt di esempio"
```

## Esempi
```bash
# Esecuzione con riduzione moderata
./main -m model.gguf --rtk-filter --headroom 2048 --caveman-level 1 -p "Spiega la teoria della relatività"

# Esecuzione con riduzione aggressiva
./main -m model.gguf --rtk-filter --headroom 512 --caveman-level 3 -p "Genera un riassunto di 50 parole"
```

## Stato
- **Fase 1**: Requisiti definiti ✅
- **Fase 2**: Modulo di filtraggio token implementato ✅
- **Fase 3**: Integrazione nel build system completata ✅
- **Fase 4**: Guida di test manuale fornita ✅
- **Fase 5**: **COMPLETATO** (2026‑06‑18) ✅
