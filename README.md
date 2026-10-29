# Machine Learning De-Esser

A real-time audio plugin that uses machine learning for phonetic classification to selectively apply corrective DSP to sibilant frequencies. Created as part of a Master's Dissertation project at Queen Mary University of London (QMUL).

Current Status: **v1.0 (Testing & Academic Documentation)**

---

## Overview

The ML De-Esser combines real-time neural network inference with digital signal processing (DSP) to detect and reduce sibilance in voice/vocal recordings. 

> **Important Licensing Note on Models:**  
> The model was trained on academic datasets. Models provided within this repository are released strictly for academic study, testing, and documentation purposes. Commercial use of the pre-trained weights is **not permitted** due to dataset copyright restrictions.

---

## Installation & Setup

### Option 1: Use Pre-built Binary (Quickest)

1. Clone or download this repository.
2. Copy `ML De-Esser.vst3` into your system's default VST3 directory:
   * **macOS:** `~/Library/Audio/Plug-Ins/VST3/` or `/Library/Audio/Plug-Ins/VST3/`
   * **Windows:** `C:\Program Files\Common Files\VST3\`

---

### Option 2: Build from Source (JUCE)

#### Prerequisites
* [JUCE Framework](https://juce.com/)
* C++17 compatible compiler (Xcode / Visual Studio / GCC)
* [RTNeural](https://github.com/jatinchowdhury18/RTNeural) module for neural network inference

#### Build Steps
1. Clone the repository:
   ```bash
   git clone [https://github.com/your-username/ml-deesser.git](https://github.com/your-username/ml-deesser.git)
   ```
2. Add the `RTNeural` module to your JUCE project/Projucer configuration.
3. Replace or update the `Source/` files in your JUCE project with the files from this repository.
4. Set the path to your model JSON file:
    * Select your target `.json` weight file from the resources/ directory.
    * Update the path reference in `PluginProcessor.cpp`.
5. Build the target project in your IDE (Xcode, Visual Studio, or CMake).

---

## Contributing

Contributions, bug reports, and pull requests are welcome! Feel free to open an issue or submit a PR.

---

## Citation & Licensing

This project's code is licensed under the MIT License.

If you reference or build upon this project for academic or research work, please cite:

    Michael W., ML De-Esser, Master's Dissertation, Queen Mary University of London (August 19, 2025).

---

## Roadmap

    [ ] Transition model architecture from standard NN to RNN / LSTM for better dynamic tracking.

    [ ] Improve DSP smoothing filter algorithms to minimize processing artifacts.

*Last Updated: October 29, 2026*