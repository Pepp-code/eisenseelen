"""Generates the placeholder sounds as WAV files in SourceAssets/Audio. Needs only the Python standard library.

Same seed, same files: the sounds only change when this script changes.
"""
import math
import os
import random
import struct
import wave

RATE = 44100
OUTPUT_FOLDER = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "SourceAssets", "Audio")


def lowpass(samples, cutoff_at):
    """A one-pole filter whose cutoff in Hz changes over time. cutoff_at takes the position from 0 to 1."""
    result = []
    state = 0.0
    count = len(samples)
    for index, sample in enumerate(samples):
        cutoff = cutoff_at(index / count)
        alpha = 1.0 - math.exp(-2.0 * math.pi * cutoff / RATE)
        state += alpha * (sample - state)
        result.append(state)
    return result


def noise(seconds, seed):
    generator = random.Random(seed)
    return [generator.uniform(-1.0, 1.0) for _ in range(int(seconds * RATE))]


def envelope(samples, attack, release):
    """Fades in over the first attack seconds and decays exponentially, with release as the time constant."""
    result = []
    for index, sample in enumerate(samples):
        time = index / RATE
        rise = min(1.0, time / attack) if attack > 0 else 1.0
        result.append(sample * rise * math.exp(-time / release))
    return result


def bell(samples, peak_at):
    """Swells to its loudest point at peak_at (0 to 1) and fades out, the shape of something flying past."""
    count = len(samples)
    result = []
    for index, sample in enumerate(samples):
        position = index / count
        if position < peak_at:
            level = position / peak_at
        else:
            level = (1.0 - position) / (1.0 - peak_at)
        result.append(sample * level * level)
    return result


def tone(seconds, frequency_at, harmonics=(1.0,)):
    """A tone whose pitch follows frequency_at. Harmonics give the level of each overtone."""
    result = []
    phase = 0.0
    count = int(seconds * RATE)
    for index in range(count):
        phase += 2.0 * math.pi * frequency_at(index / count) / RATE
        result.append(sum(level * math.sin(phase * (number + 1)) for number, level in enumerate(harmonics)))
    return result


def mix(*parts):
    length = max(len(samples) for samples, _ in parts)
    result = [0.0] * length
    for samples, gain in parts:
        for index, sample in enumerate(samples):
            result[index] += sample * gain
    return result


def save(name, samples, peak=0.9):
    """Writes 16-bit mono, normalized to peak, with a short fade at the end so it never clicks."""
    highest = max(abs(sample) for sample in samples) or 1.0
    fade = min(int(0.01 * RATE), len(samples))
    frames = bytearray()
    for index, sample in enumerate(samples):
        level = min(1.0, (len(samples) - index) / fade)
        frames += struct.pack("<h", int(sample / highest * peak * level * 32767))
    with wave.open(os.path.join(OUTPUT_FOLDER, f"{name}.wav"), "wb") as file:
        file.setnchannels(1)
        file.setsampwidth(2)
        file.setframerate(RATE)
        file.writeframes(bytes(frames))


def swing_light():
    air = lowpass(noise(0.28, 1), lambda p: 500 + 3500 * math.sin(math.pi * p))
    return bell(air, 0.45)


def swing_heavy():
    air = lowpass(noise(0.45, 2), lambda p: 250 + 1800 * math.sin(math.pi * p))
    return bell(air, 0.55)


def hit():
    thump = envelope(tone(0.22, lambda p: 150 - 90 * p), 0.001, 0.06)
    crack = envelope(lowpass(noise(0.12, 3), lambda p: 6000 - 4000 * p), 0.0005, 0.018)
    return mix((thump, 1.0), (crack, 0.8))


def hit_heavy():
    thump = envelope(tone(0.4, lambda p: 110 - 70 * p), 0.001, 0.12)
    crack = envelope(lowpass(noise(0.2, 4), lambda p: 5000 - 3500 * p), 0.0005, 0.035)
    return mix((thump, 1.0), (crack, 0.9))


def block():
    clank = envelope(tone(0.25, lambda p: 520 - 120 * p, (1.0, 0.6, 0.35, 0.2)), 0.001, 0.09)
    crack = envelope(lowpass(noise(0.08, 20), lambda p: 7000 - 3000 * p), 0.0005, 0.02)
    return mix((clank, 0.8), (crack, 0.7))


def parry():
    ring = envelope(tone(0.5, lambda p: 1250 - 60 * p, (1.0, 0.5, 0.3)), 0.001, 0.2)
    crack = envelope(lowpass(noise(0.06, 21), lambda p: 9000 - 3000 * p), 0.0005, 0.015)
    return mix((ring, 0.7), (crack, 0.8))


def dash():
    air = lowpass(noise(0.22, 5), lambda p: 900 + 2600 * (1.0 - p))
    return bell(air, 0.2)


