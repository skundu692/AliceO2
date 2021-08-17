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

/// \file HFD0CandidateSelector.cxx
/// \brief D0(bar) → π± K∓ selection task
///
/// \author Nima Zardoshti <nima.zardoshti@cern.ch>, CERN
/// \author Vít Kučera <vit.kucera@cern.ch>, CERN

#include "Framework/runDataProcessing.h"
#include "Framework/AnalysisTask.h"
#include "AnalysisDataModel/HFSecondaryVertex.h"
#include "AnalysisDataModel/HFCandidateSelectionTables.h"
#include "AnalysisCore/TrackSelectorPID.h"
#include "ALICE3Analysis/RICH.h"
#include "Framework/runDataProcessing.h"

using namespace o2;
using namespace o2::framework;
using namespace o2::aod::hf_cand_prong2;
using namespace o2::analysis::hf_cuts_d0_topik;




namespace o2::aod
{
namespace hf_track_index_alice3_pid
{
DECLARE_SOA_INDEX_COLUMN(Track, track); //!
DECLARE_SOA_INDEX_COLUMN(RICH, rich);
} // namespace hf_track_index_alice3_pid

  DECLARE_SOA_INDEX_TABLE_USER(HfTrackIndexALICE3PID, Tracks, "HFTRKIDXA3PID",//!
                             hf_track_index_alice3_pid::TrackId,
                             hf_track_index_alice3_pid::RICHId);
} // namespace o2::aod

struct Alice3PidIndexBuilder {
  Builds<o2::aod::HfTrackIndexALICE3PID> index;
  void init(o2::framework::InitContext&) {}
};







/// Struct for applying D0 selection cuts
struct HFD0CandidateSelector {
  Produces<aod::HFSelD0Candidate> hfSelD0Candidate;

  Configurable<double> d_pTCandMin{"d_pTCandMin", 0., "Lower bound of candidate pT"};
  Configurable<double> d_pTCandMax{"d_pTCandMax", 50., "Upper bound of candidate pT"};
  // TPC
  Configurable<double> d_pidTPCMinpT{"d_pidTPCMinpT", 0.15, "Lower bound of track pT for TPC PID"};
  Configurable<double> d_pidTPCMaxpT{"d_pidTPCMaxpT", 5., "Upper bound of track pT for TPC PID"};
  Configurable<double> d_nSigmaTPC{"d_nSigmaTPC", 3., "Nsigma cut on TPC only"};
  Configurable<double> d_nSigmaTPCCombined{"d_nSigmaTPCCombined", 5., "Nsigma cut on TPC combined with TOF"};
  //Configurable<double> d_TPCNClsFindablePIDCut{"d_TPCNClsFindablePIDCut", 50., "Lower bound of TPC findable clusters for good PID"};
  // TOF
  Configurable<double> d_pidTOFMinpT{"d_pidTOFMinpT", 0.15, "Lower bound of track pT for TOF PID"};
  Configurable<double> d_pidTOFMaxpT{"d_pidTOFMaxpT", 5., "Upper bound of track pT for TOF PID"};
  Configurable<double> d_nSigmaTOF{"d_nSigmaTOF", 3., "Nsigma cut on TOF only"};
  Configurable<double> d_nSigmaTOFCombined{"d_nSigmaTOFCombined", 5., "Nsigma cut on TOF combined with TPC"};
  //////////////////new//////////////////////////
  // RICH
  Configurable<double> d_pidRICHMinpT{"d_pidRICHMinpT", 0.15, "Lower bound of track pT for RICH PID"};
  Configurable<double> d_pidRICHMaxpT{"d_pidRICHMaxpT", 10., "Upper bound of track pT for RICH PID"};
  Configurable<double> d_nSigmaRICH{"d_nSigmaRICH", 3., "Nsigma cut on RICH only"};
  Configurable<double> d_nSigmaRICHCombinedTOF{"d_nSigmaRICHCombinedTOF", 5., "Nsigma cut on RICH combined with TOF"};
  //////////////////////
  // topological cuts
  Configurable<std::vector<double>> pTBins{"pTBins", std::vector<double>{hf_cuts_d0_topik::pTBins_v}, "pT bin limits"};
  Configurable<LabeledArray<double>> cuts{"D0_to_pi_K_cuts", {hf_cuts_d0_topik::cuts[0], npTBins, nCutVars, pTBinLabels, cutVarLabels}, "D0 candidate selection per pT bin"};

  /*
  /// Selection on goodness of daughter tracks
  /// \note should be applied at candidate selection
  /// \param track is daughter track
  /// \return true if track is good
  template <typename T>
  bool daughterSelection(const T& track)
  {
    if (track.tpcNClsFound() == 0) {
      return false; //is it clusters findable or found - need to check
    }
    return true;
  }
  */

  /// Conjugate-independent topological cuts
  /// \param candidate is candidate
  /// \return true if candidate passes all cuts

