// rtk_headroom_caveman.cpp
// Implementazione locale di filtraggio token (rtk, headroom, caveman) per llama.cpp
// Path relativo, senza riferimenti assoluti.

#include <vector>
#include <unordered_map>
#include <cmath>
#include <algorithm>

// Rappresentazione semplificata di un token generato da llama.cpp
struct Token {
    int id;            // ID del token (llama_token)
    float logprob;     // Log‑probabilità del token (valore negativo, più piccolo = meno probabile)
};

// Configurazione per rtk (Reduced Token Knowledge)
struct RtkConfig {
    int max_ngram = 3;          // Lunghezza massima n‑grammi da deduplicare
    float entropy_thr = 0.15f;  // Soglia di entropia sotto la quale il token è considerato prevedibile
};

// Configurazione per caveman (post‑processing output)
struct CavemanConfig {
    float prob_thr = -5.0f;      // Log‑probabilità minima per mantenere il token
    int dup_ngram_len = 4;       // Lunghezza n‑grammi da considerare per rimozione duplicati
};

// Configurazione per headroom (wrapper che combina rtk e caveman)
struct HeadroomConfig {
    bool enable_input = true;   // Applica rtk sull'input
    bool enable_output = true;  // Applica caveman sull'output
    int max_output_len = 2048;  // Lunghezza massima dell'output dopo il filtraggio
    RtkConfig rtk_cfg;
    CavemanConfig caveman_cfg;
};

// ---------- Helper utilities ----------

// Calcola l'entropia di un token basata sulla sua frequenza nella sequenza.
static float token_entropy(const std::vector<int>& seq, int token) {
    if (seq.empty()) return 0.0f;
    size_t count = 0;
    for (int t : seq) if (t == token) ++count;
    float p = static_cast<float>(count) / static_cast<float>(seq.size());
    if (p <= 0.0f) return 0.0f;
    return -p * std::log2(p);
}

// Rimuove n‑grammi duplicati (fino a max_len) dalla sequenza di token.
static std::vector<int> dedup_ngrams(const std::vector<int>& tokens, int max_len) {
    std::vector<int> out;
    for (size_t i = 0; i < tokens.size(); ++i) {
        bool duplicate = false;
        for (int n = 2; n <= max_len && i + n <= tokens.size(); ++n) {
            // confronta il n‑gramma corrente con quello precedente immediato
            bool same = true;
            for (int k = 0; k < n; ++k) {
                if (tokens[i + k] != tokens[i + k - n]) { same = false; break; }
            }
            if (same) { duplicate = true; break; }
        }
        if (!duplicate) out.push_back(tokens[i]);
    }
    return out;
}

// ---------- Filtri ----------

// rtk: rimuove token a bassa entropia e deduplica n‑grammi.
static std::vector<int> rtk_filter(const std::vector<int>& input, const RtkConfig& cfg) {
    // Prima fase: rimuovi token con entropia < soglia
    std::vector<int> filtered;
    for (int token : input) {
        float ent = token_entropy(input, token);
        if (ent >= cfg.entropy_thr) {
            filtered.push_back(token);
        }
    }
    // Seconda fase: deduplica n‑grammi fino a max_ngram
    if (cfg.max_ngram > 1) {
        filtered = dedup_ngrams(filtered, cfg.max_ngram);
    }
    return filtered;
}

// caveman: rimuove token con log‑probabilità bassa e n‑grammi duplicati.
static std::vector<Token> caveman_filter(const std::vector<Token>& input, const CavemanConfig& cfg) {
    // Prima fase: filtra per log‑probabilità
    std::vector<Token> prob_filtered;
    prob_filtered.reserve(input.size());
    for (const auto& t : input) {
        if (t.logprob >= cfg.prob_thr) {
            prob_filtered.push_back(t);
        }
    }
    // Seconda fase: rimuovi n‑grammi duplicati basati sugli ID dei token
    if (cfg.dup_ngram_len > 1) {
        std::vector<int> ids;
        ids.reserve(prob_filtered.size());
        for (const auto& t : prob_filtered) ids.push_back(t.id);
        std::vector<int> deduped_ids = dedup_ngrams(ids, cfg.dup_ngram_len);
        // Ricostruisci vettore Token mantenendo l'ordine originale dei token rimasti
        std::vector<Token> out;
        out.reserve(deduped_ids.size());
        for (int id : deduped_ids) {
            auto it = std::find_if(prob_filtered.begin(), prob_filtered.end(), [&](const Token& tk){ return tk.id == id; });
            if (it != prob_filtered.end()) out.push_back(*it);
        }
        return out;
    }
    return prob_filtered;
}

// headroom: combina rtk sull'input, chiama il core di llama (non implementato qui) e poi caveman sull'output.
// Per scopi di questo modulo, la funzione accetta già i token di output e li filtra.
static std::vector<Token> headroom_process(const std::vector<Token>& output, const HeadroomConfig& cfg) {
    std::vector<Token> result = output;
    if (cfg.enable_output) {
        result = caveman_filter(result, cfg.caveman_cfg);
    }
    // Troncamento alla lunghezza massima configurata
    if (cfg.max_output_len > 0 && static_cast<int>(result.size()) > cfg.max_output_len) {
        result.resize(cfg.max_output_len);
    }
    return result;
}

// API compatibile con llama.cpp: filtra token in‑place.
// La funzione accetta un vettore di Token (output del modello) e lo modifica secondo la configurazione di default.
void filter_tokens(std::vector<Token>& tokens) {
    // Configurazione di default – può essere sovrascritta in futuro leggendo un file YAML.
    static const HeadroomConfig default_cfg{};
    // Applica rtk sull'input se abilitato – qui consideriamo i token come ID grezzi.
    if (default_cfg.enable_input) {
        // Convertiamo Token -> int per rtk, poi riconvertiamo.
        std::vector<int> ids;
        ids.reserve(tokens.size());
        for (const auto& t : tokens) ids.push_back(t.id);
        ids = rtk_filter(ids, default_cfg.rtk_cfg);
        // Ricostruisci Token mantenendo logprob originale (se presente) per gli ID rimasti.
        std::vector<Token> filtered;
        filtered.reserve(ids.size());
        for (int id : ids) {
            auto it = std::find_if(tokens.begin(), tokens.end(), [&](const Token& tk){ return tk.id == id; });
            if (it != tokens.end()) filtered.push_back(*it);
        }
        tokens.swap(filtered);
    }
    // Applica caveman (output) tramite headroom_process
    tokens = headroom_process(tokens, default_cfg);
}

// Export symbols for C linkage (optional, useful when linking with C code)
extern "C" {
    void filter_tokens_c(void* token_array, int length) {
        // Cast a array of Token (assumed layout compatible) to std::vector<Token>
        Token* ptr = static_cast<Token*>(token_array);
        std::vector<Token> vec(ptr, ptr + length);
        filter_tokens(vec);
        // Copia indietro i risultati
        std::copy(vec.begin(), vec.end(), ptr);
    }
}

// Nota: per una reale integrazione con llama.cpp sarà necessario includere questo file
// nel CMakeLists.txt del progetto e collegare la libreria risultante.
