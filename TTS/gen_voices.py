import csv, os, torch
from TTS.api import TTS

os.environ["COQUI_TOS_AGREED"] = "1"   # accetta licenza CPML (non commerciale)

OUT_DIR, REF_DIR = r"C:\TTS\out", r"C:\TTS\refs"
CSV_PATH = r"C:\TTS\lines.csv"         # colonne: id,speaker,text
os.makedirs(OUT_DIR, exist_ok=True)

device = "cuda" if torch.cuda.is_available() else "cpu"
print("Device:", device)
tts = TTS("tts_models/multilingual/multi-dataset/xtts_v2").to(device)

with open(CSV_PATH, encoding="utf-8") as f:
    for row in csv.DictReader(f):
        out = os.path.join(OUT_DIR, f"{row['id']}.wav")
        if os.path.exists(out): print("skip", out); continue
        tts.tts_to_file(
            text=row["text"],
            speaker_wav=os.path.join(REF_DIR, f"{row['speaker']}_ref.wav"),
            language="it", file_path=out, split_sentences=True,
            temperature=0.65, repetition_penalty=5.0, speed=0.95)
        torch.cuda.empty_cache()       # evita OOM se UE è aperto
        print("ok ", out)