# Dual Readout TMVA

This project goal is to facilitate particle identification process by means of application of Machine Learning algorithms. Hardware setup is outlined below. Incident particle interacts with the scintilation crystal and may produce two types of signals. Depending on the particle type, only Cerenkov radiation is produced (e.g. pions) or Cerenkov radiation is accompanied with scintillation radiation (for e- or gamma).

<figure>
  <img src="https://raw.githubusercontent.com/petrstepanov/dual-readout-tmva/main/resources/setup.png" alt="Particle identification setup schematics" />
</figure>

Tektronix oscilloscope MDO4034C was set up with external trigger by means of [the PyVisa library](https://github.com/petrstepanov/tek). A total of 16064 spectra for Cerenkov-only particles (experimental sample "Cube 6") and 33056 spectra for Cerenkov and scintillation signal (sample "Cube 9") are acquired by [Vladimir Berdnikov](berdnik@jlab.org). Waveforms are stored on the `pristine@jlab.org` machine under the `/misc/cuanas/Data` folder.

Two sample waveforms for each signal type are visualized below. Top histograms represent original waveform from the oscilloscope. Bottom images are zoomed in signals. On the left we observe a Cerenkov only signal. On the right the signal is superposition of the Cerenkov effect with a longer scintillation "tail". 

<figure>
  <img src="https://raw.githubusercontent.com/petrstepanov/dual-readout-tmva/main/resources/spectra.png" alt="Two types of signals for the binary classification" />
</figure>

We observe visual discrepancy in the shape of the waveforms from different particle types. Machine Learning (ML) can be applied to classify particles into two known groups.

## Program Description

This program demonstrates that ML techniques can be successfully applied to perform classification of "unknown" experimantal spectra. Generally speaking ML technique consists of three stages:

* **Preparation**. Oscilloscope waveforms are processed and converted into a ROOT tree of a certain format to be loaded into the ML algorithm(s).
* **Training**. Known input data that corresponds to each classification group is given to the ML algorithm.
* **Classification**. Unknown spectra are analyzed by the trained algorithm and classified into groups.

Program should be compiled and executed on the JLab computing farm. First two stages are the most resourceful ones. Classification can be carried out on a local computer. Below we elaborate on the program workflow, syntax and provide the end result of the classification.

### Preparation Stage

Waveform CSV files for each classification group hosted on the `pristine@jlab.org` are copied to the `/w/hallc-scshelf2102/kaon/petrs/Data/Cubes` folder for processing. Group rename operation is preformed to give unique name to each .csv file (with the help of `rename '' 'Mar14_' *.csv` command).

It is necessary to exclude the "testing" data from the data for ML training process. Therefore, randomly selected 600 spectra from each category (a total of 1200 files) are moved to the `/w/hallc-scshelf2102/kaon/petrs/Data/Cubes-processed/samples-testing` folder. Rest of the spectra from each category are consolidated in following folders:
* **Cerenkov-only** spectra are stored under:<br/>`/w/hallc-scshelf2102/kaon/petrs/Data/Cubes-processed/sample6-learning/`
* **Cerenkov and scintillation** are copied to:<br/>`/w/hallc-scshelf2102/kaon/petrs/Data/Cubes-processed/sample9-learning/`

During the preparation stage the .csv files are imported into ROOT histograms. Some spectra from the oscilloscope do not contain neither Cerenkov nor scintillation signals. Those are noise spectra. Program creates and visualizes a ROOT file with waveform name, minimum voltage value (signal is negative) and the peak position. Currently noise waveforms are manually filtered out from all the waveforms upon following criteria:
* Amplitude threshold value of a "good" waveform should be < 0.03 V. 
* Peak position in a reange of -10 to 20 ns.

Below the waveform criteria are visualized for the set of Cherenkov and scintillation (Cube 9) sample:

<figure>
  <img src="https://raw.githubusercontent.com/petrstepanov/dual-readout-tmva/main/resources/waveform-criteria.png" alt="Criteria for filtering out the noise waveforms" />
</figure>

**Further improvement** of the program is implementation of AI-based classification of the noise spectra into a separate group. 

Noise waveforms are filtered out. "Good" waveforms are now used to create an input data for the ML algorithm. There are two ways of preparing ROOT data for machine learning. Starting ROOT v6.20 a new method for the ML tree preparation, namely writing all the histogram bin values into a single tree leaf was implemented (refer to the image below). Unfortunately this method failed to provide correct classification results. An error in the ROOT code was found and [reported in this Pull Request](https://github.com/root-project/root/pull/10780). In order to bea able to run the program on the JLab farm, input data was formatted in a traditional way, where every ML variable (histogram bin) is stored in a separate tree leaf. Tree structures for both - modern and traditional approaches are visualized below.

<figure>
  <img src="https://raw.githubusercontent.com/petrstepanov/dual-readout-tmva/main/resources/tree.png" alt="Creating ROOT tree with data for TMVA Machine Learning" />
</figure>

Two known copied to Examples 

### Training Stage

### Classification Stage

## Program Build and Run

To reproduce obtained results program code needs to be checked out to the JLab computer environent.

* Log in to the computing farm `ssh <your-username>@login.jlab.org`.
* Connect to one of the ifarm nodes `ssh ifarm`.
* Clone the program code: `git clone https://github.com/petrstepanov/dual-readout-tmva`.
* Source the environment `source /site/12gev_phys/softenv.csh 2.5`.
* Create a folder for the out-of-source build: `mkdir dual-readout-tmva-build && cd dual-readout-tmva-build`.
* Generate the makefile with CMake: `cmake ../dual-readout-tmva`.
* Build the source code: ``make -j`nproc` ``.

Executable `dual-readout-tmva` will be generated inside the current folder. Pprogram mode (preparation, training or classification) and paths to the source directories containing input data are passed as command-line parameters.

### Preparation Stage

First we run the program in the preparation stage providing paths to source folders with known types:

```
./dual-readout-tmva --mode prepare --background <cerenkov-waveforms-path> --signal <cerenkov-and-scintillation-path>
```

where `<cerenkov-waveforms-path>` and `<cerenkov-and-scintillation-path>` are folder paths of the Cube 6 and Cube 9 waveforms respectively.

Program outputs the `tmva-input.root` file containing processed non-noise waveforms written in a ROOT tree under the `treeB` (background, Cerenkov only) and `treeS` (signal, Cerenkov and scintillation) branches.

## Future goals

Some of the acquired experimental spectra are simply noise that does not contain any meaningful data. This happens due to some challenges in the experimental setup assembly. Spectra are visualized below:

<figure>
  <img src="https://raw.githubusercontent.com/petrstepanov/dual-readout-tmva/main/resources/noise.png" alt="Example noise specrtra to be classified with AI ROOt TMVA" />
</figure>

Technically a multi-class classification can be performed to effectively sort out noise spectra into a separate group (class of signals). This will lift the necessity of filtering the noise spectra before carrying out the analysis.