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

Program should be compiled and executed on the JLab computing farm. First two stages are the most compute intensive ones. Classification can be carried out on a local computer. Below we elaborate on the program workflow, syntax and provide the end result of the classification.

### Preparation Stage

Waveform CSV files for each classification group hosted on the `pristine@jlab.org` are copied to the `/w/hallc-scshelf2102/kaon/petrs/Data/Cubes` folder for processing. Group rename operation is preformed to give unique name to each .csv file (with the help of `rename '' 'Mar14_' *.csv` command).

It is necessary to exclude a subset of "testing" data for ML training process. Therefore, randomly selected 600 spectra from each category (a total of 1200 files) are moved to the `/w/hallc-scshelf2102/kaon/petrs/Data/Cubes-processed/samples-testing` folder. Rest of the spectra from each category are consolidated in following folders:
* **Cerenkov-only** spectra are stored under:<br/>`/w/hallc-scshelf2102/kaon/petrs/Data/Cubes-processed/sample6-learning/`
* **Cerenkov and scintillation** are copied to:<br/>`/w/hallc-scshelf2102/kaon/petrs/Data/Cubes-processed/sample9-learning/`

In this experiment the size of the trigger plates (top and bottom) is larger compared to the scintillation crystal. Therefore some trigger events are produced with a very weak to empty signal. **We will name these waveforms as the "baseline" waveforms**. A few of the "baseline" waveforms are visualized on the figure below:

<figure>
  <img src="https://raw.githubusercontent.com/petrstepanov/dual-readout-tmva/main/resources/noise.png" alt="Example set of baseline specrtra to be classified with AI ROOT TMVA" />
</figure>

On the other hand, when incident particle hits both trigger plates and the scintillation crystal, we obtain a waveform containing Cerenkov and (possibly) scintillation information.  **We will refer these waveforms as the "event" waveforms**. 

During the preparation stage the .csv files are imported into ROOT histograms. To separate the "baseline" spectra from the actual "event" signals (when scintillation signal is registeres), program creates and visualizes a ROOT file with a waveform name, minimum registered voltage value (signal is negative) and the peak position. Next, the "baseline" waveforms are manually filtered out from all the waveforms upon following criteria:

* Amplitude threshold value of a "event" waveform should be < 0.03 V. 
* Peak position in a reange of -10 to 20 ns.

Image below visualizes the criteria of filtering out noisy spectra (for the set of Cherenkov-only samples, Cube 9).

<figure>
  <img src="https://raw.githubusercontent.com/petrstepanov/dual-readout-tmva/main/resources/waveform-criteria.png" alt="Criteria for filtering out baseline waveforms" />
</figure>

The ratio of the "event" to "baseline" waveforms for two groups of samples is following:
* For **Cerenkov-only** spectra:<br/>`Identified 51% "event" waveforms (8226 files), 49% baseline waveforms (7837 files).`
* For **Cerenkov and scintillation** spectra:<br/>`Identified 56% "event" waveforms (18606 files), 44% baseline waveforms (14449 files).`

**Further improvement** of the program is implementation of AI-based classification of the baseline spectra into a separate group. 

Now that the "baseline" waveforms are filtered out, "event" signals are prepared to input to the ML algorithm. To my knowledge, some ML algorithms cannot work with negative variable data. Therefore, original waveforms (oscilloscope gives negative signal) are inverted. Next, negative values are set to zero. Additionally, we crop the the waveforms to exclude insignificant data. This processing is preformed in the `HistUtils::prepHistForTMVA()` method.

Next, the data is ready to be written into the ROOT tree of a special format for the TMVA analysis. There are two approaches of creating ROOT tree data for machine learning. Starting ROOT v6.20 a modern method for the ML tree preparation, where all the histogram bin values into a single tree branch as an array is implemented. Refer to the image below.

<figure>
  <img src="https://raw.githubusercontent.com/petrstepanov/dual-readout-tmva/main/resources/tree.png" alt="Creating ROOT tree with data for TMVA Machine Learning" />
</figure>

