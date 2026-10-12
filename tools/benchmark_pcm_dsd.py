#!/usr/bin/env python3
"""Benchmark the deployed PCM2DSD library without opening an audio device."""
import argparse
import ctypes as C
import hashlib
import json
import math
from pathlib import Path
import time

class Config(C.Structure):
    _fields_ = [("struct_size", C.c_uint32), ("input_sample_rate", C.c_uint32),
                ("channels", C.c_uint32), ("dsd_multiplier", C.c_uint32),
                ("input_gain", C.c_double), ("dither_amplitude", C.c_double),
                ("seed", C.c_uint64)]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("library", type=Path)
    parser.add_argument("--rate", type=int, default=96000)
    parser.add_argument("--seconds", type=int, default=2)
    parser.add_argument("--multiplier", type=int, default=64)
    args = parser.parse_args()
    if args.seconds <= 0 or args.rate <= 0:
        parser.error("rate and seconds must be positive")
    lib = C.CDLL(str(args.library.resolve()))
    ptr, size = C.c_void_p, C.c_size_t
    lib.pcm_dsd_default_config.argtypes = [C.POINTER(Config)]
    lib.pcm_dsd_default_config.restype = None
    lib.pcm_dsd_create.argtypes = [C.POINTER(Config), C.POINTER(ptr)]
    lib.pcm_dsd_process.argtypes = [ptr, C.POINTER(C.c_double), size,
                                   C.POINTER(C.c_uint8), size, C.POINTER(size), C.POINTER(size)]
    lib.pcm_dsd_flush.argtypes = [ptr, C.POINTER(C.c_uint8), size, C.POINTER(size)]
    lib.pcm_dsd_destroy.argtypes = [ptr]
    lib.pcm_dsd_destroy.restype = None
    cfg = Config()
    lib.pcm_dsd_default_config(C.byref(cfg))
    cfg.input_sample_rate, cfg.dsd_multiplier = args.rate, args.multiplier
    handle = ptr()
    if lib.pcm_dsd_create(C.byref(cfg), C.byref(handle)) != 0:
        raise RuntimeError("converter creation failed")
    frames = args.rate * args.seconds
    samples = (C.c_double * (frames * 2))()
    for frame in range(frames):
        samples[frame * 2] = 0.25 * math.sin(2 * math.pi * 997 * frame / args.rate)
        samples[frame * 2 + 1] = 0.2 * math.cos(2 * math.pi * 1301 * frame / args.rate)
    output = (C.c_uint8 * 8192)()
    consumed, written = size(), size()
    digest, total_bytes, pos = hashlib.sha256(), 0, 0
    started = time.perf_counter()
    try:
        while pos < frames:
            source = C.cast(C.byref(samples, pos * 2 * C.sizeof(C.c_double)), C.POINTER(C.c_double))
            count = min(2048, frames - pos)
            status = lib.pcm_dsd_process(handle, source, count, output, len(output), C.byref(consumed), C.byref(written))
            if status != 0 or consumed.value > count or written.value > len(output) or not (consumed.value or written.value):
                raise RuntimeError(f"invalid process result: {status}")
            pos += consumed.value
            total_bytes += written.value
            digest.update(bytes(output[:written.value]))
        while True:
            status = lib.pcm_dsd_flush(handle, output, len(output), C.byref(written))
            if status not in (0, 3) or written.value > len(output):
                raise RuntimeError(f"flush failed: {status}")
            total_bytes += written.value
            digest.update(bytes(output[:written.value]))
            if status == 3:
                break
        elapsed = time.perf_counter() - started
    finally:
        lib.pcm_dsd_destroy(handle)
    print(json.dumps(dict(rate=args.rate, dsd_multiplier=args.multiplier,
                         audio_seconds=args.seconds, elapsed_seconds=elapsed,
                         realtime_ratio=args.seconds / elapsed,
                         output_bytes=total_bytes, sha256=digest.hexdigest()), indent=2))

if __name__ == "__main__":
    main()
