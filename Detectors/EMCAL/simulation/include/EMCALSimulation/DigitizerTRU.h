// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#ifndef ALICEO2_EMCAL_TRIGGERDIGITIZER_H
#define ALICEO2_EMCAL_TRIGGERDIGITIZER_H

#include <memory>
#include <unordered_map>
#include <vector>
#include <list>

#include "Rtypes.h"  // for DigitizerTRU::Class, Double_t, ClassDef, etc
#include "TObject.h" // for TObject
#include "TRandom3.h"

#include "DataFormatsEMCAL/Digit.h"
#include "EMCALBase/Hit.h"
#include "EMCALBase/TriggerMappingV2.h"
#include "EMCALSimulation/SimParam.h"
#include "EMCALSimulation/LabeledDigit.h"
#include "EMCALSimulation/DigitsWriteoutBufferTRU.h"
#include "EMCALSimulation/LZEROElectronics.h"
#include "SimulationDataFormat/MCTruthContainer.h"
#include "DataFormatsEMCAL/TriggerRecord.h"
#include "CommonUtils/TreeStreamRedirector.h"

namespace o2
{
namespace utils
{
class TreeStreamRedirector;
}
namespace emcal
{

/// \class DigitizerTRU
/// \brief EMCAL DigitizerTRU, digitizes with the help of a temporary description based upon a pol9*Heavyside
/// \ingroup EMCALsimulation
/// \author Anders Knospe, University of Houston
/// \author Hadi Hassan, ORNL
/// \author Simone Ragoni, Creighton
class DigitizerTRU : public TObject
{
 public:
  DigitizerTRU() = default;
  ~DigitizerTRU() override = default;
  DigitizerTRU(const DigitizerTRU&) = delete;
  DigitizerTRU& operator=(const DigitizerTRU&) = delete;

  void init();
  void clear();

  /// \brief Sets patches for the current geometry
  void setPatches();

  /// clear DigitsVectorStream
  void flush() { mDigits.flush(); }

  /// This is for the readout window that was interrupted by the end of the run
  void finish();
  // void finish() { mDigits.finish(); }

  /// Steer conversion of hits to digits
  void process(const gsl::span<const Digit> labeledDigit);
  // void process(const std::vector<Digit>& labeledDigit);

  /// Postprocessing of the digits, gathers by Fastors, not by Tower/Cell
  /// \param sdigits results of the SDigitizer
  std::vector<std::tuple<int, Digit>> makeAnaloguesFastorSums(const gsl::span<const Digit> sdigits);
  // std::vector<Digit> makeAnaloguesFastorSums(const gsl::span<const Digit> sdigits);

  void setEventTime(o2::InteractionTimeRecord record);
  // double getTriggerTime() const { return mDigits.getTriggerTime(); }
  // double getEventTime() const { return mDigits.getEventTime(); }
  // bool isLive(double t) const { return mDigits.isLive(t); }
  // bool isLive() const { return mDigits.isLive(); }

  /// Sets geometry for trigger mapping
  void setGeometry(o2::emcal::Geometry* gm) { mGeometry = gm; }

  void setWindowStartTime(int time) { mTimeWindowStart = time; }
  void setDebugStreaming(bool doStreaming) { mEnableDebugStreaming = doStreaming; }

  // function returns true if the collision occurs 600ns before the readout window is open
  // bool preTriggerCollision() const { return mDigits.preTriggerCollision(); }

  void fillOutputContainer(std::vector<Digit>& digits, o2::dataformats::MCTruthContainer<o2::emcal::MCLabel>& labels);

  bool doSmearEnergy() const { return mSmearEnergy; }
  double smearEnergy(double energy);
  bool doSimulateTimeResponse() const { return mSimulateTimeResponse; }

  void sampleSDigit(const Digit& sdigit);

  /// Close the TreeStreamer to make the file readable
  void endDebugStream() { mDebugStream->Close(); mDebugStreamPatch->Close(); }

  /// Getter for debug mode
  bool isDebugMode() { return mEnableDebugStreaming; }

  /// Getter for patches
  std::vector<Patches> getPatchesVector() { return patchesFromAllTRUs; }

  /// raw pointers used here to allow interface with TF1
  static double rawResponseFunction(double* x, double* par);

  // const std::vector<o2::emcal::Digit> &getDigits() const { return mDigits.getDigits(); }
  // const std::vector<o2::emcal::TriggerRecord> &getTriggerRecords() const { return mDigits.getTriggerRecords(); }
  // const o2::dataformats::MCTruthContainer<o2::emcal::MCLabel> &getMCLabels() const { return mDigits.getMCLabels(); }

 private:
  short mEventTimeOffset = 0; ///< event time difference from trigger time (in number of bins)
  short mPhase = 0;           ///< event phase
  UInt_t mROFrameMin = 0;     ///< lowest RO frame of current digits
  UInt_t mROFrameMax = 0;     ///< highest RO frame of current digits
  // bool mSmearEnergy = false;            ///< do time and energy smearing
  bool mSmearEnergy = true;            ///< do time and energy smearing
  bool mSimulateTimeResponse = true;   ///< simulate time response
  const SimParam* mSimParam = nullptr; ///< SimParam object

  std::vector<Digit> mTempDigitVector; ///< temporary digit storage
  // std::unordered_map<Int_t, std::list<LabeledDigit>> mDigits; ///< used to sort digits and labels by tower
  o2::emcal::DigitsWriteoutBufferTRU mDigits; ///< used to sort digits by tower
  o2::emcal::LZEROElectronics LZERO;          ///< to start the trigger
  std::vector<Patches> patchesFromAllTRUs;    ///< patches from all TRUs

  TRandom3* mRandomGenerator = nullptr;                  ///< random number generator
  std::vector<std::vector<double>> mAmplitudeInTimeBins; ///< amplitude of signal for each time bin

  TriggerMappingV2* mTriggerMap = nullptr; ///< Trigger map for tower to fastor ID
  Geometry* mGeometry = nullptr;           ///< EMCAL geometry

  int mTimeWindowStart = 7; ///< The start of the time window
  int mDelay = 7;           ///< number of (full) time bins corresponding to the signal time delay

  std::unique_ptr<o2::utils::TreeStreamRedirector> mDebugStream = nullptr;
  std::unique_ptr<o2::utils::TreeStreamRedirector> mDebugStreamPatch = nullptr;
  bool mEnableDebugStreaming = false;

  ClassDefOverride(DigitizerTRU, 1);
};
} // namespace emcal
} // namespace o2

#endif /* ALICEO2_EMCAL_TRIGGERDIGITIZER_H */