Unfortunately this method failed to provide correct classification results. An error in the ROOT code was found and [reported in this Pull Request](https://github.com/root-project/root/pull/10780). Tree structures for both - modern and traditional approaches are visualized below. As a temporary workaround to be able to run the program on the JLab farm, the input data was formatted in a traditional way, where every ML variable (histogram bin) is stored in a separate tree branch.

### Training Stage

Currently two ML alorithms are implemented in the code: boosted decision trees (BDT) and deep neural network (DNN). DNN algorithm requires splitting the input data into the "training" and "test" events. A ratio of 80÷20 for training-to-test events was selected respectively. Results of the training stage are presented on the graphs below:

<figure>
  <img src="https://raw.githubusercontent.com/petrstepanov/dual-readout-tmva/main/resources/plots/4b-overtraining-check.png" alt="TMVA Overtraining Check" />
</figure>

<figure>
  <img src="https://raw.githubusercontent.com/petrstepanov/dual-readout-tmva/main/resources/plots/5a-cut-efficiences.png" alt="TMVA Cut Efficiences" />
</figure>

<figure>
  <img src="https://raw.githubusercontent.com/petrstepanov/dual-readout-tmva/main/resources/plots/5b-signal-efficiency.png" alt="TMVA Signal Efficiency" />
</figure>

<figure>
  <img width="50%" src="https://raw.githubusercontent.com/petrstepanov/dual-readout-tmva/main/resources/plots/8-training-history.png" alt="TMVA Training History" />
</figure>

Training stage is rather resourceful. However it should be run only once. As the result of the training stage, TMVA outputs the so-called **weight" files** containing ML training information. A set of weight files each corresponding to the implemented ML algorithm (BDT and DNN) are used to classify the "unknown" waveforms without the need to re-train the model.

### Classification Stage

On this stage program takes a set of the "unknown" spectra and appliues the trained ML algorithm to determine if a spectrum shape corresponds to the **Cerenkov-only** or **Cerenkov with scintillation** category. 

A set of "unknown" spectra which is a random mix of Cerenkov and Cerenkov+scintillation spectra excluded from the training stage is analyzed by the AI algorithm. "Unknown" spectra are segregated under `/w/hallc-scshelf2102/kaon/petrs/Data/Cubes-processed/samples-testing` folder. 

The output of the classification stage for a particular spectrum is a float number in a range of [0, 1]. Classification results for unknown specrta are stored in the histograms and presented on the image below.

<figure>
  <img src="https://raw.githubusercontent.com/petrstepanov/dual-readout-tmva/main/resources/classification.png" alt="TMVA Classification results"/>
</figure>

Additionally program ouptuts the classification results in the Terminal output. There is a set of two classification results for each spectrum - for BDT and DNN classifiers.

```
Entry: 1
Filename: Mar28_DataLog_10236_7f6b_812b
MVA response for "TMVA_CNN_Classification_BDT.weights": 0.486826
MVA response for "TMVA_CNN_Classification_DNN.weights": 0.999035

Entry: 2
Filename: Mar28_DataLog_10237_7f6d_812b
MVA response for "TMVA_CNN_Classification_BDT.weights": 0.56271
MVA response for "TMVA_CNN_Classification_DNN.weights": 0.999979

Entry 3: 
Filename: Mar28_DataLog_10264_7fb0_812c
MVA response for "TMVA_CNN_Classification_BDT.weights": 0.378375
MVA response for "TMVA_CNN_Classification_DNN.weights": 0.999851

...
```

All information output by the program is currently stored in `/w/hallc-scshelf2102/kaon/petrs/Data/Cubes-results/TMVA-Jun23` folder. Next section describes how to reproduce the obtained results.

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

Program outputs the `tmva-input.root` file containing processed "event" waveforms written in a ROOT tree under the `treeB` (background, Cerenkov only) and `treeS` (signal, Cerenkov and scintillation) branches.

### Training Stage

Next, we train the ML algorithms by providing it with a two sets of "known" waveforms from two different sets:

```
./dual-readout-tmva --mode train <path-to-tmva-input-file>
```

During the training stage program outputs `ClassificationOutput.root` file containing training plots data along with the weight files. To run the TMVA GUI and view plots with training history, one can use following command:

```
./dual-readout-tmva --mode tmva-gui <path-to-classification-output-file>
```

where `<path-to-classification-output-file>` is the `ClassificationOutput.root` file path.

### Classification Stage

Finally, to proceed with the classification stage, program must be run with following command line parameters:

```
./dual-readout-tmva --mode classify --weight <weight-folder> --test <test-folder>
``` 

where `<weight-folder>` is a directory path where the weight files are stored, `<test-folder>` is a directory path containing the "unknown" waveforms to be classified.

Program outputs the classification information in the Terminal and additionally saves classification results in the output `TMVApp.root` file.

## Conclusion

In this work we successfully applied Machine Learning (ML) techniques to perform binary classification of the oscilloscope spectra upon their shape. 

* **First group** of spectra are obtaines from a crystal without scintillation centers (CUA sample #6). These waveforms contain only Cerenkov signal.

* **Second group** of spectra are obtained from a scintillator block (CUA sample #9). These waveforms represent superimposed Cerenkov and scintillation tail signals.

Program utilizes CERN ROOT TMVA framework to perform the classification. Currently two classifier techniques are implemented: Binary Desigion Trees (BDT) and Deep Neural Network (DNN). For each individual waveform from the testing set, program outputs two float numbers corresponding to the probabilities of belonging to each group. 

Classifier outputs for DNN and BDT algotithms do confirm each other for vast majority of the analyzed spectra. Therefore we can conclude that ML techniques can be successfully used for differentiating between the spectra shape and applied in the particle analysis procedure.

One of the improvements of the approach outlined in article is upgrading the algotithm to support the multi-class classification. For instance, Machine Learning algorithms can be trained to recognize the "baseline" spectra and attribute them into a thid class of signals.

This will lift the necessity of filtering the "baseline" spectra from the set of all spectra prior to the "learning" stage of the ML analysis.