def launch_pad():
    rise = envelope(tone(0.4, lambda p: 160 + 500 * p, (1.0, 0.4)), 0.005, 0.2)
    air = bell(lowpass(noise(0.4, 6), lambda p: 800 + 3000 * p), 0.5)
    return mix((rise, 0.8), (air, 0.5))


def battle_horn():
    """A war horn instead of a voice: a rising note with strong overtones and a little vibrato."""
    def pitch(position):
        slide = 130 + 45 * min(1.0, position * 6)
        return slide * (1.0 + 0.012 * math.sin(2.0 * math.pi * 5.5 * position * 1.4))

    note = tone(1.4, pitch, (1.0, 0.8, 0.55, 0.4, 0.25, 0.12))
    swell = [
        sample * min(1.0, index / (0.12 * RATE)) * min(1.0, (len(note) - index) / (0.35 * RATE))
        for index, sample in enumerate(note)
    ]
    return lowpass(swell, lambda p: 1500 + 1500 * min(1.0, p * 4))


def enemy_windup():
    """A dry bone rattle: a few short clicks speeding up."""
    samples = [0.0] * int(0.4 * RATE)
    generator = random.Random(7)
    time = 0.0
    gap = 0.07
    while time < 0.36:
        start = int(time * RATE)
        click = envelope(lowpass(noise(0.03, generator.randint(0, 9999)), lambda p: 3500), 0.0005, 0.006)
        for index, sample in enumerate(click):
            if start + index < len(samples):
                samples[start + index] += sample
        time += gap
        gap *= 0.82
    return samples


def enemy_strike():
    """What a hit on the hero sounds like: dull and short."""
    return mix(
        (envelope(tone(0.2, lambda p: 95 - 40 * p), 0.001, 0.05), 1.0),
        (envelope(lowpass(noise(0.1, 8), lambda p: 1500), 0.0005, 0.02), 0.7),
    )


def enemy_death():
    """Bones falling apart: many clicks that get further apart."""
    samples = [0.0] * int(0.7 * RATE)
    generator = random.Random(9)
    time = 0.0
    gap = 0.012
    while time < 0.6:
        start = int(time * RATE)
        pitch = generator.uniform(1500, 5000)
        click = envelope(lowpass(noise(0.04, generator.randint(0, 9999)), lambda p, cutoff=pitch: cutoff), 0.0005, 0.01)
        level = generator.uniform(0.4, 1.0) * (1.0 - time)
        for index, sample in enumerate(click):
            if start + index < len(samples):
                samples[start + index] += sample * level
        time += gap
        gap *= 1.16
    return samples


def staff_shot():
    zap = envelope(tone(0.2, lambda p: 900 - 650 * p, (1.0, 0.5, 0.25)), 0.001, 0.06)
    air = envelope(lowpass(noise(0.2, 10), lambda p: 4000 - 2500 * p), 0.001, 0.05)
    return mix((zap, 0.8), (air, 0.6))


def fireball_cast():
    roar = bell(lowpass(noise(0.6, 11), lambda p: 300 + 900 * p), 0.35)
    body = envelope(tone(0.6, lambda p: 90 + 120 * p, (1.0, 0.6)), 0.03, 0.25)
    return mix((roar, 1.0), (body, 0.6))


def explosion_small():
    return mix(
        (envelope(tone(0.3, lambda p: 130 - 80 * p), 0.001, 0.07), 1.0),
        (envelope(lowpass(noise(0.3, 12), lambda p: 2500 - 2000 * p), 0.001, 0.05), 0.9),
    )


def explosion_big():
    return mix(
        (envelope(tone(0.9, lambda p: 85 - 50 * p), 0.002, 0.25), 1.0),
        (envelope(lowpass(noise(0.9, 13), lambda p: 1800 - 1500 * p), 0.002, 0.2), 1.0),
    )


SOUNDS = {
    "S_Hieb": swing_light,
    "S_HiebSchwer": swing_heavy,
    "S_Treffer": hit,
    "S_TrefferSchwer": hit_heavy,
    "S_Blocken": block,
    "S_Parade": parry,
    "S_Dash": dash,
    "S_Sprungfeld": launch_pad,
    "S_Schlachtruf": battle_horn,
    "S_GegnerAusholen": enemy_windup,
    "S_GegnerSchlag": enemy_strike,
    "S_GegnerTod": enemy_death,
    "S_Feuerstab": staff_shot,
    "S_Feuerball": fireball_cast,
    "S_ExplosionKlein": explosion_small,
    "S_ExplosionGross": explosion_big,
}


def main():
    os.makedirs(OUTPUT_FOLDER, exist_ok=True)
    for name, make in SOUNDS.items():
        samples = make()
        save(name, samples)
        print(f"{name}: {len(samples) / RATE:.2f} s")


main()