  template <typename T>
  bool selectionTopol(const T& candidate)
  {
    auto candpT = candidate.pt();
    auto pTBin = findBin(pTBins, candpT);
    if (pTBin == -1) {
      return false;
    }

    // check that the candidate pT is within the analysis range
    if (candpT < d_pTCandMin || candpT >= d_pTCandMax) {
      return false;
    }
    // product of daughter impact parameters
    if (candidate.impactParameterProduct() > cuts->get(pTBin, "d0d0")) {
      return false;
    }
    // cosine of pointing angle
    if (candidate.cpa() < cuts->get(pTBin, "cos pointing angle")) {
      return false;
    }
    // cosine of pointing angle XY
    if (candidate.cpaXY() < cuts->get(pTBin, "cos pointing angle xy")) {
      return false;
    }
    // normalised decay length in XY plane
    if (candidate.decayLengthXYNormalised() < cuts->get(pTBin, "normalized decay length XY")) {
      return false;
    }
    // candidate DCA
    //if (candidate.chi2PCA() > cuts[pTBin][1]) return false;

    // decay exponentail law, with tau = beta*gamma*ctau
    // decay length > ctau retains (1-1/e)
    if (std::abs(candidate.impactParameterNormalised0()) < 0.5 || std::abs(candidate.impactParameterNormalised1()) < 0.5) {
      return false;
    }
    double decayLengthCut = std::min((candidate.p() * 0.0066) + 0.01, 0.06);
    if (candidate.decayLength() * candidate.decayLength() < decayLengthCut * decayLengthCut) {
      return false;
    }
    if (candidate.decayLengthNormalised() * candidate.decayLengthNormalised() < 1.0) {
      //return false; // add back when getter fixed
    }
    return true;
  }

  /// Conjugate-dependent topological cuts
  /// \param candidate is candidate
  /// \param trackPion is the track with the pion hypothesis
  /// \param trackKaon is the track with the kaon hypothesis
  /// \note trackPion = positive and trackKaon = negative for D0 selection and inverse for D0bar
  /// \return true if candidate passes all cuts for the given Conjugate

