# Manual Test Guide for RTK/Headroom/Caveman Filter

## Prerequisites

* **Hardware** – A free GPU (e.g., NVIDIA RTX 3050 or Tesla P40) with at least 8 GB VRAM. The filter is designed to run on the **35 B** model.
* **Model** – `Qwen3.6-35B-A3B-UD-IQ4_XS.gguf` located in the project directory.
* **Environment** – The project has been built with `llama.cpp` and the filter module is compiled into the binary `rtk_headroom_caveman`. Ensure the binary is executable.

## 1. Verify GPU Availability

```bash
# Show all GPUs and their current usage
nvidia-smi
```

Look for a GPU with **free memory**. If multiple GPUs are present, you can specify the device with the `CUDA_VISIBLE_DEVICES` environment variable.

## 2. Run the Filter

```bash
# Example: run the filter on a sample prompt
CUDA_VISIBLE_DEVICES=0 ./rtk_headroom_caveman \
  --model Qwen3.6-35B-A3B-UD-IQ4_XS.gguf \
  --prompt "Ciao, come va?" \
  --max-tokens 256 \
  --temperature 0.7 \
  --headroom 0.1 \
  --verbose
```

* `--headroom` – percentage of the model’s context that should be kept free for future tokens. Adjust between `0.05` and `0.2`.
* `--max-tokens` – maximum number of tokens to generate.
* `--temperature` – sampling temperature.
* `--verbose` – prints internal statistics (token count, VRAM usage, etc.).

## 3. Measure VRAM Usage

While the program is running, open another terminal and run:

```bash
nvidia-smi --query-gpu=memory.used,memory.free --format=csv -l 1
```

This will print the used and free memory every second. Observe the peak usage during generation.

## 4. Measure Throughput

The binary prints the number of tokens generated and the elapsed time when `--verbose` is enabled. To compute tokens per second (throughput), use the output:

```
Tokens generated: 256
Time elapsed: 1.23s
```

Throughput = `256 / 1.23 ≈ 208 tokens/s`.

## 5. Adjust Parameters

* **Headroom** – Lower values increase throughput but risk running out of context. Increase if you hit context‑limit errors.
* **Temperature** – Higher values produce more diverse output.
* **Max‑tokens** – Larger values consume more VRAM.

Re‑run the command with new values and compare the `nvidia-smi` output.

## 6. Clean Up

After testing, you can stop the process with `Ctrl+C`. No temporary files are created.

---

**Note**: All paths in this guide are relative to the project root. Do not hard‑code absolute paths or sensitive information.