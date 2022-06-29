# Dual Readout TMVA

This project goal is to facilitate particle identification process by means of application of Machine Learning algorithms. Hardware setup is outlined below. Incident particle interacts with the scintilation crystal and may produce two types of signals. Depending on the particle type, only Cerenkov radiation is produced (e.g. pions) or Cerenkov radiation is accompanied with scintillation radiation (for e- or gamma).

<figure>
  <img src="https://github.com/petrstepanov/dual-readout-tmva/blob/master/resources/setup.png?raw=true" alt="Particle identification setup schematics" />
</figure>

Tektronix oscilloscope MDO4034C was set up with external trigger by means of [the PyVisa library](https://github.com/petrstepanov/tek). A total of 16064 spectra for Cerenkov-only particles (experimental sample "Cube 6") and 33056 spectra for Cerenkov and scintillation signal (sample "Cube 9") are acquired by [Vladimir Berdnikov](berdnik@jlab.org). Waveforms are stored on the `pristine@jlab.org` machine under the `/misc/cuanas/Data` folder.

Two sample waveforms for each signal type are visualized below. Top histograms represent original waveform from the oscilloscope. Bottom images are zoomed in signals. On the left we observe a Cerenkov only signal. On the right the signal is superposition of the Cerenkov effect with a longer scintillation "tail". 

<figure>
  <img src="https://github.com/petrstepanov/dual-readout-tmva/blob/master/resources/spectra.png?raw=true" alt="Two types of signals for the binary classification" />
</figure>

## Program Description

This program demonstrates that Machine Learning techniques can be successfully applied to perform classification of "unknown" experimantal spectra. Generally speaking ML technique consists of three stages:

* Preparation
* Training
* Classification

## Future goals

Some of the acquired experimental spectra are simply noise that does not contain any meaningful data. This happens due to some challenges in the experimental setup assembly. Technically a multi-class classification can be performed to effectively sort out noise spectra into a separate group (class of signals). This will lift the necessity of filtering the noise spectra before carrying out the analysis.