  template <typename T1, typename T2>
  bool selectionTopolConjugate(const T1& candidate, const T2& trackPion, const T2& trackKaon)
  {
    auto candpT = candidate.pt();
    auto pTBin = findBin(pTBins, candpT);
    if (pTBin == -1) {
      return false;
    }

    // invariant-mass cut
    if (trackPion.sign() > 0) {
      if (std::abs(InvMassD0(candidate) - RecoDecay::getMassPDG(pdg::Code::kD0)) > cuts->get(pTBin, "m")) {
        return false;
      }
    } else {
      if (std::abs(InvMassD0bar(candidate) - RecoDecay::getMassPDG(pdg::Code::kD0)) > cuts->get(pTBin, "m")) {
        return false;
      }
    }

    // cut on daughter pT
    if (trackPion.pt() < cuts->get(pTBin, "pT Pi") || trackKaon.pt() < cuts->get(pTBin, "pT K")) {
      return false;
    }

    // cut on daughter DCA - need to add secondary vertex constraint here
    if (std::abs(trackPion.dcaPrim0()) > cuts->get(pTBin, "d0pi") || std::abs(trackKaon.dcaPrim0()) > cuts->get(pTBin, "d0K")) {
      return false;
    }

    // cut on cos(theta*)
    if (trackPion.sign() > 0) {
      if (std::abs(CosThetaStarD0(candidate)) > cuts->get(pTBin, "cos theta*")) {
        return false;
      }
    } else {
      if (std::abs(CosThetaStarD0bar(candidate)) > cuts->get(pTBin, "cos theta*")) {
        return false;
      }
    }

    return true;
  }

  
  using TracksPID = soa::Join<aod::BigTracksPID, aod::HfTrackIndexALICE3PID>;
  void process(aod::HfCandProng2 const& candidates, TracksPID const&, aod::RICHs const&)
  {
    TrackSelectorPID selectorPion(kPiPlus);
    selectorPion.setRangePtTPC(d_pidTPCMinpT, d_pidTPCMaxpT);
    selectorPion.setRangePtTOF(d_pidTOFMinpT, d_pidTOFMaxpT);
    selectorPion.setRangePtRICH(d_pidRICHMinpT, d_pidRICHMaxpT);
    
    selectorPion.setRangeNSigmaTPC(-d_nSigmaTPC, d_nSigmaTPC);
    selectorPion.setRangeNSigmaTOF(-d_nSigmaTOF, d_nSigmaTOF);
    selectorPion.setRangeNSigmaRICH(-d_nSigmaRICH, d_nSigmaRICH);
    
    selectorPion.setRangeNSigmaTOFCondTPC(-d_nSigmaTOFCombined, d_nSigmaTOFCombined);
    selectorPion.setRangeNSigmaRICHCondTOF(-d_nSigmaRICHCombinedTOF, d_nSigmaRICHCombinedTOF);
    
    TrackSelectorPID selectorKaon(selectorPion);
    
    
    // looping over 2-prong candidates
    for (auto& candidate : candidates)
      {
	int statusD0 = 0;
	int statusD0bar = 0;

	if (!(candidate.hfflag() & 1 << DecayType::D0ToPiK))
	  {
	    hfSelD0Candidate(statusD0, statusD0bar);
	    continue;
	  }
	
	auto trackPos = candidate.index0_as<TracksPID>(); // positive daughter
	auto trackNeg = candidate.index1_as<TracksPID>(); // negative daughter

	// conjugate-independent topological selection
	if (!selectionTopol(candidate))
	  {
	    hfSelD0Candidate(statusD0, statusD0bar);
	    continue;
	}
	
      // conjugate-dependent topological selection for D0
	bool topolD0 = selectionTopolConjugate(candidate, trackPos, trackNeg);
	bool topolD0bar = selectionTopolConjugate(candidate, trackNeg, trackPos);
	
	if (!topolD0 && !topolD0bar)
	  {
	    hfSelD0Candidate(statusD0, statusD0bar);
	    continue;
	  }

  

	//////////////////////TrackID checking////////////////////////
	////////step1: check track is pion not electron//////////////////////////
	if (selectorPion.isElectronAndNotPion(trackPos) || selectorPion.isElectronAndNotPion(trackNeg))
	{
	  hfSelD0Candidate(statusD0, statusD0bar);
	  continue;
	}

	////////step2: pion kaon selection and D0 selection//////////////////////////
	// track-level TOF PID selection
	int pidTrackPosKaonTOF = selectorKaon.getStatusTrackPIDTOF(trackPos);
	int pidTrackPosPionTOF = selectorPion.getStatusTrackPIDTOF(trackPos);
	int pidTrackNegKaonTOF = selectorKaon.getStatusTrackPIDTOF(trackNeg);
	int pidTrackNegPionTOF = selectorPion.getStatusTrackPIDTOF(trackNeg);
	
	// track-level RICH PID selection
	int pidTrackPosKaonRICH = selectorKaon.getStatusTrackPIDRICH(trackPos);
	int pidTrackPosPionRICH = selectorPion.getStatusTrackPIDRICH(trackPos);
	int pidTrackNegKaonRICH = selectorKaon.getStatusTrackPIDRICH(trackNeg);
	int pidTrackNegPionRICH = selectorPion.getStatusTrackPIDRICH(trackNeg);
	bool ispositivepionnotkaon=selectorKaon.isPionAndNotKaon(trackPos);
	bool isnegativepionnotkaon=selectorKaon.isPionAndNotKaon(trackNeg);
      
	int pidD0 = -1;
	int pidD0bar = -1;

	///////////check positive track pion not kaon as well as negetive track kaon//////////////////////
	if ((pidTrackPosPionTOF == TrackSelectorPID::Status::PIDAccepted && pidTrackNegKaonTOF == TrackSelectorPID::Status::PIDAccepted && ispositivepionnotkaon && (!isnegativepionnotkaon))||(pidTrackPosPionRICH == TrackSelectorPID::Status::PIDAccepted && pidTrackNegKaonRICH == TrackSelectorPID::Status::PIDAccepted && ispositivepionnotkaon && (!isnegativepionnotkaon)))
	  {
	    pidD0 = 1; // accept D0
	  }
	///////////check negative track pion not kaon as well as positive track kaon//////////////////////
	if ((pidTrackNegPionTOF == TrackSelectorPID::Status::PIDAccepted && pidTrackPosKaonTOF == TrackSelectorPID::Status::PIDAccepted && isnegativepionnotkaon && (!ispositivepionnotkaon))||(pidTrackNegPionRICH == TrackSelectorPID::Status::PIDAccepted && pidTrackPosKaonRICH == TrackSelectorPID::Status::PIDAccepted && isnegativepionnotkaon && (!isnegativepionnotkaon)))
	  {
	    pidD0bar = 1; // accept D0bar
	  }

	///////////////////step3: assigning status to D0////////////////
	if (pidD0 == -1 && pidD0bar == -1)
	  {
	    hfSelD0Candidate(statusD0, statusD0bar);
	    continue;
	  }
	if(pidD0==1 && topolD0)statusD0 = 1;
	if(pidD0bar==1 && topolD0bar)statusD0bar = 1;
	hfSelD0Candidate(statusD0, statusD0bar);
    }
  }
};

WorkflowSpec defineDataProcessing(ConfigContext const& cfgc)
{
  WorkflowSpec workflow{};
  workflow.push_back(adaptAnalysisTask<Alice3PidIndexBuilder>(cfgc));
  workflow.push_back(adaptAnalysisTask<HFD0CandidateSelector>(cfgc, TaskName{"hf-d0-candidate-selector"}));
  return workflow;
}
