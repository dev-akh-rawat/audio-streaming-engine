# Build and run commands

cmake -S . -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

cmake --build build

./build/audio_streaming_engine

# Audio Streaming Engine

A C++20 real-time audio streaming engine designed to process continuous audio data with low latency, safe multithreading, and clean architectural boundaries.

This project focuses on **systems-level engineering concepts** commonly used in audio devices, media pipelines, and low-latency streaming systems.

---

## ✨ Goals

- Build a **real-time safe audio engine** in modern C++
- Explore **lock-free data structures** and threading models
- Design clean, extensible **streaming and rendering abstractions**
- Support **local playback and network-based audio streaming**
- Emphasize **clarity, correctness, and performance**

This is not a UI application.  
It is an **engine-level project** focused on architecture and execution models.

---

## 🛠️ Tech Stack

- **Language:** C++20
- **Build System:** CMake + Ninja
- **Compiler:** Clang (Apple Clang on macOS)
- **Platform:** macOS (Core Audio backend)
- **Networking:** UDP (BSD sockets)
- **Editor:** VS Code

---

## 📁 Project Structure (evolving)

audio-streaming-engine/
├── CMakeLists.txt
├── src/
│ ├── core/ # Engine orchestration
│ ├── buffer/ # Lock-free buffers
│ ├── stream/ # Audio sources (file, network, generators)
│ ├── output/ # Platform-specific audio output
│ └── main.cpp
├── tests/
├── benchmarks/
├── docs/
│ └── architecture.md
└── README.md


---

## 🧭 Development Roadmap

The project is developed incrementally, with each phase introducing a core system concept used in real-world audio and streaming systems.

---

### Phase 1: Project Skeleton & Tooling
- Set up a clean C++20 project using **CMake + Ninja**
- Verify compiler, threading, and build configuration
- Establish a stable baseline for further development

**Goal:** Ensure a reliable foundation before adding real-time components.

---

### Phase 2: Lock-Free Buffering Primitives
- Implement a **single-producer single-consumer (SPSC) ring buffer**
- Use `std::atomic` for synchronization
- Avoid dynamic allocation in hot paths
- Ensure cache-friendly memory layout

**Goal:** Enable real-time safe data transfer between threads.

---

### Phase 3: Streaming Abstractions
- Define core data structures:
  - `AudioFrame`
  - `AudioStream` interface
- Implement simple stream sources:
  - Synthetic generator
  - File-based stream
- Decouple data production from consumption

**Goal:** Create a modular and extensible streaming pipeline.

---

### Phase 4: Threaded Engine Core
- Introduce an engine orchestration layer
- Run:
  - Producer thread (stream source)
  - Consumer thread (renderer)
- Integrate ring buffers into execution flow
- Handle underrun and backpressure scenarios

**Goal:** Model a realistic audio engine execution architecture.

---

### Phase 5: Platform Output Backend (macOS)
- Implement an audio output backend using **Core Audio**
- Connect engine output to the system audio device
- Enforce real-time safe constraints inside audio callbacks

**Goal:** Achieve real audio playback on the host system.

---

### Phase 6: Network Audio Streaming
- Stream audio over **UDP**
- Implement:
  - Packetization
  - Jitter buffering
  - Packet reordering and loss handling
- Support local loopback testing

**Goal:** Enable low-latency network-based audio streaming.

---

### Phase 7: Clock Synchronization & Drift Handling
- Timestamp audio frames
- Detect clock drift between sender and receiver
- Apply gradual correction strategies:
  - Frame drop or duplication
  - Minor rate adjustments

**Goal:** Maintain long-term audio synchronization.

---

### Phase 8: Metrics, Logging & Observability
- Instrument pipeline stages
- Track:
  - Buffer depth
  - End-to-end latency
  - Underruns and overruns
- Provide lightweight logging and metrics output

**Goal:** Improve debuggability and performance insight.

---

### Phase 9: Testing & Benchmarking
- Unit test core components (buffers, streams)
- Add micro-benchmarks for:
  - Throughput
  - Latency
- Validate real-time safety assumptions

**Goal:** Ensure correctness and predictable performance.

---

### Phase 10: Documentation & Refinement
- Document architecture and design decisions
- Add diagrams for:
  - Data flow
  - Threading model
  - Network pipeline
- Identify future extensions and limitations

**Goal:** Make the project easy to understand, evaluate, and extend.

---

## 🔑 Guiding Principles

- Favor **clarity over cleverness**
- Keep real-time paths **allocation-free**
- Separate platform-specific code from core logic
- Validate assumptions incrementally
- Treat tooling and observability as first-class concerns

---

## 🚀 Status

This project is actively under development.  
Each phase is implemented and validated before moving to the next.

---

## 📌 Motivation

This project is built to deepen understanding of:
- Real-time systems
- Multithreading and lock-free programming
- Audio pipelines
- Low-latency networking
- Production-quality C++ architecture
